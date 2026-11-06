#pragma once
#include <bcrypt.h>
#pragma comment(lib,"bcrypt.lib")
static void printActiveEncounters(const MeathookHudApi& api);

struct LabCommandDefinition { const char* name; const char* usage; bool legacy=false; };
static constexpr LabCommandDefinition labCommandDefinitions[]{
    {"labplus_commands_list","List LAB Plus cvars and action commands"},
    {"labplus_encounters_list","List active encounter managers with reference names"},
    {"labplus_debughud_scale","[0.5..3] - query or set DebugHUD text scale"},
    {"labplus_load_last_save","Load the last LAB Plus archive for the current account, patch and campaign"},
    {"labplus_enemies","0|1 - living enemies and health HUD"},
    {"labplus_enemy_hp_bars","0|1 - world enemy health bars"},
    {"labplus_position","0|1 - player coordinates in debug HUD"},
    {"labplus_angles","0|1 - player angles in debug HUD"},
    {"labplus_timers","0|1 - engine countdown HUD"},
    {"labplus_techs","0|1 - speedrun technique states HUD"},
    {"labplus_recording_start","Start/resume research recording"}, {"labplus_recording_stop","Stop research recording"},
    {"labplus_recording_clear","Start a new research session"}, {"labplus_markers_add","<text> - add a research marker"},
    {"labplus_trigger_set","<class> 0|1 - one exact trigger class"},
    {"labplus_help","Legacy alias",true}, {"labplus_encounters","Legacy list / 0|1 HUD alias",true},
    {"labplus_viewer","Legacy alias",true}, {"labplus_triggers","Legacy alias",true},
    {"labplus_collision","Legacy alias",true}, {"labplus_walls","Legacy alias",true},
    {"labplus_inspector","Legacy alias",true}, {"labplus_disabled","Legacy alias",true},
    {"labplus_history","Legacy alias",true}, {"labplus_fill","Legacy alias",true},
    {"labplus_distance","Legacy alias",true}, {"labplus_trigger","Legacy alias",true},
    {"labplus_path","Legacy alias",true}, {"labplus_record","Legacy alias",true},
    {"labplus_player","Player collider alias",true}, {"labplus_attempts","Attempt overlay alias",true},
    {"labplus_surface","Support plane HUD alias",true},
    {"labplus_clear","Legacy alias",true}, {"labplus_mark","Legacy alias",true}
};
static std::vector<std::string> normalizeLabArguments(const std::vector<std::string>& args) {
    if(args.empty()) return {};
    const auto& name=args[0];
    for(const auto& alias:{std::pair<const char*,const char*>{"labplus_commands_list","labplus_help"},
        {"labplus_encounters_list","labplus_encounters"},{"labplus_recording_clear","labplus_clear"}})
        if(name==alias.first) return args.size()==1?std::vector<std::string>{alias.second}:std::vector<std::string>{};
    if(name=="labplus_recording_start"||name=="labplus_recording_stop") return args.size()==1?
        std::vector<std::string>{"labplus_record",name=="labplus_recording_start"?"1":"0"}:std::vector<std::string>{};
    if(name=="labplus_markers_add"||name=="labplus_trigger_set") {
        auto result=args; result[0]=name=="labplus_markers_add"?"labplus_mark":"labplus_trigger"; return result;
    }
    return args;
}
static bool validLabArguments(const std::vector<std::string>& input) {
    auto args=normalizeLabArguments(input);
    if(args.empty()) return false;
    const std::string& command=args[0];
    if(command=="labplus_debughud_scale") {
        float value=1; return args.size()==1||(args.size()==2&&parseDebugHudScale(args[1],value));
    }
    if(command=="labplus_help"||command=="labplus_clear"||command=="labplus_load_last_save") return args.size()==1;
    if(command=="labplus_encounters") return args.size()==1||(args.size()==2&&(args[1]=="0"||args[1]=="1"));
    if(command=="labplus_mark") return args.size()>1;
    if(command=="labplus_trigger") return args.size()==3&&validTriggerClassName(args[1])&&(args[2]=="0"||args[2]=="1");
    if(args.size()!=2) return false;
    if(command=="labplus_distance") {
        if(args[1].empty()||args[1].size()>3||!std::all_of(args[1].begin(),args[1].end(),[](char c) { return c>='0'&&c<='9'; })) return false;
        int value=atoi(args[1].c_str()); return value>=20&&value<=200;
    }
    if(args[1]!="0"&&args[1]!="1") return false;
    if(command=="labplus_position"||command=="labplus_angles"||command=="labplus_enemies"||command=="labplus_enemy_hp_bars"||command=="labplus_timers"||command=="labplus_techs") return true;
    for(const auto& definition:labCommandDefinitions) if(definition.legacy&&command==definition.name) return true;
    return false;
}
static bool printDebugHudScaleQuery(const MeathookHudApi& api,const std::vector<std::string>& args) {
    if(args.size()!=1||args[0]!="labplus_debughud_scale") return false;
    std::ostringstream text; text.imbue(std::locale::classic());
    text<<"labplus_debughud_scale is "<<debugHudScale.load()<<" (0.5..3)\n";
    api.consolePrint("%s",text.str().c_str()); return true;
}
static bool sendLabRequest(const std::vector<std::string>& args) {
    if(!labAppAvailable()) return false;
    std::string request=std::to_string(GetCurrentProcessId());
    for(const auto& arg:args) { request+='\t'; request+=arg; }
    request+='\n'; if(request.size()>2000) return false;
    HANDLE pipe=CreateFileW(L"\\\\.\\pipe\\labplus.commands",GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
    if(pipe==INVALID_HANDLE_VALUE) return false;
    DWORD written=0; bool success=WriteFile(pipe,request.data(),static_cast<DWORD>(request.size()),&written,nullptr)&&written==request.size();
    CloseHandle(pipe); return success;
}
static bool saveCommandTokenValid(const std::string& token) {
    return token.size()==32&&std::all_of(token.begin(),token.end(),[](char c) { return (c>='0'&&c<='9')||(c>='a'&&c<='f'); });
}
static std::string newSaveCommandToken() {
    std::array<unsigned char,16> bytes{};
    if(BCryptGenRandom(nullptr,bytes.data(),static_cast<ULONG>(bytes.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0) return {};
    std::string token; const char hex[]="0123456789abcdef";
    for(auto byte:bytes) { token+=hex[byte>>4]; token+=hex[byte&15]; } return token;
}
static std::string readSaveCommandReply(const std::filesystem::path& path,DWORD process,const std::string& token) {
    if(!saveCommandTokenValid(token)) return {};
    std::error_code error; auto length=std::filesystem::file_size(path,error); if(error||length>4096) return {};
    std::ifstream file(path,std::ios::binary); std::string line,extra; if(!std::getline(file,line)||std::getline(file,extra)) return {};
    std::istringstream stream(line); std::vector<std::string> fields; std::string part;
    while(std::getline(stream,part,'\t')) fields.push_back(part);
    if(fields.size()!=5||fields[0]!="#LABPLUS_SAVE_COMMAND_V1"||fields[1]!=std::to_string(process)||fields[2]!=token||
       (fields[3]!="ok"&&fields[3]!="error")||fields[4].empty()||fields[4].size()>3072) return {};
    for(auto c:fields[4]) if(static_cast<unsigned char>(c)<32||c==127) return {};
    return fields[4];
}

struct LabCommandSystem { uintptr_t object=0,add=0,exists=0,slot=0; explicit operator bool() const { return object&&add&&exists; } };
// Recognize the public meathook v7.2 ExecuteCommandText thunk, then validate
// the engine object with MSVC RTTI and executable-page checks. Unknown API
// implementations withhold registration instead of guessing addresses.
static LabCommandSystem resolveLabCommandSystem(const WallMemory& memory,uintptr_t execute,uintptr_t base,const GameProfile& profile) {
    unsigned char code[23]{};
    if(!memory.read(execute,code,sizeof(code))) return {};
    const unsigned char prefix[]{0x48,0x89,0xca,0x48,0x8b,0x05};
    const unsigned char suffix[]{0x48,0x8b,0x08,0x48,0x8b,0x01,0x48,0x8b,0x40,0x40,0x48,0xff,0xe0};
    if(memcmp(code,prefix,sizeof(prefix))||memcmp(code+10,suffix,sizeof(suffix))) return {};
    int32_t displacement=0; memcpy(&displacement,code+6,4);
    uintptr_t slot=0,object=0,vtable=0,locator=0; uint32_t rtti[6]{};
    auto inImage=[&](uintptr_t address,size_t size=8) { return address>=base&&address-base<profile.imageSize&&size<=profile.imageSize-(address-base); };
    if(!memory.get(execute+10+displacement,slot)||!inImage(slot)||!memory.get(slot,object)||!memory.get(object,vtable)||
       !inImage(vtable,72)||!memory.get(vtable-8,locator)||!inImage(locator,24)||!memory.read(locator,rtti,sizeof(rtti))||
       rtti[0]!=1||locator-base!=rtti[5]||!inImage(base+rtti[3],64)||
       readEntityText(memory,base+rtti[3]+16,64)!=".?AVidCmdSystemLocal@@") return {};
    LabCommandSystem result; result.object=object; result.slot=slot; uintptr_t executeMethod=0;
    if(!memory.get(vtable+24,result.add)||!memory.get(vtable+40,result.exists)||!memory.get(vtable+64,executeMethod)) return {};
    for(uintptr_t method:{result.add,result.exists,executeMethod}) {
        MEMORY_BASIC_INFORMATION info{};
        SIZE_T inspected=memory.remote?VirtualQueryEx(memory.remote,reinterpret_cast<void*>(method),&info,sizeof(info)):VirtualQuery(reinterpret_cast<void*>(method),&info,sizeof(info));
        if(!inImage(method)||!inspected||info.State!=MEM_COMMIT||
           !(info.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))||info.Protect&PAGE_GUARD) return {};
    }
    return result;
}
#include "labplus_cvars.h"

class LabConsole {
    const MeathookHudApi* api=nullptr;
    bool callbackRegistered=false,commandsRegistered=false,registrationBlocked=false,reportedUnavailable=false;
    ULONGLONG retryAt=0;
    ULONGLONG cvarsRetryAt=0;
    LabCommandSystem commandSystem;
    LabCvarBridge cvars;
    std::string saveToken;
    ULONGLONG saveStarted=0,saveReplyPoll=0;
    static LabConsole*& instance() { static LabConsole* value=nullptr; return value; }
    static void callback(void* self) { static_cast<LabConsole*>(self)->frame(); }
    static void command(void* raw) {
        auto* self=instance(); if(!self||!self->api) return;
        if(!labAppAvailable()) { self->api->consolePrint("%s","labplus_: LAB Plus is closed; commands are inactive.\n"); return; }
        WallMemory memory; int count=0;
        if(!memory.get(reinterpret_cast<uintptr_t>(raw),count)||count<1||count>128) return;
        std::vector<std::string> args;
        for(int i=0;i<count;++i) {
            uintptr_t text=0;
            if(!memory.get(reinterpret_cast<uintptr_t>(raw)+8+size_t(i)*8,text)) return;
            auto value=readEntityText(memory,text,256);
            if(value.find('\t')!=std::string::npos||value.find('\n')!=std::string::npos) return;
            args.push_back(std::move(value));
        }
        if(!validLabArguments(args)) { self->api->consolePrint("%s","labplus_: invalid arguments. Use labplus_commands_list.\n"); return; }
        args=normalizeLabArguments(args);
        if(args[0]=="labplus_load_last_save") {
            if(!self->saveToken.empty()) { self->api->consolePrint("%s","labplus_load_last_save: a save operation is already pending.\n"); return; }
            auto token=newSaveCommandToken();
            if(token.empty()||!sendLabRequest({args[0],token})) { self->api->consolePrint("%s","labplus_load_last_save: LAB Plus is unavailable or busy; retry.\n"); return; }
            self->saveToken=token; self->saveStarted=GetTickCount64(); self->saveReplyPoll=0;
            self->api->consolePrint("%s","labplus_load_last_save: request sent to LAB Plus.\n");
        }
        else if(printDebugHudScaleQuery(*self->api,args)) {}
        else if(args[0]=="labplus_encounters"&&args.size()==1) { printActiveEncounters(*self->api); }
        else if(args[0]=="labplus_help") {
            for(const auto& definition:labCvarDefinitions) self->api->consolePrint("%s : %d..%d\n",definition.name,definition.minimum,definition.maximum);
            for(const auto& definition:labCommandDefinitions) if(!definition.legacy) self->api->consolePrint("%s : %s\n",definition.name,definition.usage);
        } else self->api->consolePrint("%s",sendLabRequest(args)?"labplus_: request sent to LAB Plus.\n":"labplus_: LAB Plus is unavailable or busy; retry.\n");
    }
    void frame() {
        bool available=labAppAvailable();
        if(!available) saveToken.clear();
        if(!saveToken.empty()&&GetTickCount64()>=saveReplyPoll) {
            saveReplyPoll=GetTickCount64()+100;
            auto directory=researchDirectory();
            auto reply=readSaveCommandReply(directory/(L"practice-command-"+std::to_wstring(GetCurrentProcessId())+L".tsv"),GetCurrentProcessId(),saveToken);
            if(!reply.empty()) { api->consolePrint("labplus_load_last_save: %s\n",reply.c_str()); saveToken.clear(); }
            else if(GetTickCount64()-saveStarted>390000) { api->consolePrint("%s","labplus_load_last_save: completion was not confirmed. Check Saves in LAB Plus.\n"); saveToken.clear(); }
        }
        const auto* profile=currentGameProfile(); if(!profile) return;
        if(commandsRegistered) {
            if(available&&!cvars.ready()&&!cvars.blocked()&&GetTickCount64()>=cvarsRetryAt) {
                cvarsRetryAt=GetTickCount64()+2000;
                cvars.initialize(*api,commandSystem,reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),*profile);
            }
            cvars.poll(*api,available); return;
        }
        if(registrationBlocked||!available) return;
        auto system=resolveLabCommandSystem(WallMemory{},reinterpret_cast<uintptr_t>(api->executeCommand),reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),*profile);
        if(!system) {
            if(!reportedUnavailable) { note("labplus_ registration withheld: unrecognized meathook command API or engine command-system RTTI."); reportedUnavailable=true; }
            return;
        }
        using Exists=bool (*)(void*,const char*,bool);
        using Add=void (*)(void*,const char*,void (*)(void*),const char*,uintptr_t,unsigned);
        for(const auto& definition:labCommandDefinitions)
            if(reinterpret_cast<Exists>(system.exists)(reinterpret_cast<void*>(system.object),definition.name,false)) {
                note("labplus_ registration withheld: command name is already owned by another module."); registrationBlocked=true; return;
            }
        instance()=this;
        for(const auto& definition:labCommandDefinitions)
            reinterpret_cast<Add>(system.add)(reinterpret_cast<void*>(system.object),definition.name,command,definition.usage,0,0);
        commandSystem=system;
        commandsRegistered=true; note("labplus_ commands registered on the game thread; execution requires the running LAB Plus companion.");
    }
public:
    void connect() {
        if(callbackRegistered||!labAppAvailable()||GetTickCount64()<retryAt) return;
        retryAt=GetTickCount64()+2000;
        HMODULE module=GetModuleHandleW(L"XINPUT1_3.dll"); if(!module) return;
        auto getter=reinterpret_cast<const MeathookHudApi* (*)()>(GetProcAddress(module,"mh_api_get")); if(!getter) return;
        const auto* candidate=getter(); MeathookHudApi copy{};
        if(!candidate||!WallMemory{}.get(reinterpret_cast<uintptr_t>(candidate),copy)||!copy.executeCommand||!copy.consolePrint||!copy.registerPreFrame) return;
        HMODULE pinned=nullptr;
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(&callback),&pinned)||
           !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(getter),&pinned)) return;
        api=candidate; callbackRegistered=true; api->registerPreFrame(callback,this);
    }
};
static LabConsole& labConsole() { static auto* console=new LabConsole; return *console; }
