#include "../src/vulkan_layer.cpp"
#include <cstdarg>
static void expect(bool ok,const char* text) { if(!ok) { fprintf(stderr,"FAIL: %s\n",text); exit(1); } }
template<class T> static void put(void* memory,size_t offset,T value) { memcpy(static_cast<unsigned char*>(memory)+offset,&value,sizeof(value)); }
static unsigned diagnostics=0,commands=0;
static std::string consoleOutput;
static void print(const char* format,...) {
    ++diagnostics; char text[2048]; va_list args; va_start(args,format);
    vsnprintf(text,sizeof(text),format,args); va_end(args); consoleOutput+=text;
}
static void execute(const char*) { ++commands; }
struct CvarConstructorCall {
    void* object=nullptr; const char* name=nullptr; const char* description=nullptr;
    int flags=0; float minimum=0,maximum=0; const char* const* completions=nullptr; void* callback=nullptr;
    void* registeredSystem=nullptr; void* registeredObject=nullptr;
} constructorCall;
static void captureCvarConstructor(void* object,const char* name,const char*,int flags,const char* description,
    float minimum,float maximum,const char* const* completions,void* callback) {
    constructorCall={object,name,description,flags,minimum,maximum,completions,callback,nullptr,nullptr};
}
static void captureCvarRegistration(void* system,void* object) {
    constructorCall.registeredSystem=system; constructorCall.registeredObject=object;
}
static void checkNativeCvarDescriptionsAndCompletion() {
    LabCvarSystem system{0x1234,1};
    LabCvarFactory factory{reinterpret_cast<uintptr_t>(&captureCvarConstructor),reinterpret_cast<uintptr_t>(&captureCvarRegistration),1};
    auto backend=nativeLabCvarBackend(system,factory);
    for(const auto& definition:nativeLabCvarDefinitions()) {
        OwnedLabCvar owned; sprintf_s(owned.initial,"%d",definition.initial); backend.create(owned,definition);
        expect(constructorCall.object==owned.object&&constructorCall.registeredObject==owned.object&&
            reinterpret_cast<uintptr_t>(constructorCall.registeredSystem)==system.object,"native constructor and registration receive the owned cvar object");
        expect(constructorCall.name==definition.name&&constructorCall.description==definition.description&&strlen(definition.description)>10&&
            !strstr(definition.description,"toggle")&&!strchr(definition.description,';'),"each native cvar receives a useful description without injected commands");
        expect(constructorCall.minimum==definition.minimum&&constructorCall.maximum==definition.maximum&&!constructorCall.callback,"cvar bounds and default engine completion callback ABI are preserved");
        if(definition.maximum==1) {
            auto values=constructorCall.completions;
            expect(values&&!strcmp(values[0],"0")&&!strcmp(values[1],"1")&&!values[2]&&(constructorCall.flags&3)==1,
                "native binary cvar value completion offers exactly 0 and 1, never postfix toggle");
        } else expect(!constructorCall.completions&&(constructorCall.flags&3)==2,"numeric range cvars do not get binary value completion");
    }
    OwnedLabCvar owned; uintptr_t data=reinterpret_cast<uintptr_t>(owned.object)+8; put(owned.object,0,data);
    memset(owned.object+8,0x5a,104); unsigned char before[120]{}; memcpy(before,owned.object,sizeof(before));
    const auto& definition=labCvarDefinitions[0];
    expect(completeOwnedLabCvarMetadata(owned,definition),"retail-stripped metadata is restored in module-owned inline storage");
    for(size_t i=0;i<sizeof(before);++i) if(!(i>=8+56&&i<8+64)&&!(i>=8+80&&i<8+88))
        expect(before[i]==owned.object[i],"metadata restoration leaves values, flags, names, bounds and reset strings untouched");
    uintptr_t description=0,values=0; memcpy(&description,owned.object+8+56,8); memcpy(&values,owned.object+8+80,8);
    expect(description==reinterpret_cast<uintptr_t>(definition.description)&&values==reinterpret_cast<uintptr_t>(labCvarValueCompletions(definition)),"owned description and completion pointers retain stable storage");
    OwnedLabCvar foreign; put(foreign.object,0,data); unsigned char foreignBefore[120]{}; memcpy(foreignBefore,foreign.object,sizeof(foreignBefore));
    expect(!completeOwnedLabCvarMetadata(foreign,definition)&&!memcmp(foreign.object,foreignBefore,sizeof(foreignBefore)),"foreign data pointers are rejected without writing either object");
}
struct EngineFixture {
    std::unordered_map<std::string,uintptr_t> names;
    std::unordered_map<std::string,std::string> values;
    std::unordered_map<std::string,int> defaults;
    OwnedLabCvar debug;
    unsigned creates=0,writes=0;
    bool registerWorks=true,writeWorks=true;
    EngineFixture() {
        const char* name="com_debugHUD"; auto variable=reinterpret_cast<uintptr_t>(debug.object);
        put(debug.object,0,variable+8); put(debug.object,8+40,reinterpret_cast<uintptr_t>(name)); names[name]=variable; change(name,0);
    }
    void change(const char* name,int value,const char* text=nullptr) {
        auto variable=names.at(name); values[name]=text?text:std::to_string(value);
        auto* data=reinterpret_cast<void*>(variable+8);
        put(data,0,reinterpret_cast<uintptr_t>(values[name].c_str())); put(data,8,value);
    }
    int number(const char* name) { int value=-1; std::string text; expect(readLabCvarValue(WallMemory{},names.at(name),name,value,text),"fixture value ABI"); return value; }
    LabCvarBackend backend() {
        return {
            [this](const char* name) { auto found=names.find(name); return found==names.end()?uintptr_t(0):found->second; },
            [this](OwnedLabCvar& storage,const LabCvarDefinition& definition) {
                ++creates; if(!registerWorks) return;
                auto variable=reinterpret_cast<uintptr_t>(storage.object);
                put(storage.object,0,variable+8); put(storage.object,8+40,reinterpret_cast<uintptr_t>(definition.name));
                names[definition.name]=variable; defaults[definition.name]=atoi(storage.initial); change(definition.name,atoi(storage.initial));
            },
            [this](const char* name,const char* value) { ++writes; if(writeWorks) change(name,atoi(value),value); }
        };
    }
};
static void checkSettingsCommands(const MeathookHudApi& api,const std::wstring& ini) {
    std::unordered_set<std::string> registered;
    auto exists=[&](const char* name) { return registered.count(name)!=0; };
    auto add=[&](const char* name,const char* usage) {
        expect(usage&&*usage&&!strstr(usage,"toggle")&&registered.insert(name).second,"engine receives each completion name once with descriptive help and no postfix toggle suggestion");
    };
    expect(registerLabConsoleCommands(true,exists,add),"settings commands register when native cvar creation is unavailable");
    for(const auto& definition:labCvarDefinitions) {
        expect(registered.count(definition.name)==size_t(!definition.source),"main fallback settings register once; the managed HUD retains its own commands");
        expect(validLabArguments({definition.name}),"each setting supports a value query");
        for(int value:{definition.minimum,definition.maximum}) {
            std::vector<std::string> input{definition.name,std::to_string(value)};
            expect(validLabArguments(input)&&normalizeLabArguments(input)==std::vector<std::string>{definition.request,std::to_string(value)}&&
                validLabArguments(normalizeLabArguments(input)),"each advertised setting dispatches to a valid companion request");
        }
        for(const auto& invalid:std::vector<std::vector<std::string>>{
            {definition.name,"-1"},{definition.name,std::to_string(definition.maximum+1)},
            {definition.name,"1;quit"},{definition.name,""},{definition.name,"1","extra"}})
            expect(!validLabArguments(invalid),"fallback rejects invalid values before transport");
    }
    expect(!registered.count("labplus_command_list")&&!validLabArguments({"labplus_command_list"})&&registered.count("labplus_commands_list"),
        "only the plural list command is registered and accepted");
    expect(normalizeLabArguments({"labplus_viewer_show","1"})==std::vector<std::string>{"labplus_viewer","1"}&&
        normalizeLabArguments({"labplus_shots_show","1"})==std::vector<std::string>{"labplus_shots","1"},"viewer and shot toggles use the existing companion handlers");
    consoleOutput.clear(); printLabCommandList(api,true);
    expect(consoleOutput.find("toggle")==std::string::npos&&consoleOutput.find("Show trigger volumes.")!=std::string::npos,
        "command list describes settings instead of appending executable toggle text");
    std::istringstream help(consoleOutput); std::string line;
    while(std::getline(help,line)) expect(registered.count(line.substr(0,line.find(' ')))==1,"every help entry has an engine registration");
    consoleOutput.clear();
    expect(printLabSettingQuery(api,{"labplus_viewer_distance"},ini)&&consoleOutput==
        "\"labplus_viewer_distance\" is:\"75\"^7 default:\"40\"\n^7Maximum viewer draw distance in metres.\n",
        "query uses the exact engine two-line format with distinct current and factory defaults");
    expect(!printLabSettingQuery(api,{"labplus_viewer_show","1"},ini)&&commands==0,"set requests are not swallowed by the query handler");
    registered.clear();
    expect(registerLabConsoleCommands(false,exists,add),"audited cvar backend keeps action commands separate");
    for(const auto& definition:nativeLabCvarDefinitions()) expect(!registered.count(definition.name),"canonical and short native cvars never get duplicate command registrations");
    expect(findLabCvarDefinition("labplus_viewer")&&registered.count("labplus_encounters"),"viewer supports engine toggle while the encounter-list command retains its meaning");
    for(const char* name:{"labplus_enemies","labplus_enemy_hp_bars","labplus_position","labplus_angles","labplus_timers","labplus_techs"}) {
        const auto* alias=findLabCvarDefinition(name); const auto* preferred=findLabCvarDefinition(std::string(name)+"_show");
        expect(alias&&preferred&&alias->maximum==1&&preferred->maximum==1&&!strcmp(alias->request,preferred->request),"new show names and existing short names are synchronized native binary cvars");
        expect(!registered.count(name)&&!registered.count(preferred->name),"neither binary spelling is registered as a shadowing command");
    }
    expect(normalizeLabArguments({"labplus_recording_start"})==std::vector<std::string>{"labplus_record","1"}&&
        normalizeLabArguments({"labplus_recording_stop"})==std::vector<std::string>{"labplus_record","0"}&&
        registered.count("labplus_recording_start")&&registered.count("labplus_recording_stop"),"recording has separate start and stop action commands");
    for(const char* name:{"labplus_speedometer_landing_show","labplus_speedometer_vertical_show","labplus_speedometer_sign_vertical"})
        expect(findLabCvarDefinition(name)&&findLabCvarDefinition(name)->source,"renamed speedometer cvars retain the HUD persistence path");
    for(const char* name:{"labplus_speedometer_showlanding","labplus_speedometer_showvertical","labplus_speedometer_verticalsign"})
        expect(!findLabCvarDefinition(name),"old speedometer spellings are replaced by the requested names");
    consoleOutput.clear(); printLabCommandList(api,true,true);
    expect(consoleOutput.find("toggle")==std::string::npos&&consoleOutput.find("labplus_command_list")==std::string::npos,
        "native command list contains descriptions and excludes the removed alias and toggle examples");
    for(const char* name:{"labplus_enemies","labplus_enemy_hp_bars","labplus_position","labplus_angles","labplus_timers","labplus_techs"})
        expect(consoleOutput.find(std::string(name)+"_show : ")!=std::string::npos&&consoleOutput.find(std::string(name)+" : ")==std::string::npos,
            "command list prefers the new show names rather than their short aliases");
    consoleOutput.clear(); printLabCommandList(api,false,true);
    for(const auto& definition:labCvarDefinitions) expect(consoleOutput.find(std::string(definition.name)+" : ")==std::string::npos,"unregistered native cvars are not advertised during loading or after failure");
    registered.clear(); registered.insert("labplus_viewer_show"); auto before=registered.size();
    expect(!registerLabConsoleCommands(true,exists,add)&&registered.size()==before,"name conflicts prevent all registration before any writes");
    registered.clear();
    expect(!registerLabConsoleCommands(true,exists,[](const char*,const char*) {}),"failed engine registration is detected");
    for(const auto& definition:labCvarDefinitions) if(definition.maximum==1) {
        expect(validLabArguments({definition.name,"toggle"})&&validLabArguments({definition.request,"toggle"}),"binary command fallback and aliases accept toggle");
    }
    expect(!validLabArguments({"labplus_viewer_distance","toggle"})&&!validLabArguments({"labplus_debughud_scale","toggle"})&&
        !validLabArguments({"labplus_toggle","labplus_viewer_show"}),"numeric ranges and the rejected generic wrapper are not toggle commands");
}
static void checkEveryBinaryCvar(const MeathookHudApi& api,const std::wstring& ini) {
    unsigned checked=0;
    std::unordered_set<std::string> names;
    for(const auto& definition:nativeLabCvarDefinitions()) {
        expect(names.insert(definition.name).second,"native cvar names are unique");
        if(definition.minimum!=0||definition.maximum!=1) continue;
        EngineFixture engine; LabCvarBridge bridge; std::vector<std::vector<std::string>> requests;
        expect(bridge.initializeBackend(api,engine.backend(),ini,[&](const auto& request) { requests.push_back(request); return true; }),"binary catalogue registration");
        std::unordered_map<std::string,int> before;
        for(const auto& other:nativeLabCvarDefinitions()) before[other.name]=engine.number(other.name);
        const int initial=before.at(definition.name);
        ULONGLONG now=100;
        for(int value:{1-initial,initial}) {
            const auto requestCount=requests.size();
            engine.change(definition.name,value); bridge.poll(api,true,now); now+=100;
            expect(bridge.ready()&&requests.size()==requestCount+1&&requests.back()==std::vector<std::string>{definition.request,std::to_string(value)},"every binary native name dispatches exactly one numeric companion request");
            for(const auto& other:nativeLabCvarDefinitions()) {
                int expected=!strcmp(other.request,definition.request)?value:before.at(other.name);
                expect(engine.number(other.name)==expected,"binary toggle mirrors only matching aliases and preserves every other setting");
            }
            bridge.poll(api,true,now); now+=100;
            expect(requests.size()==requestCount+1,"no alias echo while waiting for persistence");
            auto source=definition.source?(std::filesystem::path(ini).parent_path()/definition.source).wstring():ini;
            expect(WritePrivateProfileStringW(L"Viewer",definition.setting,std::to_wstring(value).c_str(),source.c_str())!=0,"persist binary main/HUD acknowledgement");
            bridge.poll(api,true,now); now+=100;
            expect(bridge.ready()&&requests.size()==requestCount+1&&engine.number(definition.name)==value,"acknowledgement preserves the toggled cvar without a repeat request");
        }
        for(const auto& other:nativeLabCvarDefinitions()) expect(engine.number(other.name)==before.at(other.name)&&engine.defaults.at(other.name)==other.initial,
            "two toggles restore the complete current catalogue without changing any factory default");
        printf("PASS: binary cvar round trip and alias isolation: %s\n",definition.name); ++checked;
    }
    expect(checked==44,"all 25 binary settings and 19 short aliases are covered");
}
static void checkCommandArgumentHelp(const MeathookHudApi& api) {
    unsigned executableRequests=0;
    auto dispatch=[&](const std::vector<std::string>& args) {
        if(validateLabConsoleInput(api,args)) ++executableRequests;
    };
    for(const auto& args:std::vector<std::vector<std::string>>{
        {"labplus_trigger_set"},{"labplus_trigger_set","idTrigger","2"},
        {"labplus_trigger_set","idTrigger;quit","0"},{"labplus_trigger_set","idTrigger","0","extra"},
        {"labplus_trigger"}}) {
        consoleOutput.clear(); dispatch(args);
        expect(consoleOutput.find(args[0]+" : ")!=std::string::npos&&consoleOutput.find("<class> <0|1>")!=std::string::npos&&
            consoleOutput.find("0=hide, 1=show")!=std::string::npos&&consoleOutput.find("invalid arguments")==std::string::npos,
            "missing/invalid trigger arguments show the exact command description and parameter syntax");
    }
    expect(executableRequests==0,"argument help never reaches executable dispatch");
    for(const auto& definition:labCommandDefinitions) {
        std::vector<std::string> args{definition.name};
        if(validLabArguments(args)) continue;
        consoleOutput.clear(); dispatch(args);
        expect(consoleOutput.find(std::string(definition.name)+" : ")!=std::string::npos,
            "every registered command requiring arguments explains them when called without parameters");
    }
    expect(executableRequests==0,"all missing-argument requests remain read-only");
    consoleOutput.clear(); dispatch({"labplus_markers_add"});
    expect(consoleOutput.find("<text>")!=std::string::npos,"marker commands explain their required text");
    for(const auto& args:std::vector<std::vector<std::string>>{
        {"labplus_trigger_set","idTrigger","0"},{"labplus_trigger_set","idTrigger","1"},
        {"labplus_recording_start"},{"labplus_recording_stop"},{"labplus_commands_list"}}) {
        consoleOutput.clear(); auto before=executableRequests; dispatch(args);
        expect(executableRequests==before+1&&consoleOutput.empty(),"valid parameterized commands and zero-argument actions still execute normally");
    }
    consoleOutput.clear(); dispatch({"labplus_command_list"});
    expect(consoleOutput.find("unknown command")!=std::string::npos,"removed names are not revived by argument help");
}
static void checkTriggerValueQueries(const MeathookHudApi& api,const std::wstring& ini) {
    expect(WritePrivateProfileStringW(L"TriggerClasses",L"idTrigger",L"0",ini.c_str())!=0,"isolated hidden trigger class");
    consoleOutput.clear();
    expect(validLabArguments({"labplus_trigger_set","idTrigger"})&&printLabTriggerQuery(api,{"labplus_trigger_set","idTrigger"},ini)&&consoleOutput==
        "\"labplus_trigger_set idTrigger\" is:\"0\"^7 default:\"1\"\n^7Show or hide this exact trigger class (0=hide, 1=show).\n",
        "class-only trigger query shows the current value and default using the engine format");
    expect(readHiddenTriggerClasses(ini).count("idTrigger")==1,"trigger query does not change visibility");
    expect(WritePrivateProfileStringW(L"TriggerClasses",L"idTrigger",nullptr,ini.c_str())!=0,"remove isolated class override");
    consoleOutput.clear(); expect(printLabTriggerQuery(api,{"labplus_trigger","idTrigger"},ini)&&consoleOutput.find("is:\"1\"^7 default:\"1\"")!=std::string::npos,
        "unconfigured classes report the visible default through the legacy query too");
    expect(!printLabTriggerQuery(api,{"labplus_trigger_set","idTrigger","0"},ini)&&!printLabTriggerQuery(api,{"labplus_trigger_set","idTrigger;quit"},ini),
        "value writes and invalid classes are not swallowed by the query handler");
}
int main() {
    MeathookHudApi api{}; api.consolePrint=print; api.executeCommand=execute;
    checkNativeCvarDescriptionsAndCompletion();
    auto temporary=std::filesystem::temp_directory_path()/(L"labplus-Cvar-Test-"+std::to_wstring(GetCurrentProcessId()));
    std::filesystem::create_directories(temporary); auto ini=(temporary/L"viewer.ini").wstring();
    auto setSaved=[&](const wchar_t* key,const wchar_t* value) { expect(WritePrivateProfileStringW(L"Viewer",key,value,ini.c_str())!=0,"isolated saved setting"); };
    setSaved(L"ShowResearchPath",L"1"); setSaved(L"Distance",L"75");
    std::filesystem::create_directories(temporary/L"hud");
    auto hudIni=(temporary/L"hud"/L"console.ini").wstring();
    expect(WritePrivateProfileStringW(L"Viewer",L"VerticalShowSign",L"0",hudIni.c_str())!=0,"isolated HUD console setting");
    checkSettingsCommands(api,ini);
    checkCommandArgumentHelp(api);
    checkTriggerValueQueries(api,ini);
    checkEveryBinaryCvar(api,ini);
    EngineFixture engine; LabCvarBridge bridge; std::vector<std::vector<std::string>> requests;
    expect(bridge.initializeBackend(api,engine.backend(),ini,[&](const auto& request) { requests.push_back(request); return true; }),"native creation registers all owned variables");
    expect(engine.creates==nativeLabCvarDefinitions().size()&&engine.number("labplus_path_show")==1&&engine.number("labplus_viewer_distance")==75&&
        engine.number("labplus_player_show")==0&&engine.number("labplus_attempts_show")==1&&
        engine.number("labplus_speedometer_sign_vertical")==0&&commands==0,"initialization uses main and HUD saved values and sends no console commands");
    expect(engine.defaults.at("labplus_viewer_distance")==40&&engine.defaults.at("labplus_speedometer_sign_vertical")==1,
        "native reset/default strings retain factory values while saved current values are restored");
    unsigned initializationWrites=engine.writes;
    for(ULONGLONG time=100;time<1000;time+=100) bridge.poll(api,true,time);
    expect(engine.writes==initializationWrites&&requests.empty(),"unchanged settings cause no writes or console spam");
    setSaved(L"ShowResearchPath",L"0"); bridge.poll(api,true,1000);
    expect(engine.number("labplus_path_show")==0&&engine.number("labplus_path")==0&&engine.writes==initializationWrites+2&&requests.empty()&&
        engine.defaults.at("labplus_path_show")==1&&engine.defaults.at("labplus_path")==1,"UI changes update canonical and short current values without changing either default");
    engine.change("labplus_path_show",1); bridge.poll(api,true,1100);
    expect(requests.size()==1&&requests.back()==std::vector<std::string>{"labplus_path","1"},"console cvar changes reach the injected companion transport");
    setSaved(L"ShowResearchPath",L"1"); bridge.poll(api,true,1200); unsigned acknowledged=engine.writes;
    bridge.poll(api,true,4000); expect(engine.writes==acknowledged,"acknowledged changes do not time out or repeat");
    engine.change("labplus_path_show",0); bridge.poll(api,false,4100);
    expect(engine.number("labplus_path_show")==1&&requests.size()==1,"closed companion changes are restored without sending a request");
    engine.change("labplus_path_show",0); bridge.poll(api,true,4200); // reopening restores saved values
    expect(engine.number("labplus_path_show")==1,"reopening restores saved state");
    engine.change("labplus_path_show",0); bridge.poll(api,true,4300); bridge.poll(api,true,6500);
    expect(engine.number("labplus_path_show")==1,"unacknowledged console changes are restored after timeout");
    engine.writeWorks=false; setSaved(L"ShowResearchPath",L"0"); bridge.poll(api,true,6600);
    expect(bridge.blocked()&&!bridge.ready(),"failed native writes block further synchronization");
    unsigned failedWrites=engine.writes,failedDiagnostics=diagnostics;
    for(ULONGLONG time=6700;time<10000;time+=100) bridge.poll(api,true,time);
    expect(engine.writes==failedWrites&&diagnostics==failedDiagnostics&&commands==0,"failed updates are not retried on every poll");

    EngineFixture aliases; LabCvarBridge aliasBridge; std::vector<std::vector<std::string>> aliasRequests;
    expect(aliasBridge.initializeBackend(api,aliases.backend(),ini,[&](const auto& request) { aliasRequests.push_back(request); return true; }),"short names register as native cvars");
    aliases.change("labplus_viewer",1); aliasBridge.poll(api,true,100);
    expect(aliasRequests==std::vector<std::vector<std::string>>{{"labplus_viewer","1"}}&&
        aliases.number("labplus_viewer_show")==1&&aliases.number("labplus_viewer")==1,"toggle through the short name dispatches once and updates the canonical cvar");
    aliasBridge.poll(api,true,200);
    expect(aliasRequests.size()==1,"mirroring does not echo another companion request before acknowledgement");
    aliases.change("labplus_viewer_show",0); aliasBridge.poll(api,true,300);
    expect(aliasRequests.size()==2&&aliasRequests.back()==std::vector<std::string>{"labplus_viewer","0"}&&aliases.number("labplus_viewer")==0,
        "alternating toggle names before acknowledgement reads the mirrored value");
    setSaved(L"Enabled",L"1"); aliasBridge.poll(api,true,400);
    expect(aliases.number("labplus_viewer")==1&&aliases.number("labplus_viewer_show")==1&&aliasRequests.size()==2,"UI changes synchronize both cvar names without requests");
    aliases.change("labplus_viewer",0); aliasBridge.poll(api,true,500); aliasBridge.poll(api,true,2600);
    expect(aliases.number("labplus_viewer")==1&&aliases.number("labplus_viewer_show")==1,"unacknowledged alias changes restore both values");
    setSaved(L"Enabled",L"0");

    EngineFixture rejected; rejected.registerWorks=false; LabCvarBridge failed;
    auto ignore=[](const auto&) { return false; };
    expect(!failed.initializeBackend(api,rejected.backend(),ini,ignore)&&failed.blocked()&&rejected.creates==1,"unregistered variables stop initialization immediately");
    auto initialDiagnostics=diagnostics;
    for(unsigned attempt=0;attempt<50;++attempt) failed.initializeBackend(api,rejected.backend(),ini,ignore);
    expect(rejected.creates==1&&diagnostics==initialDiagnostics&&commands==0,"registration failure produces one diagnostic and no retry storm");
    EngineFixture restoreFailure; restoreFailure.writeWorks=false; LabCvarBridge restoreBlocked;
    expect(!restoreBlocked.initializeBackend(api,restoreFailure.backend(),ini,ignore)&&restoreBlocked.blocked()&&!restoreBlocked.ready(),
        "failed restoration of a saved current value stops initialization without misreporting the factory default as current");
    EngineFixture foreign; foreign.names["labplus_path_show"]=foreign.names["com_debugHUD"]; LabCvarBridge collision;
    expect(!collision.initializeBackend(api,foreign.backend(),ini,ignore)&&collision.blocked()&&foreign.creates==0,"foreign names are detected before creating or changing any cvar");
    EngineFixture foreignAlias; foreignAlias.names["labplus_viewer"]=foreignAlias.names["com_debugHUD"]; LabCvarBridge aliasCollision;
    expect(!aliasCollision.initializeBackend(api,foreignAlias.backend(),ini,ignore)&&aliasCollision.blocked()&&foreignAlias.creates==0,"short-name conflicts are checked before creating canonical cvars");

    auto* block=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x744d000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)); expect(block!=nullptr,"ABI fixture allocation");
    uintptr_t base=reinterpret_cast<uintptr_t>(block); LabCvarSystem system{base+0x800,base+0x178e160};
    put(block,0x800,base+0x1000); put(block,0x1108,base+0x178f470); put(block,0x1038,base+0x178f8e0);
    const auto* baselineAbi=labCvarAbiProfile(kProfile6663);
    memcpy(block+0x371100,baselineAbi->constructorPrefix,sizeof(baselineAbi->constructorPrefix));
    memcpy(block+0x178f470,baselineAbi->registerPrefix,sizeof(baselineAbi->registerPrefix));
    memcpy(block+0x178f8e0,baselineAbi->setPrefix,sizeof(baselineAbi->setPrefix));
    DWORD old=0; VirtualProtect(block+0x371000,4096,PAGE_EXECUTE_READWRITE,&old); VirtualProtect(block+0x178f000,4096,PAGE_EXECUTE_READWRITE,&old);
    GameProfile profile=*kGameProfiles[20]; profile.timestamp=0x669f0dcf; profile.imageSize=0x744d000;
    expect(bool(resolveLabCvarFactory(WallMemory{},system,base,profile)),"audited constructor, register and setter code resolves");
    expect(!resolveLabCvarFactory(WallMemory{},system,base,kProfile11),"game 1.1 rejects the foreign 6.66 method addresses");
    for(const auto* selected:kGameProfiles) {
        const bool native=bool(resolveLabCvarFactory(WallMemory{},system,base,*selected));
        expect(native==(selected->timestamp==profile.timestamp&&selected->imageSize==profile.imageSize),"every supported patch selects only its audited native ABI");
        std::unordered_set<std::string> names;
        expect(registerLabConsoleCommands(!native,[&](const char* name) { return names.count(name)!=0; },
            [&](const char* name,const char*) { names.insert(name); }),"command registration succeeds for every supported patch");
        for(const auto& definition:labCvarDefinitions)
            expect(names.count(definition.name)==size_t(!native&&!definition.source),"main fallbacks register once while native cvars and managed HUD settings avoid duplicate commands");
    }
    block[0x371100]^=1; expect(!resolveLabCvarFactory(WallMemory{},system,base,profile),"changed constructor fails closed"); block[0x371100]^=1;
    block[0x178f8e0]^=1; expect(!resolveLabCvarFactory(WallMemory{},system,base,profile),"changed setter fails closed"); block[0x178f8e0]^=1;
    put(block,0x1108,base+0x178f480); expect(!resolveLabCvarFactory(WallMemory{},system,base,profile),"foreign registration method is rejected"); put(block,0x1108,base+0x178f470);
    VirtualProtect(block+0x371000,4096,PAGE_READWRITE,&old); expect(!resolveLabCvarFactory(WallMemory{},system,base,profile),"nonexecutable constructor is rejected");
    for(const auto* other:kGameProfiles) if(other->timestamp!=profile.timestamp)
        expect(!resolveLabCvarFactory(WallMemory{},system,base,*other),"other patch identities reject this fixture's foreign method addresses");
    expect(std::size(labCvarAbiProfiles)==std::size(kGameProfiles),"native cvar creation covers every supported retail profile");
    for(const auto* selected:kGameProfiles) {
        const auto* abi=labCvarAbiProfile(*selected);
        expect(abi!=nullptr,"no supported patch may silently lose native cvars and engine toggle support");
        expect(labCvarMode(*selected,{}, {})==LabCvarMode::Waiting,"early loading must wait rather than latch a command fallback on an audited patch");
        for(uintptr_t address:{base+abi->construct,base+abi->registerVariable,base+abi->setString})
            VirtualProtect(reinterpret_cast<void*>(address&~uintptr_t(4095)),4096,PAGE_EXECUTE_READWRITE,&old);
        memcpy(block+abi->construct,abi->constructorPrefix,sizeof(abi->constructorPrefix));
        memcpy(block+abi->registerVariable,abi->registerPrefix,sizeof(abi->registerPrefix));
        memcpy(block+abi->setString,abi->setPrefix,sizeof(abi->setPrefix));
        put(block,0x1108,base+abi->registerVariable); put(block,0x1038,base+abi->setString);
        auto factory=resolveLabCvarFactory(WallMemory{},system,base,*selected);
        expect(bool(factory)&&labCvarMode(*selected,system,factory)==LabCvarMode::Native,"each audited patch transitions from waiting to native cvars when the engine is ready");
        block[abi->construct]^=1;
        expect(!resolveLabCvarFactory(WallMemory{},system,base,*selected),"a changed constructor is rejected on every audited patch");
        block[abi->construct]^=1;
        block[abi->registerVariable+24]^=1;
        expect(!resolveLabCvarFactory(WallMemory{},system,base,*selected),"changed registration code is rejected on every patch");
        block[abi->registerVariable+24]^=1;
        block[abi->setString+8]^=1;
        expect(!resolveLabCvarFactory(WallMemory{},system,base,*selected),"changed setter code is rejected on every patch");
        block[abi->setString+8]^=1;
        GameProfile unknown=*selected;unknown.timestamp^=1;
        expect(!resolveLabCvarFactory(WallMemory{},system,base,unknown),"unknown executable identities cannot borrow a nearby patch ABI");
        expect(labCvarMode(unknown,{}, {})==LabCvarMode::Commands&&labCvarMode(*selected,system,{})==LabCvarMode::Commands,
            "unknown identities and rejected code retain the guarded command fallback");
        put(block,0x1108,base+abi->registerVariable+1);
        expect(!resolveLabCvarFactory(WallMemory{},system,base,*selected),"a replaced registration method is rejected on every audited patch");
    }
    VirtualFree(block,0,MEM_RELEASE); std::filesystem::remove_all(temporary);
    puts("PASS: canonical settings command registration/completion, help, queries, bounds, companion dispatch, conflicts and failures; native cvar creation/sync and guarded ABI selection on all game profiles. No engine commands executed.");
}
