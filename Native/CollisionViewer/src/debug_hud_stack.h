#pragma once
#include "meathook_api.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <mutex>
#include <string>
#include <vector>
#include <sstream>
#include <locale>
#include <memory>

static std::atomic<float> debugHudScale{1.f};
static bool parseDebugHudScale(const std::string& text,float& value) {
    if(text.empty()||text.size()>32) return false;
    if(!std::all_of(text.begin(),text.end(),[](char c) { return (c>='0'&&c<='9')||c=='.'||c=='+'||c=='-'||c=='e'||c=='E'; })) return false;
    std::istringstream input(text); input.imbue(std::locale::classic());
    double parsed=0;
    if(!(input>>std::noskipws>>parsed)||!input.eof()||!std::isfinite(parsed)||parsed<.5f||parsed>3.f) return false;
    value=static_cast<float>(parsed); return true;
}

enum class DebugHudBlock : size_t { Inspector, SupportPlane, Encounters, Enemies, Timers, Techs };
enum class DebugHudProducer : size_t { Inspector, Observations, Numbers };
struct DebugHudLine { std::string text; unsigned color=0xffeeeeeeu; bool sectionStart=false; };
struct DebugHudFrame { std::vector<DebugHudLine> rows; float scale=1; uint64_t generation=0; bool visible=true; uint64_t captured=0; };
static constexpr float debugHudFontScale=.72f,debugHudRowHeight=16.f;
// All engine text calls happen in this one game-frame callback. Producers only
// publish rows; layout is committed after every producer has run, independent of
// the order in which their features connected to meathook.
class DebugHudStack {
    using VisibilityProbe=bool (*)(const MeathookHudApi*);
    struct Row { std::string text; unsigned color; float scale,height; };
    struct Cell { unsigned x,y,color; float scale; std::string text; };
    struct Producer { void (*callback)(void*)=nullptr; void* user=nullptr; };
    std::array<std::vector<Row>,6> blocks;
    std::vector<Cell> previous;
    std::array<Producer,3> producers{};
    std::mutex registrations;
    std::atomic<const MeathookHudApi*> api{nullptr};
    std::shared_ptr<const DebugHudFrame> exported;
    std::atomic<bool> vulkanRendering{false};
    std::atomic<uint64_t> frameAt{0};
    std::atomic<VisibilityProbe> visibilityProbe{nullptr};
    uint64_t generation=0;
    bool registered=false,collecting=false,dirty=true;
    bool previousVulkan=false,hudVisible=true;
    float previousScale=0;
    static void callback(void* self) { static_cast<DebugHudStack*>(self)->onGameFrame(); }
    static bool same(const Cell& a,const Cell& b) { return a.x==b.x&&a.y==b.y&&a.color==b.color&&a.scale==b.scale&&a.text==b.text; }
    void render() {
        frameAt=GetTickCount64();
        const auto* backend=api.load();
        const float factor=debugHudScale.load();
        const bool vulkan=vulkanRendering.load();
        const bool changed=dirty||factor!=previousScale;
        if(!changed&&vulkan==previousVulkan) return;
        if(changed) {
            auto frame=std::make_shared<DebugHudFrame>(); frame->scale=factor; frame->generation=++generation; frame->visible=hudVisible; frame->captured=frameAt.load();
            for(const auto& block:blocks) {
                bool first=true;
                for(const auto& row:block) { frame->rows.push_back({row.text,row.color,first}); first=false; }
            }
            std::atomic_store(&exported,std::shared_ptr<const DebugHudFrame>(std::move(frame)));
        }
        // The engine's persistent queue clips y>480 on early game versions.
        // Once the Vulkan renderer is available it consumes the immutable
        // rows on the full viewport; never submit a second engine copy.
        if(vulkan||!hudVisible) {
            if(backend&&backend->removeText) for(const auto& old:previous) backend->removeText(old.x,old.y);
            previous.clear(); previousScale=factor; previousVulkan=vulkan; dirty=false; return;
        }
        if(!backend||!backend->addText||!backend->removeText) return;
        std::vector<Cell> next; double y=8; bool any=false;
        for(const auto& block:blocks) {
            if(block.empty()) continue;
            if(any) y+=4*factor;
            for(const auto& row:block) {
                next.push_back({8,static_cast<unsigned>(std::lround(y)),row.color,row.scale*factor,row.text});
                y+=row.height*factor;
            }
            any=true;
        }
        // Remove stale coordinates before adding relocated rows: old lower
        // blocks can occupy the new coordinates of a block that moved upward.
        for(const auto& old:previous) if(std::none_of(next.begin(),next.end(),[&](const Cell& cell) { return same(old,cell); })) backend->removeText(old.x,old.y);
        for(const auto& cell:next) if(std::none_of(previous.begin(),previous.end(),[&](const Cell& old) { return same(old,cell); })) backend->addText(cell.x,cell.y,cell.color,cell.scale,"%s",cell.text.c_str());
        previous=std::move(next); previousScale=factor; previousVulkan=false; dirty=false;
    }
public:
    std::shared_ptr<const DebugHudFrame> snapshot() const { return std::atomic_load(&exported); }
    uint64_t lastFrameAt() const { return frameAt.load(); }
    void setVulkanRendering(bool enabled) { vulkanRendering=enabled; }
    void setVisibilityProbe(VisibilityProbe probe) { visibilityProbe=probe; }
    void connect(const MeathookHudApi* backend,DebugHudProducer producer,void (*function)(void*),void* user) {
        bool install=false;
        {
            std::lock_guard<std::mutex> lock(registrations);
            api=backend; producers[static_cast<size_t>(producer)]={function,user};
            if(!registered) { registered=true; install=true; }
        }
        if(install) backend->registerPreFrame(callback,this);
    }
    void onGameFrame() {
        std::array<Producer,3> current;
        { std::lock_guard<std::mutex> lock(registrations); current=producers; }
        collecting=true;
        for(const auto& producer:current) if(producer.callback) producer.callback(producer.user);
        const auto probe=visibilityProbe.load();
        const bool visible=probe?probe(api.load()):true;
        if(visible!=hudVisible) { hudVisible=visible; dirty=true; }
        collecting=false; render();
    }
    template<class Rows> void publish(DebugHudBlock block,const MeathookHudApi* backend,const Rows& lines,float,float,unsigned title) {
        if(api.exchange(backend)!=backend) dirty=true;
        auto& rows=blocks[static_cast<size_t>(block)]; std::vector<Row> next;
        for(size_t i=0;i<lines.size();++i) if(!lines[i].empty()) next.push_back({lines[i],i?0xffeeeeeeu:title,debugHudFontScale,debugHudRowHeight});
        if(rows.size()!=next.size()||!std::equal(rows.begin(),rows.end(),next.begin(),[](const Row& a,const Row& b) {
            return a.text==b.text&&a.color==b.color&&a.scale==b.scale&&a.height==b.height;
        })) { rows=std::move(next); dirty=true; }
        if(!collecting) render();
    }
    void clear(DebugHudBlock block) { auto& rows=blocks[static_cast<size_t>(block)]; if(!rows.empty()) { rows.clear(); dirty=true; } if(!collecting) render(); }
};
static DebugHudStack& debugHudStack() { static auto* stack=new DebugHudStack; return *stack; }
