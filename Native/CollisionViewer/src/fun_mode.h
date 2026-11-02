#pragma once
#include "fun_audio.h"
#include "fun_helmet.h"
#include "fun_music_bus.h"
#include "fun_camera.h"
#include "fun_control.h"

static std::array<float,4> funColor(double seconds) {
    // Caramelldansen's approximately 164 BPM: one saturated hue per beat.
    static constexpr std::array<float,4> colors[]{
        {.60f,.12f,1,.27f},{1,.12f,.55f,.27f},{.12f,.38f,1,.27f},
        {.12f,1,.70f,.25f},{1,.58f,.12f,.25f},{.85f,.12f,1,.27f}
    };
    auto beat=static_cast<uint64_t>(std::max(0.,seconds)/.365); return colors[beat%std::size(colors)];
}
static bool funNumber(const std::string& text,float& value) {
    if(text.empty()||text.size()>32||text.find_first_not_of("0123456789.+-eE")!=std::string::npos) return false;
    char* end=nullptr; value=strtof(text.c_str(),&end);
    return end==text.c_str()+text.size()&&std::isfinite(value)&&value>=0&&value<=1;
}
class FunMode {
    FunAudio audio;
    FunHelmet helmet;
    FunMusicBus musicBus;
    const MeathookHudApi* api=nullptr;
    uintptr_t music=0,master=0;
    static constexpr float trackVolume=.25f;
    bool active=false;
    bool controlSuspended=false;
    int lastCameraMode=-1;
    std::atomic<bool> screen{false};
    std::atomic<double> clock{0};
    InspectorField firstPerson,cinematic,firstPersonOrigin,controlledEntities;
    bool fieldsReady=false;
    bool readVolume(uintptr_t at,const char* name,float& value) const {
        int unused=0; std::string text;
        return at&&readLabCvarValue(WallMemory{},at,name,unused,text)&&funNumber(text,value);
    }
public:
    bool enabled() const { return screen.load(); }
    std::array<float,4> color() const { return funColor(clock.load()); }
    bool command(const MeathookHudApi& engine,const LabCommandSystem& commands,const std::vector<std::string>& args) {
        if(args.empty()||args[0]!="labplus_fun") return false;
        if(args.size()!=2||(args[1]!="0"&&args[1]!="1")) { engine.consolePrint("%s","labplus_fun 0|1\n"); return true; }
        if(args[1]=="0") { stop(); engine.consolePrint("%s","labplus_fun: off.\n"); return true; }
        if(active) { engine.consolePrint("%s","labplus_fun: already on.\n"); return true; }
        const auto* profile=currentGameProfile(); if(!profile) return true;
        const auto system=resolveLabCvarSystem(WallMemory{},commands,reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),*profile);
        if(!system) { engine.consolePrint("%s","labplus_fun: music channel unavailable.\n"); return true; }
        using Find=void* (*)(void*,const char*);
        auto find=[&](const char* name) { return reinterpret_cast<uintptr_t>(reinterpret_cast<Find>(system.find)(reinterpret_cast<void*>(system.object),name)); };
        music=find("s_volumeMusic"); master=find("s_volume"); float initial=0,masterValue=0;
        if(!readVolume(music,"s_volumeMusic",initial)||!readVolume(master,"s_volume",masterValue)) {
            engine.consolePrint("%s","labplus_fun: unknown music volume layout; nothing changed.\n"); return true;
        }
        firstPerson=inspectorField(engine,"idPlayer","isViewedFirstPersonLocally");
        cinematic=inspectorField(engine,"idPlayer","hideHudForCinematic");
        firstPersonOrigin=inspectorField(engine,"idPlayer","firstPersonViewOrigin");
        controlledEntities=inspectorField(engine,"idPlayer","controlledEntities");
        fieldsReady=firstPerson&&firstPerson.size==1&&firstPerson.type=="bool"&&cinematic&&cinematic.size==1&&cinematic.type=="bool";
        if(!fieldsReady) { engine.consolePrint("%s","labplus_fun: camera metadata unavailable; nothing changed.\n"); return true; }
        if(!musicBus.connect(engine,music,reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),*profile)) {
            engine.consolePrint("%s","labplus_fun: music routing is unavailable for this build; nothing changed.\n"); return true;
        }
        wchar_t path[32768]{};
        const auto length=GetEnvironmentVariableW(L"LABPLUS_FUN_AUDIO",path,32768);
        bool started=false;
        if(length&&length<32768) try { started=audio.start(path); } catch(...) { audio.stop(); }
        if(!started) {
            engine.consolePrint("%s","labplus_fun: bundled audio could not start; nothing changed.\n"); return true;
        }
        api=&engine;
        if(!musicBus.set(0)) {
            musicBus.set(initial,100); audio.stop(); engine.consolePrint("%s","labplus_fun: music mute failed; restored previous settings.\n"); return true;
        }
        helmet.initialize(engine,reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),profile);
        controlSuspended=false;active=true; engine.consolePrint("%s","labplus_fun: on.\n");
        note("Fun mode enabled; soundtrack suppressed separately from the live music volume slider. Third-person material validation pending."); return true;
    }
    void stop() {
        screen=false;
        if(!active) return;
        uintptr_t system=0,currentMap=0;
        if(const auto* profile=currentGameProfile()) {
            const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
            if(WallMemory{}.get(base+profile->gameSystemRva,system)) WallMemory{}.get(system+0x50,currentMap);
        }
        helmet.restore(currentMap);
        audio.stop(); float current=0;
        if(readVolume(music,"s_volumeMusic",current)) musicBus.set(current,500);
        active=false; note("Fun mode disabled; original soundtrack state restored. Music volume slider was left untouched.");
        controlSuspended=false;lastCameraMode=-1;
    }
    void frame() {
        if(!active) return;
        if(!labAppAvailable()||!audio.healthy()) { stop(); return; }
        const auto* profile=currentGameProfile(); if(!profile) { stop(); return; }
        auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); WallMemory memory;
        uintptr_t system=0,map=0,player=0,vtable=0; uint32_t state=0;
        bool scene=memory.get(base+profile->gameSystemRva,system)&&memory.get(system+0x44,state)&&memory.get(system+0x50,map)&&map;
        bool shell=scene&&state==0;
        bool live=scene&&state==2&&memory.get(map+profile->mapPlayerOffset,player)&&memory.get(player,vtable)&&vtable==base+profile->playerVtableRva;
        const bool demonEntity=scene&&state==2&&player&&vtable&&!live&&funDemonPlayerClass(readEntityClass(memory,vtable,base,*profile));
        if((live&&funControlledDemon(memory,player,controlledEntities,base,*profile))||demonEntity) {
            screen=false;helmet.restore(map);audio.mix(0,true);
            float currentMusic=0;
            if(!readVolume(music,"s_volumeMusic",currentMusic)||!musicBus.set(currentMusic,controlSuspended?0:100)) {stop();return;}
            if(!controlSuspended) note("Fun suspended during demon/AI control; original game music restored.");
            controlSuspended=true;lastCameraMode=-1;clock=audio.seconds();return;
        }
        if(controlSuspended) {controlSuspended=false;note("Fun resumed after returning control to the Slayer.");}
        unsigned char fp=0,cine=0;
        bool view=live&&fieldsReady&&memory.get(player+firstPerson.offset,fp)&&fp<=1&&memory.get(player+cinematic.offset,cine)&&cine<=1;
        bool playing=live&&gameplayVisible(memory,base,*profile);
        unsigned char paused=1;
        const bool audioPaused=!live||!memory.get(player+profile->playerHudOffset+profile->hudPausedOffset,paused)||paused!=0;
        GameCamera camera{};const bool haveCamera=readGameCamera(memory,base,*profile,camera);
        float povOrigin[3]{};
        const bool havePovOrigin=view&&firstPersonOrigin&&firstPersonOrigin.type=="idVec3"&&firstPersonOrigin.size==12&&
            memory.read(player+firstPersonOrigin.offset,povOrigin,12);
        auto animatedPov=FunAnimatedView::Unknown;
        FunShotView shot;
        if(view&&cine&&haveCamera) {
            helmet.update(*api,base,*profile,map,funColor(audio.seconds()),false);
            shot=helmet.directedCamera(camera);
            animatedPov=shot.view!=FunAnimatedView::Unknown?shot.view:helmet.cameraAtAnimatedBody(camera);
        }
        const bool pov=view&&funPlayerPov(fp!=0,cine!=0,haveCamera?&camera:nullptr,havePovOrigin?povOrigin:nullptr,animatedPov);
        const int mode=shell?0:!view?1:pov?(cine?3:2):(cine?5:4);
        if(mode!=lastCameraMode) {
            static constexpr const char* messages[]{"Fun camera: shell; visor only, music muffled.","Fun camera: unavailable; screen tint withheld, music muffled.",
                "Fun camera: player POV; screen tint on, music clear.","Fun camera: cinematic player POV; screen tint on, music clear.",
                "Fun camera: third person; visor only, music muffled.","Fun camera: external cinematic; visor only, music muffled."};
            note(messages[mode]);lastCameraMode=mode;
        }
        FunSpatialMix spatial;
        if(shell||(view&&!pov)) {
            helmet.update(*api,base,*profile,map,funColor(audio.seconds()));
            if(!shell&&haveCamera) {
                // An unregistered generic cinematic camera can have only old
                // actors from previous scenes in the cache. Its actual Slayer
                // helmet origin is preferable to those unrelated old models.
                if(!cine||shot.view!=FunAnimatedView::Unknown||!shot.actorPrefix.empty()) spatial=helmet.spatial(camera,shot.actorPrefix);
                else spatial.gain=0;
                if(cine) spatial=funCinematicSpatialMix(camera,spatial,havePovOrigin?povOrigin:nullptr);
            }
        }
        else if(view&&cine) helmet.clearColors(map);
        else helmet.restore(map);
        float volume=0,currentMusic=0;
        if(!musicBus.set(0)) { stop(); return; }
        if(!readVolume(music,"s_volumeMusic",currentMusic)) currentMusic=0;
        if(!readVolume(master,"s_volume",volume)) volume=0;
        // The track stays audible behind menus and pause, with the same
        // helmet-style attenuation/filter transition used for an outside view.
        // Notification overlays may hide the visual overlay without pausing
        // the game. They must not duck this separate music stream.
        const float outsideGain=view&&!pov?2.f:1.f;
        audio.mix(volume*currentMusic*trackVolume*spatial.gain*outsideGain,audioPaused||!pov,spatial.pan);
        clock=audio.seconds(); screen=playing&&pov;
    }
};
static FunMode& funMode() { static auto* value=new FunMode; return *value; }
