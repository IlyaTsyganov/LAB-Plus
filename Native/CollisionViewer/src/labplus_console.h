#pragma once
#include "labplus_cvar_definitions.h"
#include <bcrypt.h>
#pragma comment(lib,"bcrypt.lib")
static void printActiveEncounters(const MeathookHudApi& api);

struct LabCommandDefinition { const char* name; const char* usage; bool legacy=false; };
static constexpr LabCommandDefinition labCommandDefinitions[]{
    {"labplus_commands_list","List LAB Plus cvars and action commands"},
    {"labplus_encounters_list","List active encounter managers with reference names"},
    {"labplus_debughud_scale","[0.5..3] - query or set DebugHUD text scale"},
    {"labplus_load_last_save","Reload the last successfully loaded LAB Plus archive for the current account, patch and campaign"},
    {"labplus_enemies","0|1 - living enemies and health HUD",true},
    {"labplus_enemy_hp_bars","0|1 - world enemy health bars",true},
    {"labplus_position","0|1 - player coordinates in debug HUD",true},
    {"labplus_angles","0|1 - player angles in debug HUD",true},
    {"labplus_timers","0|1 - engine countdown HUD",true},
    {"labplus_techs","0|1 - speedrun technique states HUD",true},
    {"labplus_recording_start","Start/resume research recording"}, {"labplus_recording_stop","Stop research recording"},
    {"labplus_recording_clear","Start a new research session"}, {"labplus_markers_add","<text> - add a research marker"},
    {"labplus_trigger_set","<class> <0|1> - show or hide one exact trigger class (0=hide, 1=show)"},
    {"labplus_help","List LAB Plus console variables and action commands",true}, {"labplus_encounters","List active encounters; 0|1 controls their HUD",true},
    {"labplus_viewer","Legacy alias",true}, {"labplus_triggers","Legacy alias",true},
    {"labplus_shots","Legacy alias",true},
    {"labplus_collision","Legacy alias",true}, {"labplus_walls","Legacy alias",true},
    {"labplus_inspector","Legacy alias",true}, {"labplus_disabled","Legacy alias",true},
    {"labplus_history","Legacy alias",true}, {"labplus_fill","Legacy alias",true},
    {"labplus_distance","[20..200] - viewer draw distance in metres",true}, {"labplus_trigger","<class> <0|1> - show or hide one exact trigger class (0=hide, 1=show)",true},
    {"labplus_path","Legacy alias",true}, {"labplus_record","Legacy alias",true},
    {"labplus_player","Player collider alias",true}, {"labplus_attempts","Attempt overlay alias",true},
    {"labplus_surface","Support plane HUD alias",true},
    {"labplus_clear","Clear the current research session",true}, {"labplus_mark","<text> - add a research marker",true}
};
static bool labBooleanRequest(const std::string& name) {
    for(const auto& definition:labCvarDefinitions)
        if(definition.maximum==1&&name==definition.request) return true;
    return name=="labplus_position"||name=="labplus_angles"||name=="labplus_enemies"||
        name=="labplus_enemy_hp_bars"||name=="labplus_timers"||name=="labplus_techs"||name=="labplus_record";
}
static bool labBooleanValue(const std::string& value) { return value=="0"||value=="1"||value=="toggle"; }
static std::vector<std::string> normalizeLabArguments(const std::vector<std::string>& args) {
    if(args.empty()) return {};
    const auto& name=args[0];
    if(const auto* definition=findLabCvarDefinition(name)) {
        auto result=args; result[0]=definition->request; return result;
    }
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
    if(!input.empty()) if(const auto* definition=findLabCvarDefinition(input[0])) {
        if(input.size()==1) return true; // Query the saved setting on command-only profiles.
        if(input.size()!=2) return false;
        if(definition->maximum==1) return labBooleanValue(input[1]);
        const auto& text=input[1];
        if(text.empty()||text.size()>3||!std::all_of(text.begin(),text.end(),[](char c) { return c>='0'&&c<='9'; })) return false;
        int value=atoi(text.c_str()); return value>=definition->minimum&&value<=definition->maximum;
    }
    auto args=normalizeLabArguments(input);
    if(args.empty()) return false;
    const std::string& command=args[0];
    if(command=="labplus_debughud_scale") {
        float value=1; return args.size()==1||(args.size()==2&&parseDebugHudScale(args[1],value));
    }
    if(command=="labplus_help"||command=="labplus_clear"||command=="labplus_load_last_save") return args.size()==1;
    if(command=="labplus_encounters") return args.size()==1||(args.size()==2&&labBooleanValue(args[1]));
    if(command=="labplus_mark") return args.size()>1;
    if(command=="labplus_trigger") return args.size()==3&&validTriggerClassName(args[1])&&labBooleanValue(args[2]);
    if(args.size()!=2) return false;
    if(command=="labplus_distance") {
        if(args[1].empty()||args[1].size()>3||!std::all_of(args[1].begin(),args[1].end(),[](char c) { return c>='0'&&c<='9'; })) return false;
        int value=atoi(args[1].c_str()); return value>=20&&value<=200;
    }
    return labBooleanRequest(command)&&labBooleanValue(args[1]);
}
static bool printDebugHudScaleQuery(const MeathookHudApi& api,const std::vector<std::string>& args) {
    if(args.size()!=1||args[0]!="labplus_debughud_scale") return false;
    std::ostringstream text; text.imbue(std::locale::classic());
    text<<"labplus_debughud_scale is "<<debugHudScale.load()<<" (0.5..3)\n";
    api.consolePrint("%s",text.str().c_str()); return true;
}
static bool writeLabRequest(const std::vector<std::string>& args,DWORD* openError) {
    if(openError) *openError=ERROR_SUCCESS;
    if(!labAppAvailable()) return false;
    std::string request=std::to_string(GetCurrentProcessId());
    for(const auto& arg:args) { request+='\t'; request+=arg; }
    request+='\n'; if(request.size()>2000) return false;
    HANDLE pipe=CreateFileW(L"\\\\.\\pipe\\labplus.commands",GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
    if(pipe==INVALID_HANDLE_VALUE) { if(openError) *openError=GetLastError(); return false; }
    DWORD written=0; bool success=WriteFile(pipe,request.data(),static_cast<DWORD>(request.size()),&written,nullptr)&&written==request.size();
    CloseHandle(pipe); return success;
}
static bool sendLabRequest(const std::vector<std::string>& args) { return writeLabRequest(args,nullptr); }
static bool saveCommandTokenValid(const std::string& token) {
    return token.size()==32&&std::all_of(token.begin(),token.end(),[](char c) { return (c>='0'&&c<='9')||(c>='a'&&c<='f'); });
}
static std::string newSaveCommandToken() {
    std::array<unsigned char,16> bytes{};
    if(BCryptGenRandom(nullptr,bytes.data(),static_cast<ULONG>(bytes.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0) return {};
    std::string token; const char hex[]="0123456789abcdef";
    for(auto byte:bytes) { token+=hex[byte>>4]; token+=hex[byte&15]; } return token;
}
struct SaveCommandReply { std::string detail; bool accepted=false; };
static SaveCommandReply readSaveCommandReply(const std::filesystem::path& path,DWORD process,const std::string& token) {
    if(!saveCommandTokenValid(token)) return {};
    std::error_code error; auto length=std::filesystem::file_size(path,error); if(error||length>4096) return {};
    std::ifstream file(path,std::ios::binary); std::string line,extra; if(!std::getline(file,line)||std::getline(file,extra)) return {};
    std::istringstream stream(line); std::vector<std::string> fields; std::string part;
    while(std::getline(stream,part,'\t')) fields.push_back(part);
    if(fields.size()!=5||fields[0]!="#LABPLUS_SAVE_COMMAND_V1"||fields[1]!=std::to_string(process)||fields[2]!=token||
       (fields[3]!="ok"&&fields[3]!="error"&&fields[3]!="accepted")||fields[4].empty()||fields[4].size()>3072) return {};
    for(auto c:fields[4]) if(static_cast<unsigned char>(c)<32||c==127) return {};
    return {fields[4],fields[3]=="accepted"};
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

// Use the same names for engine completion and execution. Unsupported cvar
// constructors never run: those profiles expose ordinary settings commands.
template<class Visit> static void visitLabConsoleCommands(bool settingsAsCommands,Visit visit) {
    for(const auto& definition:labCommandDefinitions) {
        const auto* setting=findLabCvarDefinition(definition.name);
        if(settingsAsCommands||!setting) visit(definition.name,setting?setting->description:definition.usage);
    }
    if(settingsAsCommands) for(const auto& definition:labCvarDefinitions) {
        if(definition.source) continue; // The managed HUD owns its fallback commands.
        bool alreadyListed=false;
        for(const auto& command:labCommandDefinitions) if(!strcmp(command.name,definition.name)) alreadyListed=true;
        if(!alreadyListed) visit(definition.name,definition.description);
    }
}
static bool registerLabConsoleCommands(bool settingsAsCommands,
    const std::function<bool(const char*)>& exists,const std::function<void(const char*,const char*)>& add) {
    bool conflict=false;
    visitLabConsoleCommands(settingsAsCommands,[&](const char* name,const char*) { if(exists(name)) conflict=true; });
    if(conflict) return false;
    visitLabConsoleCommands(settingsAsCommands,add);
    bool registered=true;
    visitLabConsoleCommands(settingsAsCommands,[&](const char* name,const char*) { if(!exists(name)) registered=false; });
    return registered;
}
static void printLabCommandList(const MeathookHudApi& api,bool settingsAvailable,bool nativeCvars=false) {
    if(settingsAvailable) for(const auto& definition:labCvarDefinitions) {
        if(definition.source&&!nativeCvars) continue;
        api.consolePrint("%s : %s [%d..%d]\n",definition.name,definition.description,definition.minimum,definition.maximum);
    }
    else api.consolePrint("%s","labplus_: state settings are not registered; see the LAB Plus log.\n");
    for(const auto& definition:labCommandDefinitions) if(!definition.legacy&&(!nativeCvars||!findLabCvarDefinition(definition.name)))
        api.consolePrint("%s : %s\n",definition.name,definition.usage);
}
static bool printLabCommandUsage(const MeathookHudApi& api,const std::string& name) {
    if(const auto* setting=findLabCvarDefinition(name)) {
        api.consolePrint("%s : %s [%d..%d]\n",setting->name,setting->description,setting->minimum,setting->maximum);
        return true;
    }
    for(const auto& definition:labCommandDefinitions) if(name==definition.name) {
        api.consolePrint("%s : %s\n",definition.name,definition.usage); return true;
    }
    return false;
}
static bool validateLabConsoleInput(const MeathookHudApi& api,const std::vector<std::string>& args) {
    if(validLabArguments(args)) return true;
    // Missing/invalid parameters on known commands are help requests, not
    // executable actions. Keep this before transport or argument normalization.
    if(args.empty()||!printLabCommandUsage(api,args[0]))
        api.consolePrint("%s","labplus_: unknown command. Use labplus_commands_list.\n");
    return false;
}
static bool printLabSettingQuery(const MeathookHudApi& api,const std::vector<std::string>& args,const std::wstring& ini) {
    if(args.size()!=1) return false;
    const auto* definition=findLabCvarDefinition(args[0]); if(!definition) return false;
    api.consolePrint("%s is %d (%d..%d; saved LAB Plus setting)\n%s\n",definition->name,
        savedLabCvarValue(*definition,ini),definition->minimum,definition->maximum,definition->description);
    return true;
}

class LabConsole {
    const MeathookHudApi* api=nullptr;
    bool callbackRegistered=false,commandsRegistered=false,registrationBlocked=false,reportedUnavailable=false;
    ULONGLONG retryAt=0;
    ULONGLONG cvarsRetryAt=0;
    LabCommandSystem commandSystem;
    LabCvarBridge cvars;
    bool settingsAsCommands=false;
    std::mutex saveMutex;
    std::string saveToken;
    ULONGLONG saveStarted=0,saveReplyPoll=0;
    bool saveSent=false,saveAccepted=false;

    static bool retrySaveDelivery(DWORD error) { return error==ERROR_PIPE_BUSY||error==ERROR_FILE_NOT_FOUND; }
    const char* beginSaveCommand() {
        std::lock_guard<std::mutex> lock(saveMutex);
        if(!saveToken.empty()) return "A save operation is already pending.";
        auto token=newSaveCommandToken();
        if(token.empty()) return "Could not create a save request.";
        DWORD error=0;
        const bool sent=writeLabRequest({"labplus_load_last_save",token},&error);
        if(!sent&&!retrySaveDelivery(error)) return "LAB Plus is unavailable. Retry.";
        saveToken=std::move(token); saveStarted=GetTickCount64(); saveReplyPoll=0;
        saveSent=sent; saveAccepted=false;
        return sent?"Request sent to LAB Plus.":"LAB Plus is busy. Retrying delivery.";
    }
    std::string pollSaveCommand(bool available) {
        std::lock_guard<std::mutex> lock(saveMutex);
        if(!available) saveToken.clear();
        const auto now=GetTickCount64();
        if(saveToken.empty()||now<saveReplyPoll) return {};
        saveReplyPoll=now+100;
        if(!saveSent) {
            DWORD error=0;
            if(writeLabRequest({"labplus_load_last_save",saveToken},&error)) {
                saveSent=true; saveStarted=now;
                return "Request sent to LAB Plus.";
            }
            if(retrySaveDelivery(error)&&now-saveStarted<1000) return {};
            saveToken.clear();
            return "Could not send the request to LAB Plus. Retry.";
        }
        auto directory=researchDirectory();
        auto reply=readSaveCommandReply(directory/(L"practice-command-"+std::to_wstring(GetCurrentProcessId())+L".tsv"),GetCurrentProcessId(),saveToken);
        if(reply.accepted) {
            if(!saveAccepted) { saveAccepted=true; return reply.detail; }
        } else if(!reply.detail.empty()) { saveToken.clear(); return reply.detail; }
        if(!saveAccepted&&now-saveStarted>10000) {
            saveToken.clear();
            return "LAB Plus did not acknowledge the request. Retry.";
        }
        if(now-saveStarted>390000) {
            saveToken.clear();
            return "Completion was not confirmed. Check Saves in LAB Plus.";
        }
        return {};
    }
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
        if(!validateLabConsoleInput(*self->api,args)) return;
        if(printLabSettingQuery(*self->api,args,(researchDirectory()/L"viewer.ini").wstring())) return;
        args=normalizeLabArguments(args);
        if(args[0]=="labplus_load_last_save") {
            self->api->consolePrint("labplus_load_last_save: %s\n",self->beginSaveCommand());
        }
        else if(printDebugHudScaleQuery(*self->api,args)) {}
        else if(args[0]=="labplus_encounters"&&args.size()==1) { printActiveEncounters(*self->api); }
        else if(args[0]=="labplus_help") {
            printLabCommandList(*self->api,self->settingsAsCommands||self->cvars.ready(),!self->settingsAsCommands);
        } else self->api->consolePrint("%s",sendLabRequest(args)?"labplus_: request sent to LAB Plus.\n":"labplus_: LAB Plus is unavailable or busy; retry.\n");
    }
    void frame() {
        bool available=labAppAvailable();
        auto saveReply=pollSaveCommand(available);
        if(!saveReply.empty()) api->consolePrint("labplus_load_last_save: %s\n",saveReply.c_str());
        const auto* profile=currentGameProfile(); if(!profile) return;
        if(commandsRegistered) {
            if(!settingsAsCommands&&available&&!cvars.ready()&&!cvars.blocked()&&GetTickCount64()>=cvarsRetryAt) {
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
        const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        auto cvarSystem=resolveLabCvarSystem(WallMemory{},system,base,*profile);
        auto mode=labCvarMode(*profile,cvarSystem,resolveLabCvarFactory(WallMemory{},cvarSystem,base,*profile));
        if(mode==LabCvarMode::Waiting) return;
        settingsAsCommands=mode==LabCvarMode::Commands;
        instance()=this;
        if(!registerLabConsoleCommands(settingsAsCommands,[&](const char* name) {
                return reinterpret_cast<Exists>(system.exists)(reinterpret_cast<void*>(system.object),name,false);
            },[&](const char* name,const char* usage) {
                reinterpret_cast<Add>(system.add)(reinterpret_cast<void*>(system.object),name,command,usage,0,0);
            })) {
            note("labplus_ registration blocked: command name conflict or engine registration failed."); registrationBlocked=true; return;
        }
        if(settingsAsCommands) note("labplus_ state settings registered as commands; native cvar creation ABI is unavailable for this game image.");
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
