#include "../src/vulkan_layer.cpp"
#include <map>
#include <cstdarg>
static void expect(bool value,const char* text) { if(!value) { fprintf(stderr,"FAIL: %s\n",text); exit(1); } }
struct Drawn { std::string text; float scale; };
static std::map<std::pair<unsigned,unsigned>,Drawn> drawn;
static std::vector<char> operations;
static void (*engineCallback)(void*)=nullptr;
static void* engineUser=nullptr;
static unsigned registrations=0,visibility=0;
static std::string console;
static void add(unsigned x,unsigned y,unsigned,float scale,const char* format,...) {
    char text[1024];va_list args;va_start(args,format);vsprintf_s(text,format,args);va_end(args);
    drawn[{x,y}]={text,scale};operations.push_back('a');
}
static void remove(unsigned x,unsigned y) { drawn.erase({x,y});operations.push_back('r'); }
static void execute(const char* command) { expect(strcmp(command,"com_debugHUD 1")==0,"only native HUD visibility is requested");++visibility; }
static void print(const char* format,...) { char text[1024];va_list args;va_start(args,format);vsprintf_s(text,format,args);va_end(args);console=text; }
static void registerFrame(void (*callback)(void*),void* user) { ++registrations;engineCallback=callback;engineUser=user; }
static bool debugVisible=true;
static bool visibilityProbe(const MeathookHudApi*) { return debugVisible; }
struct Sources {
    DebugHudStack* stack;
    const MeathookHudApi* api;
    unsigned visible=0;
    std::array<std::string,10> inspector{"Volume inspector","Name","Class"};
    std::array<std::string,6> support{"Support plane","Normal","Slope","XY","XZ","YZ"};
    std::array<std::string,14> encounters{"Active encounters","Count","Manager","Script"};
    static void inspectorFrame(void* raw) {
        auto& self=*static_cast<Sources*>(raw);
        if(self.visible&1) self.stack->publish(DebugHudBlock::Inspector,self.api,self.inspector,20,.8f,1);else self.stack->clear(DebugHudBlock::Inspector);
    }
    static void observationsFrame(void* raw) {
        auto& self=*static_cast<Sources*>(raw);
        if(self.visible&2) self.stack->publish(DebugHudBlock::SupportPlane,self.api,self.support,17,.7f,1);else self.stack->clear(DebugHudBlock::SupportPlane);
        if(self.visible&4) self.stack->publish(DebugHudBlock::Encounters,self.api,self.encounters,16,.72f,1);else self.stack->clear(DebugHudBlock::Encounters);
    }
    void verify(float scale) {
        std::map<std::pair<unsigned,unsigned>,Drawn> expected;
        double y=8;bool any=false;
        auto block=[&](const auto& rows,unsigned flag) {
            if(!(visible&flag))return;
            if(any)y+=4*scale;
            for(const auto& text:rows)if(!text.empty()){expected[{8,static_cast<unsigned>(std::lround(y))}]={text,debugHudFontScale*scale};y+=debugHudRowHeight*scale;}
            any=true;
        };
        block(inspector,1);block(support,2);block(encounters,4);
        expect(drawn.size()==expected.size(),"layout removes old coordinates and reserves only actual nonempty rows");
        for(const auto& pair:expected){auto found=drawn.find(pair.first);expect(found!=drawn.end()&&found->second.text==pair.second.text&&found->second.scale==pair.second.scale,"blocks stack Inspector/Support/Encounters at scaled row heights in top-left coordinates");}
        bool added=false;for(char operation:operations){if(operation=='a')added=true;else expect(!added,"old positions are cleared before any relocated row is added");}
        if(visible==4)expect(drawn.begin()->first==std::make_pair(8u,8u)&&drawn.begin()->second.text=="Active encounters","encounters alone starts at the very top left");
    }
};
int main() {
    MeathookHudApi api{};api.addText=add;api.removeText=remove;api.executeCommand=execute;api.consolePrint=print;api.registerPreFrame=registerFrame;
    for(bool reverse:{false,true})for(float scale:{.5f,1.f,1.5f,3.f}) {
        DebugHudStack stack;Sources source{&stack,&api};drawn.clear();registrations=0;debugHudScale=scale;
        if(reverse)stack.connect(&api,DebugHudProducer::Observations,Sources::observationsFrame,&source);
        stack.connect(&api,DebugHudProducer::Inspector,Sources::inspectorFrame,&source);
        if(!reverse)stack.connect(&api,DebugHudProducer::Observations,Sources::observationsFrame,&source);
        expect(registrations==1,"all features share one game-frame callback regardless of registration order");
        for(unsigned mask:{7u,6u,4u,0u,1u,5u,3u,2u}) {
            source.visible=mask;operations.clear();engineCallback(engineUser);source.verify(scale);
        }
        source.visible=7;source.inspector[3]="ID";source.inspector[7]="Manager";source.support[2].clear();source.encounters[13]="Unavailable";
        operations.clear();engineCallback(engineUser);source.verify(scale);
        source.inspector={"Volume inspector","Aim at a volume"};source.support={"Support plane","No contact"};source.encounters={"Active encounters","Count"};
        operations.clear();engineCallback(engineUser);source.verify(scale);
        debugHudScale=scale==3?.5f:3.f;operations.clear();engineCallback(engineUser);source.verify(debugHudScale.load());
    }
    {
        // The exported frame retains every row, independent of the engine's
        // 480-high persistent text canvas. Turning on Vulkan removes only our
        // old persistent rows, and unchanged callbacks reuse the same frame.
        DebugHudStack stack;drawn.clear();debugHudScale=1;
        std::vector<std::string> many;
        for(unsigned i=0;i<80;++i)many.push_back("Enemy "+std::to_string(i));
        stack.publish(DebugHudBlock::Enemies,&api,many,99,2.5f,1);
        auto first=stack.snapshot();
        expect(first&&first->rows.size()==80&&first->rows.back().text=="Enemy 79"&&first->rows.front().sectionStart&&!first->rows[1].sectionStart,"export contains all dynamic rows without line caps");
        for(const auto& pair:drawn)expect(pair.second.scale==debugHudFontScale,"all blocks use the same font size despite old caller style values");
        stack.setVulkanRendering(true);stack.onGameFrame();
        expect(drawn.empty()&&stack.snapshot()==first&&stack.lastFrameAt()!=0,"Vulkan takes over without duplicate persistent text or unnecessary snapshot allocations");
        auto immutable=stack.snapshot();many[0]="Changed";
        stack.publish(DebugHudBlock::Enemies,&api,many,16,.72f,1);
        expect(immutable->rows.front().text=="Enemy 0"&&stack.snapshot()->rows.front().text=="Changed"&&stack.snapshot()->generation>immutable->generation,"published frames are immutable and generation changes with content");
        stack.setVisibilityProbe(visibilityProbe);debugVisible=false;stack.onGameFrame();
        expect(!stack.snapshot()->visible&&stack.snapshot()->rows.size()==80,"actual debug HUD visibility hides rendering without discarding rows");
        debugVisible=true;stack.onGameFrame();expect(stack.snapshot()->visible,"actual debug HUD visibility restores the complete frame");
        std::atomic<bool> stop{false},consistent{true};
        std::thread reader([&] {
            while(!stop.load()) { auto frame=stack.snapshot(); if(!frame||frame->rows.size()!=80||frame->rows[79].text!="Enemy 79")consistent=false; }
        });
        for(unsigned i=0;i<100;++i) { many[0]="Frame "+std::to_string(i);stack.publish(DebugHudBlock::Enemies,&api,many,16,.72f,1); }
        stop=true;reader.join();expect(consistent.load(),"render-thread snapshots remain complete during game-thread publication");
        stack.clear(DebugHudBlock::Enemies);expect(stack.snapshot()->rows.empty(),"clearing exports empty content immediately");
        stack.setVulkanRendering(false);stack.publish(DebugHudBlock::Timers,&api,std::vector<std::string>{"Timer"},16,.72f,1);
        expect(drawn.size()==1,"engine fallback can resume when Vulkan rendering is unavailable");
    }
    {
        DebugHudStack stack;stack.setVulkanRendering(true);ActiveEncounterHud hud(&api,&stack);debugHudScale=1;
        ActiveEncounterSnapshot snapshot;snapshot.phase="live";snapshot.captured=1000;
        for(unsigned i=0;i<7;++i) { ActiveEncounterRow row;row.known=true;row.think=1;row.id=i;row.name="manager_"+std::to_string(i);row.label=std::string(120,'A')+std::to_string(i);snapshot.rows.push_back(row); }
        hud.update(&snapshot,true,true,1000);const auto frame=stack.snapshot();
        expect(frame&&frame->rows.size()==16&&frame->rows.back().text.find("manager_6")!=std::string::npos,"all active encounter managers are exported beyond the former five-manager cap");
        expect(frame->rows[2].text.size()>120,"long encounter labels are preserved for viewport wrapping instead of truncated");
    }
    // Exercise real block formatters together, including stale/menu/disable
    // removal. A stale higher block must not leave space above lower blocks.
    {
        DebugHudStack stack;SupportPlaneHud support(&stack);ActiveEncounterHud encounters(&api,&stack);drawn.clear();debugHudScale=1;
        PlayerMotion player;player.valid=true;player.contact=true;player.gravity=true;player.normal={0,0,1};player.up={0,0,1};player.tick=1000;
        ActiveEncounterSnapshot snapshot;snapshot.phase="live";snapshot.captured=1000;
        support.update(api,&player,true,1000);encounters.update(&snapshot,true,true,1000);
        expect(drawn[{8,108}].text=="LAB Plus | Active encounters","real encounters follow six uniform support rows with a scaled gap");
        player.contact=false;player.tick=1100;support.update(api,&player,true,1100);
        expect(drawn[{8,108}].text=="LAB Plus | Active encounters"&&drawn[{8,88}].text=="YZ tangent: -","loss of ground contact keeps every fixed support row and lower-block position");
        support.update(api,&player,true,1400);
        expect(drawn.begin()->first.second==8&&drawn.begin()->second.text=="LAB Plus | Active encounters","stale support consumes zero space immediately");
        encounters.update(&snapshot,true,false,1400);expect(drawn.empty(),"menu or console gates clear every encounter row");
        encounters.update(&snapshot,true,true,4000);expect(drawn.empty(),"stale encounter data consumes zero rows");
    }
    for(const char* value:{"0.5","1","1.5","3","2.25","1e0"})expect(validLabArguments({"labplus_debughud_scale",value}),"fractional scale and endpoints are valid action arguments");
    for(const char* value:{"","0","0.499","3.001","0.499999999","3.00000001","nan","inf","NaN","1,5","1x","1 2","0x1.8p0"})if(validLabArguments({"labplus_debughud_scale",value})){fprintf(stderr,"Accepted invalid scale: [%s]\n",value);expect(false,"nonfinite, out-of-range and non-invariant scale values are rejected");}
    expect(validLabArguments({"labplus_debughud_scale"})&&!validLabArguments({"labplus_debughud_scale","1","2"}),"no-argument query is valid and extra values are rejected");
    {
        wchar_t temporary[MAX_PATH]{},directory[MAX_PATH]{};
        expect(GetTempPathW(MAX_PATH,temporary)&&GetTempFileNameW(temporary,L"lph",0,directory),"temporary viewer settings path");
        DeleteFileW(directory);expect(CreateDirectoryW(directory,nullptr)!=0,"temporary viewer settings directory");
        SetEnvironmentVariableW(L"LABPLUS_DATA_DIRECTORY",directory);auto ini=(std::filesystem::path(directory)/L"viewer.ini").wstring();
        WritePrivateProfileStringW(L"Viewer",L"DebugHudScale",L"1.5",ini.c_str());updateViewerSettings();expect(debugHudScale.load()==1.5f,"persisted invariant fractional scale reaches the native layout");
        WritePrivateProfileStringW(L"Viewer",L"DebugHudScale",L"3",ini.c_str());Sleep(260);updateViewerSettings();expect(debugHudScale.load()==3.f,"live settings update scale independently of row formatting throttles");
        WritePrivateProfileStringW(L"Viewer",L"DebugHudScale",L"nan",ini.c_str());Sleep(260);updateViewerSettings();expect(debugHudScale.load()==3.f,"invalid external settings preserve the last valid scale");
        WritePrivateProfileStringW(L"Viewer",L"DebugHudScale",L"1.5",ini.c_str());Sleep(260);updateViewerSettings();
        DeleteFileW(ini.c_str());RemoveDirectoryW(directory);
    }
    expect(printDebugHudScaleQuery(api,{"labplus_debughud_scale"})&&console=="labplus_debughud_scale is 1.5 (0.5..3)\n"&&debugHudScale.load()==1.5f,"native no-argument query prints the current invariant fractional scale without changing it");
    puts("PASS: all eight visibility combinations in both callback registration orders; compact dynamic rows, stale/menu removal, coordinate cleanup and scaling at0.5/1/1.5/3; scale command validation/query.");
}
