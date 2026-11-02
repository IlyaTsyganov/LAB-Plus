#pragma once
#include "practice_scope.h"
#include "practice_profile.h"
static uint64_t encounterEpochMillis();
static std::string practiceClean(std::string text) {
    for(char& c:text) if(static_cast<unsigned char>(c)<32||c==127) c=' ';
    if(text.size()>2048) text.resize(2048); return text;
}
// Live checkpoint archival/restore through explicit, patch-specific save/load adapters.
// Engine calls and game-save replacement run only in meathook's frame callback.
// Presentation can service status/capture while that callback is suspended at
// the loading prompt. That path only reads the game and copies archive files.
// Account ownership is never changed.
struct PracticeRuntime {
    uintptr_t system=0,map=0,record=0;
    uint32_t user=0,entryIndex=~0u,engineState=0;
    std::string folder,slot,level,error;
    const PracticeProfile* adapter=nullptr;
    bool busy=false;
    explicit operator bool() const { return error.empty()&&system&&map&&record; }
};
static PracticeRuntime readPracticeRuntime(const WallMemory& memory,uintptr_t base,const GameProfile* profile,bool requireIdle=true,bool allowLoadingCapture=false) {
    PracticeRuntime out;
    auto fail=[&](const char* message) { out.error=message; return out; };
    auto busy=[&](const char* message) { out.busy=true; return fail(message); };
    const auto* adapter=practiceProfile(profile); out.adapter=adapter;
    if(!adapter) return fail("In-game saves require an audited save/load adapter for this patch.");
    uint32_t state=0,pending=~0u,type=0; unsigned char requested=1,paused=0,removed=1;
    uintptr_t ghost=0,globalGhost=0,users=0,master=0,shared=0,player=0,vt=0;
    if(!memory.get(base+profile->gameSystemRva,out.system)||out.system!=base+adapter->systemRva||
       !memory.get(out.system+0x44,state)||(state!=2&&state!=1)||!memory.get(out.system+0x50,out.map)||!out.map)
        return fail("Load the selected campaign slot first.");
    out.engineState=state;
    // At the completed loading prompt retail remains in state 1 until continued.
    // Reading an idle checkpoint there is safe; starting another engine load
    // must wait for state 2. No engine calls or file replacement in state 1.
    if(state==1&&!allowLoadingCapture) return busy("Continue into the loaded level in the game before loading an archive.");
    if(!memory.get(out.map+adapter->mapTypeOffset,type)||type!=1||!memory.get(out.map+0x20,shared)) return fail("Campaign map unavailable.");
    out.level=readEntityText(memory,shared,512);

    if(!memory.get(out.map+profile->mapPlayerOffset,player)||!memory.get(player,vt)||vt!=base+profile->playerVtableRva||
       !memory.get(player+0xa4,removed)||removed||!memory.get(player+profile->playerHudOffset+profile->hudPausedOffset,paused)||paused>1)
        return fail("The campaign player is not ready.");
    if(!memory.get(out.system+adapter->pendingOffset,pending)||!memory.get(out.system+0xb8,requested)) return fail("Cannot read engine save/map-change state.");
    if(requested||(requireIdle&&pending)) return busy("Waiting for the engine save/map-change queue.");
    if(!memory.get(out.system+adapter->ghostOffset,ghost)||!ghost||!memory.get(base+adapter->ghostRva,globalGhost)||ghost!=globalGhost)
        return fail("Save manager unavailable.");
    if(requireIdle&&adapter->slotsOffset==0x68) {
        // Legacy save manager: outstanding save work is a list, not a shared queue.
        uint64_t writers=~0ULL;
        if(!memory.get(ghost+0x40,writers)) return fail("Cannot read save writer state.");
        if(writers) return busy("Waiting for autosave to finish.");
        for(auto offset:{0x50u,0x58u}) {
            uintptr_t task=0,object=0;
            if(!memory.get(ghost+offset,task)||(task&&!memory.get(task+8,object))) return fail("Cannot read save task state.");
            if(object) return busy("Waiting for the engine save/load task.");
        }
    } else if(requireIdle) {
        for(auto offset:{0x50u,0x58u,0x60u}) {
            uintptr_t task=0,object=0,count=0;
            if(!memory.get(ghost+offset,task)||(task&&!memory.get(task+8,object))) return fail("Cannot read save task state.");
            if(offset==0x58) {
                if(object&&!memory.get(object+8,count)) return fail("Cannot read save writer queue.");
                if(count) return busy("Waiting for autosave to finish.");
            } else if(object) return busy("Waiting for the engine save/load task.");
        }
    }
    if(!memory.get(base+adapter->usersRva,users)||!memory.get(users+0x18,shared)||!shared||
       !memory.get(shared+8,master)||!master||!memory.get(master+4,out.user)) return fail("Master local user unavailable.");
    uintptr_t data=0; uint64_t count=0;
    if(!memory.get(ghost+adapter->slotsOffset,data)||!memory.get(ghost+adapter->slotsOffset+8,count)||count>8) return fail("User slot list unavailable.");
    for(uint64_t i=0;i<count;++i) {
        uintptr_t object=0; uint32_t handle=0;
        if(!memory.get(data+i*8,shared)||!shared||!memory.get(shared+8,object)||!object||!memory.get(object,handle)) return fail("Unreadable user slot record.");
        if(handle!=out.user) continue;
        if(out.record) return fail("Ambiguous master user slot.");
        out.record=object+8;
        if(!memory.get(out.record,out.entryIndex)||!memory.get(out.record+16,shared)) return fail("Current slot unavailable.");
        out.folder=readEntityText(memory,shared,64);
    }
    // This is an index into the user's discovered save entries, not a physical
    // or campaign-relative slot number. Retail GetCurrentSlotInfo uses +0x80
    // data / +0x88 count on the user object, with 0xe0-byte entries.
    uintptr_t entries=0,entryFolder=0; uint32_t entryCount=0;
    if(!out.record||!memory.get(out.record+0x78,entries)||!entries||!memory.get(out.record+0x80,entryCount)||entryCount>64||out.entryIndex>=entryCount||
       !memory.get(entries+size_t(out.entryIndex)*0xe0+0x78,entryFolder)||readEntityText(memory,entryFolder,64)!=out.folder) return fail("Selected save-list entry is unavailable.");
    auto scope=practiceRuntimeSlot(out.folder);
    if(!scope||!practiceMapMatches(out.level,*scope)||!practiceSlotAvailable(scope->slot,adapter->slotCount)) return fail("Current campaign and slot do not match.");
    out.slot=scope->slot;
    return out;
}
static bool practiceRequestMatches(const PracticeRuntime& state,const std::string& requestedSlot) {
    return bool(state)&&state.slot==requestedSlot;
}
struct PracticeIdleWindow {
    ULONGLONG since=0;
    PracticeRuntime initial;
    void reset() { since=0; initial={}; }
    bool update(const PracticeRuntime& state,ULONGLONG now) {
        if(!state) { reset(); return false; }
        if(!since||initial.engineState!=state.engineState||initial.map!=state.map||initial.record!=state.record||initial.user!=state.user||initial.slot!=state.slot||initial.entryIndex!=state.entryIndex) {
            initial=state; since=now; return false;
        }
        return now-since>=500;
    }
};
static bool practiceCloudDisabled(int accountEnabled,int appEnabled) {
    return accountEnabled>=0&&accountEnabled<=1&&appEnabled>=0&&appEnabled<=1&&!(accountEnabled&&appEnabled);
}
static bool practiceMayRunFromPresent(const std::string& operation) { return operation=="status"||operation=="archive"; }
struct PracticeSteamIdentity {
    uint64_t account=0;
    bool cloudDisabled=false;
    int cloudAccount=-1,cloudApp=-1;
};
static PracticeSteamIdentity readPracticeSteam() {
    auto module=GetModuleHandleW(L"steam_api64.dll"); if(!module) return {};
    using User=int(__cdecl*)(); using Find=void*(__cdecl*)(int,const char*);
    using Id=uint64_t(__cdecl*)(void*); using Enabled=bool(__cdecl*)(void*);
    auto user=reinterpret_cast<User>(GetProcAddress(module,"SteamAPI_GetHSteamUser"));
    auto find=reinterpret_cast<Find>(GetProcAddress(module,"SteamInternal_FindOrCreateUserInterface"));
    auto id=reinterpret_cast<Id>(GetProcAddress(module,"SteamAPI_ISteamUser_GetSteamID"));
    auto cloudAccount=reinterpret_cast<Enabled>(GetProcAddress(module,"SteamAPI_ISteamRemoteStorage_IsCloudEnabledForAccount"));
    auto cloudApp=reinterpret_cast<Enabled>(GetProcAddress(module,"SteamAPI_ISteamRemoteStorage_IsCloudEnabledForApp"));
    if(!user||!find||!id) return {};
    // Interface strings present in the audited retail EXE; use existing Steam
    // client interfaces only. Never initialize/shut down Steam or change Cloud.
    auto steamUser=find(user(),"SteamUser020"); if(!steamUser) return {};
    PracticeSteamIdentity state; state.account=id(steamUser);
    auto storage=find(user(),"STEAMREMOTESTORAGE_INTERFACE_VERSION014");
    if(!storage||!cloudAccount||!cloudApp) return state;
    state.cloudAccount=cloudAccount(storage)?1:0; state.cloudApp=cloudApp(storage)?1:0;
    state.cloudDisabled=practiceCloudDisabled(state.cloudAccount,state.cloudApp);
    return state;
}
static bool practiceToken(const std::string& token) {
    return token.size()==32&&std::all_of(token.begin(),token.end(),[](char c) { return (c>='0'&&c<='9')||(c>='a'&&c<='f'); });
}
static void practiceNoLinks(std::filesystem::path path) {
    for(;!path.empty();path=path.parent_path()) {
        auto attrs=GetFileAttributesW(path.c_str());
        if(attrs!=INVALID_FILE_ATTRIBUTES&&(attrs&FILE_ATTRIBUTE_REPARSE_POINT)) throw std::runtime_error("Linked save paths are unsupported.");
        if(path==path.parent_path()) break;
    }
}
static bool practiceRemote(const std::filesystem::path& remote,uint64_t account) {
    constexpr uint64_t individual=76561197960265728ULL;
    if(account<=individual||account>individual+UINT32_MAX||!remote.is_absolute()||remote.lexically_normal()!=remote) return false;
    auto app=remote.parent_path(),user=app.parent_path();
    return remote.filename()==L"remote"&&app.filename()==L"782330"&&user.filename()==std::to_wstring(account-individual)&&user.parent_path().filename()==L"userdata";
}
struct PracticeCheckpointBusy : std::runtime_error { using std::runtime_error::runtime_error; };
static std::array<std::vector<unsigned char>,4> readPracticeFiles(const std::filesystem::path& slot) {
    static const wchar_t* names[]{L"game.details",L"game.details-BACKUP",L"game_duration.dat",L"game_duration.dat-BACKUP"};
    practiceNoLinks(slot); std::vector<HANDLE> handles;
    struct Close { std::vector<HANDLE>& handles; ~Close() { for(auto handle:handles) CloseHandle(handle); } } close{handles};
    HANDLE directory=CreateFileW(slot.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    if(directory==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot lock save directory for reading."); handles.push_back(directory);
    size_t count=0;
    for(const auto& file:std::filesystem::directory_iterator(slot)) {
        practiceNoLinks(file.path());
        if(!file.is_regular_file()||std::find_if(std::begin(names),std::end(names),[&](auto name) { return file.path().filename()==name; })==std::end(names))
            throw std::runtime_error("Unexpected save files; this first experiment supports the four standard containers only.");
        ++count;
    }
    if(count!=4) throw std::runtime_error("All four checkpoint containers are required.");
    std::array<std::vector<unsigned char>,4> result; uint64_t total=0;
    // Open ALL containers before reading any; a concurrent writer is denied.
    for(const auto name:names) {
        HANDLE handle=CreateFileW((slot/name).c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(handle==INVALID_HANDLE_VALUE) {
            auto error=GetLastError();
            if(error==ERROR_SHARING_VIOLATION||error==ERROR_LOCK_VIOLATION) throw PracticeCheckpointBusy("Waiting for autosave files to close.");
            throw std::runtime_error("Cannot open checkpoint files for reading.");
        } handles.push_back(handle);
    }
    for(size_t i=0;i<4;++i) {
        LARGE_INTEGER length{};
        if(!GetFileSizeEx(handles[i+1],&length)||length.QuadPart<28||length.QuadPart>64000000||(total+=length.QuadPart)>128000000)
            throw std::runtime_error("Unsupported checkpoint container size.");
        auto& bytes=result[i]; bytes.resize(static_cast<size_t>(length.QuadPart)); DWORD got=0;
        if(!ReadFile(handles[i+1],bytes.data(),static_cast<DWORD>(bytes.size()),&got,nullptr)||got!=bytes.size()) throw std::runtime_error("Checkpoint read failed.");
    }
    return result;
}
#include "practice_transaction.h"
static bool practiceLoaderMatches(const WallMemory& memory,uintptr_t base,const PracticeProfile* adapter=&practiceProfile6663) {
    if(!adapter)return false;
    for(size_t i=0;i<adapter->methods.size();++i) { std::array<unsigned char,16> bytes{}; if(!memory.read(base+adapter->methods[i],bytes.data(),bytes.size())||bytes!=adapter->signatures[i]) return false; }
    return true;
}
// Mirrors the audited RequestRestartMission parameter construction, with b9=0
// to load PRIMARY checkpoint files. No patching of game code or save contents.
static bool requestPracticeReload(const WallMemory& memory,uintptr_t base,const PracticeRuntime& state) {
    auto adapter=state.adapter; if(!practiceLoaderMatches(memory,base,adapter)) return false;
    alignas(16) std::array<unsigned char,192> parms{};
    using StringMethod=void(__fastcall*)(void*); using Assign=void(__fastcall*)(void*,const void*);
    auto construct=reinterpret_cast<StringMethod>(base+adapter->methods[1]),destroy=reinterpret_cast<StringMethod>(base+adapter->methods[4]);
    auto clear=reinterpret_cast<StringMethod>(base+adapter->methods[3]); auto assign=reinterpret_cast<Assign>(base+adapter->methods[2]);
    for(size_t i=0;i<3;++i) construct(parms.data()+i*48);
    assign(parms.data(),reinterpret_cast<void*>(state.record+8)); clear(parms.data()+48);
    bool valid=memory.read(state.map+0x196c,parms.data()+0x90,8)&&memory.read(state.map+adapter->mapTypeOffset,parms.data()+0x98,4)&&(!adapter->mapExtraOffset||memory.read(state.map+adapter->mapExtraOffset,parms.data()+0xa0,8));
    if(valid) reinterpret_cast<void(__fastcall*)(void*,const void*)>(base+adapter->methods[0])(reinterpret_cast<void*>(state.system),parms.data());
    for(int i=2;i>=0;--i) destroy(parms.data()+size_t(i)*48);
    return valid;
}
static bool practiceReloadCompleted(bool& sawTransition,uintptr_t originalMap,bool readable,uint32_t state,uintptr_t map,bool requested,bool validCampaignLevel) {
    // A queued request alone does not prove that the previous map unloaded.
    if(readable&&(state!=2||map!=originalMap)) sawTransition=true;
    return sawTransition&&readable&&state==2&&!requested&&validCampaignLevel;
}
struct PracticeReloadReadiness {
    bool sawGameplay=false;
    ULONGLONG readySince=0;
    bool update(bool transitioned,bool gameplay,bool verifiedSession,ULONGLONG now) {
        if(!transitioned) { sawGameplay=false; readySince=0; return false; }
        if(gameplay) sawGameplay=true;
        if(!sawGameplay||!verifiedSession) { readySince=0; return false; }
        if(!readySince) readySince=now;
        return now-readySince>=2000;
    }
};
class PracticeLiveBridge {
    std::mutex work;
    std::string seen,token,operation,account,slot,build,checkpoint,cloudDetail,recoveryDetail; std::filesystem::path remote;
    uint64_t requested=0; ULONGLONG nextPoll=0,presentPoll=0,loadingSince=0; uintptr_t originalMap=0;
    PracticeIdleWindow idle;
    bool loading=false,sawTransition=false;
    PracticeReloadReadiness readiness;
    void result(const std::filesystem::path& directory,const std::string& phase,const std::string& detail,const std::string& level="") {
        auto path=directory/(L"practice-live-"+std::to_wstring(GetCurrentProcessId())+L".tsv"),temporary=path; temporary+=L".tmp";
        std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
        out<<"#LABPLUS_LIVE_PRACTICE_V2\t"<<GetCurrentProcessId()<<'\t'<<token<<'\t'<<encounterEpochMillis()<<'\t'<<phase<<'\t'<<account<<'\t'<<slot<<'\t'<<practiceClean(level)<<'\t'<<practiceClean(checkpoint)<<'\t'<<practiceClean(detail+(cloudDetail.empty()?"":" "+cloudDetail)+recoveryDetail)<<'\t'<<build<<'\n';
        out.close(); if(out) MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING);
    }
public:
    void update(const GameProfile* profile,bool engineFrame=true) {
        // Never block presentation waiting for an engine callback (or vice
        // versa). Separate clocks prevent render polling from starving a load.
        std::unique_lock<std::mutex> lock(work,std::try_to_lock); if(!lock.owns_lock()) return;
        auto& poll=engineFrame?nextPoll:presentPoll;
        if(!labAppAvailable()||GetTickCount64()<poll) return; poll=GetTickCount64()+(loading?16:100);
        const auto directory=researchDirectory(); if(directory.empty()) return;
        auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); WallMemory memory;
        struct SteamThreadMemory {
            bool release;
            ~SteamThreadMemory() {
                if(!release) return;
                auto module=GetModuleHandleW(L"steam_api64.dll");
                // Steamworks recommends releasing API parameter storage on
                // threads which do not dispatch SteamAPI_RunCallbacks.
                auto clear=module?reinterpret_cast<void(__cdecl*)()>(GetProcAddress(module,"SteamAPI_ReleaseCurrentThreadMemory")):nullptr;
                if(clear) clear();
            }
        } steamMemory{false};
        try {
            if(loading) {
                if(!engineFrame) return;
                uintptr_t system=0,map=0; uint32_t state=0; unsigned char change=0;
                bool readable=profile&&memory.get(base+profile->gameSystemRva,system)&&memory.get(system+0x44,state)&&memory.get(system+0x50,map)&&memory.get(system+0xb8,change);
                auto level=profile?readLevelIdentity(memory,base,*profile):LevelIdentity{};
                bool transitioned=practiceReloadCompleted(sawTransition,originalMap,readable,state,map,change!=0,level&&practiceSlot(slot)&&practiceMapMatches(level.name,*practiceSlot(slot)));
                bool gameplay=transitioned&&gameplayVisible(memory,base,*profile);
                auto paused=transitioned&&!gameplay?readPracticeRuntime(memory,base,profile):PracticeRuntime{};
                bool verified=gameplay||(paused&&paused.map==level.map&&paused.slot==slot);
                if(readiness.update(transitioned,gameplay,verified,GetTickCount64())) {
                    auto layout=std::atomic_load(&checkpointMetadata);
                    auto loaded=layout?readLoadedCheckpoint(memory,level.map,level.name,*layout):CheckpointObservation{};
                    checkpoint=loaded.name; result(directory,"reloaded",operation=="restore"?"Archive installed through Steam storage; engine map transition and active-player stability checks completed.":"Engine map transition and active-player stability checks completed. No save files were replaced.",level.name); loading=false; token.clear();
                } else if(GetTickCount64()-loadingSince>300000) {
                    result(directory,"error",operation=="restore"?"Archive files were installed but engine completion was not observed. Use retained Original if recovery is needed.":"Engine reload was requested but completion was not observed."); loading=false; token.clear();
                }
                return;
            }
            auto path=directory/(L"practice-live-"+std::to_wstring(GetCurrentProcessId())+L".request");
            if(token.empty()) {
                std::error_code error; auto length=std::filesystem::file_size(path,error); if(error||length>4096) return;
                std::ifstream in(path,std::ios::binary); std::string line; if(!std::getline(in,line)) return;
                std::vector<std::string> values; std::istringstream split(line); std::string part;
                while(std::getline(split,part,'\t')) values.push_back(part);
                if(values.size()!=9||values[0]!="#LABPLUS_LIVE_PRACTICE_REQUEST_V2"||values[1]!=std::to_string(GetCurrentProcessId())||
                   !practiceToken(values[2])||values[2]==seen||(values[4]!="status"&&values[4]!="prepare"&&values[4]!="archive"&&values[4]!="reload"&&values[4]!="restore")||
                   (values[4]=="status"?values[6]!="*":!practiceSlot(values[6]))) return;
                auto number=[](const std::string& value) { return !value.empty()&&value.size()<=20&&std::all_of(value.begin(),value.end(),[](char c) { return c>='0'&&c<='9'; }); };
                if(!number(values[3])||!number(values[5])) return;
                requested=std::stoull(values[3]); auto now=encounterEpochMillis();
                if(requested>now+2000||now>requested+10000) { seen=values[2]; return; }
                token=seen=values[2]; operation=values[4]; account=values[5]; slot=values[6]; build=practiceProfile(profile)?practiceProfile(profile)->version:"unsupported";
                if(values[8]!="*"&&values[8]!=build) { result(directory,"error","Archive patch does not match the running game. Original untouched."); token.clear(); return; } checkpoint.clear(); cloudDetail.clear(); recoveryDetail.clear(); idle.reset(); originalMap=0;
                remote=std::filesystem::u8path(values[7]);
            }
            if(encounterEpochMillis()>requested+30000) { result(directory,"error","Checkpoint writing did not become idle within thirty seconds. No files installed."); token.clear(); return; }
            if(!engineFrame&&!practiceMayRunFromPresent(operation)) {
                result(directory,"waiting","Continue into the loaded level in the game to start the archive load."); return;
            }
            auto state=readPracticeRuntime(memory,base,profile,operation!="status",operation=="archive"||operation=="status");
            if(!state) {
                idle.reset();
                if(state.busy&&operation!="status") { result(directory,"waiting",state.error); return; }
                result(directory,"error",state.error); token.clear(); return;
            }
            if(operation=="status") slot=state.slot;
            else if(!practiceRequestMatches(state,slot)) { result(directory,"error","Requested slot is not the currently loaded campaign slot."); token.clear(); return; }
            steamMemory.release=!engineFrame;
            auto steam=readPracticeSteam();
            cloudDetail="Steam Cloud flags: account="+std::to_string(steam.cloudAccount)+", app="+std::to_string(steam.cloudApp)+".";
            if(!steam.account||std::to_string(steam.account)!=account||!practiceRemote(remote,steam.account)) {
                result(directory,"error","Steam runtime account and save-folder owner must match. Account mixing is blocked."); token.clear(); return;
            }
            if(operation=="status") {
                auto layout=std::atomic_load(&checkpointMetadata);
                auto loaded=layout?readLoadedCheckpoint(memory,state.map,state.level,*layout):CheckpointObservation{};
                checkpoint=loaded.name; result(directory,"ready","Current campaign slot identified. No save files read or changed.",state.level); token.clear(); return;
            }
            if(!steam.cloudDisabled) {
                result(directory,"error",steam.cloudAccount<0||steam.cloudApp<0?"Steam Cloud state is unavailable; live operation blocked.":"Steam Cloud is enabled both for the account and the game. Disable Cloud before the live operation.");
                token.clear(); return;
            }
            if(originalMap&&originalMap!=state.map) { result(directory,"error","Map changed during preparation. Retry in the loaded campaign."); token.clear(); return; }
            originalMap=state.map;
            if(!idle.update(state,GetTickCount64())) { result(directory,"waiting","Waiting for a stable completed checkpoint.",state.level); return; }
            if(operation=="prepare") { result(directory,"ready","Campaign save queues are idle. No save files read or changed.",state.level); token.clear(); return; }
            auto files=readPracticeFiles(remote/std::filesystem::u8path(slot));
            auto after=readPracticeRuntime(memory,base,profile,true,operation=="archive");
            if(!after||after.engineState!=state.engineState||after.map!=state.map||after.record!=state.record||after.user!=state.user||after.slot!=state.slot) throw std::runtime_error("Engine state changed during checkpoint read.");
            auto layout=std::atomic_load(&checkpointMetadata);
            auto loaded=layout?readLoadedCheckpoint(memory,state.map,state.level,*layout):CheckpointObservation{}; checkpoint=loaded.name;
            if(operation=="archive") {
                practiceAuthenticate(files,account);
                if(practiceStorageApi(slot).read()!=files) throw std::runtime_error("Steam storage cache and selected disk checkpoint differ. No archive created.");
                auto output=directory/L"live-practice"/std::to_wstring(GetCurrentProcessId())/token/L"slot";
                practiceNoLinks(output); if(std::filesystem::exists(output)) throw std::runtime_error("Capture destination already exists.");
                std::filesystem::create_directories(output);
                const wchar_t* names[]{L"game.details",L"game.details-BACKUP",L"game_duration.dat",L"game_duration.dat-BACKUP"};
                for(size_t i=0;i<4;++i) { std::ofstream file(output/names[i],std::ios::binary); file.write(reinterpret_cast<const char*>(files[i].data()),files[i].size()); file.close(); if(!file) throw std::runtime_error("Checkpoint copy failed."); }
                result(directory,"archived","Completed checkpoint copied after native authentication and disk/Steam consistency checks. Companion must verify the copy.",state.level); token.clear();
            } else {
                if(operation=="restore") {
                    if(!practiceLoaderMatches(memory,base,state.adapter)) throw std::runtime_error("Audited engine loader signatures did not match; original slot untouched.");
                    auto bundle=directory/L"live-practice"/L"restore-pending"/std::to_wstring(GetCurrentProcessId())/token;
                    auto plan=readPracticeRestorePlan(bundle,account,token,GetCurrentProcessId(),slot,build);
                    auto incoming=readPracticeFiles(bundle/L"incoming");
                    auto stillIdle=[&] {
                        auto check=readPracticeRuntime(memory,base,profile); auto owner=readPracticeSteam();
                        return check&&check.map==state.map&&check.record==state.record&&check.user==state.user&&check.slot==slot&&owner.account==steam.account&&owner.cloudDisabled;
                    };
                    commitPracticeRestore(remote,bundle,incoming,files,plan,account,token,practiceStorageApi(slot),stillIdle,[&](const char* step) {
                        if(strcmp(step,"backed_up")==0) recoveryDetail=" Original retained: "+(remote.parent_path()/(L".labplus-live-original-"+std::wstring(token.begin(),token.end()))).u8string()+". Recovery journal: "+(bundle/L"RESTORE_PATHS.txt").u8string()+".";
                    });
                    if(!stillIdle()) throw std::runtime_error("Files installed, but engine/account/Cloud changed before reload. Original retained; do not continue until reviewed.");
                }
                if(!requestPracticeReload(memory,base,state)) throw std::runtime_error("Audited engine-loader signatures or load parameters did not match.");
                loading=true; loadingSince=GetTickCount64(); sawTransition=false; readiness={}; originalMap=state.map;
                result(directory,"loading",operation=="restore"?"Archive files installed and verified through Steam storage. Loading PRIMARY checkpoint; continue in the game when loading reaches 100%.":"Requested PRIMARY checkpoint through the engine. No save files replaced. Continue in the game when loading reaches 100%.",state.level);
                // Keep the request token until asynchronous completion is reported.
            }
        } catch(const PracticeCheckpointBusy& error) {
            // Only initial checkpoint reads may wait. Once originals exist, a
            // restore error must retain its recovery report and stop.
            if(!token.empty()&&recoveryDetail.empty()) { idle.reset(); result(directory,"waiting",error.what()); }
            else { if(!token.empty()) result(directory,"error",error.what()); token.clear(); loading=false; }
        } catch(const std::exception& error) { if(!token.empty()) result(directory,"error",error.what()); token.clear(); loading=false; }
    }
};
static PracticeLiveBridge practiceLiveBridge;
