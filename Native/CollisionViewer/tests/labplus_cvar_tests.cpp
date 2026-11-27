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
struct EngineFixture {
    std::unordered_map<std::string,uintptr_t> names;
    std::unordered_map<std::string,std::string> values;
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
                names[definition.name]=variable; change(definition.name,atoi(storage.initial));
            },
            [this](const char* name,const char* value) { ++writes; if(writeWorks) change(name,atoi(value),value); }
        };
    }
};
static void checkSettingsCommands(const MeathookHudApi& api,const std::wstring& ini) {
    std::unordered_set<std::string> registered;
    auto exists=[&](const char* name) { return registered.count(name)!=0; };
    auto add=[&](const char* name,const char* usage) {
        expect(usage&&*usage&&registered.insert(name).second,"engine receives each completion name once");
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
    expect(normalizeLabArguments({"labplus_command_list"})==std::vector<std::string>{"labplus_help"}&&
        registered.count("labplus_command_list"),"singular list alias remains discoverable and usable");
    expect(normalizeLabArguments({"labplus_viewer_show","1"})==std::vector<std::string>{"labplus_viewer","1"}&&
        normalizeLabArguments({"labplus_shots_show","1"})==std::vector<std::string>{"labplus_shots","1"},"viewer and shot toggles use the existing companion handlers");
    consoleOutput.clear(); printLabCommandList(api,true);
    std::istringstream help(consoleOutput); std::string line;
    while(std::getline(help,line)) expect(registered.count(line.substr(0,line.find(' ')))==1,"every help entry has an engine registration");
    consoleOutput.clear();
    expect(printLabSettingQuery(api,{"labplus_viewer_distance"},ini)&&consoleOutput.find("labplus_viewer_distance is 75")!=std::string::npos,
        "query reads the persisted setting without sending a request");
    expect(!printLabSettingQuery(api,{"labplus_viewer_show","1"},ini)&&commands==0,"set requests are not swallowed by the query handler");
    registered.clear();
    expect(registerLabConsoleCommands(false,exists,add),"audited cvar backend keeps action commands separate");
    for(const auto& definition:labCvarDefinitions) expect(!registered.count(definition.name),"native cvars never get duplicate command registrations");
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
int main() {
    MeathookHudApi api{}; api.consolePrint=print; api.executeCommand=execute;
    auto temporary=std::filesystem::temp_directory_path()/(L"labplus-Cvar-Test-"+std::to_wstring(GetCurrentProcessId()));
    std::filesystem::create_directories(temporary); auto ini=(temporary/L"viewer.ini").wstring();
    auto setSaved=[&](const wchar_t* key,const wchar_t* value) { expect(WritePrivateProfileStringW(L"Viewer",key,value,ini.c_str())!=0,"isolated saved setting"); };
    setSaved(L"ShowResearchPath",L"1"); setSaved(L"Distance",L"75");
    std::filesystem::create_directories(temporary/L"hud");
    auto hudIni=(temporary/L"hud"/L"console.ini").wstring();
    expect(WritePrivateProfileStringW(L"Viewer",L"VerticalShowSign",L"0",hudIni.c_str())!=0,"isolated HUD console setting");
    checkSettingsCommands(api,ini);
    EngineFixture engine; LabCvarBridge bridge; std::vector<std::vector<std::string>> requests;
    expect(bridge.initializeBackend(api,engine.backend(),ini,[&](const auto& request) { requests.push_back(request); return true; }),"native creation registers all owned variables");
    expect(engine.creates==std::size(labCvarDefinitions)&&engine.number("labplus_path_show")==1&&engine.number("labplus_viewer_distance")==75&&
        engine.number("labplus_player_show")==0&&engine.number("labplus_attempts_show")==1&&
        engine.number("labplus_speedometer_verticalsign")==0&&commands==0,"initialization uses main and HUD saved values and sends no console commands");
    for(ULONGLONG time=100;time<1000;time+=100) bridge.poll(api,true,time);
    expect(engine.writes==0&&requests.empty(),"unchanged settings cause no writes or console spam");
    setSaved(L"ShowResearchPath",L"0"); bridge.poll(api,true,1000);
    expect(engine.number("labplus_path_show")==0&&engine.writes==1&&requests.empty(),"UI changes update native cvars quietly");
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

    EngineFixture rejected; rejected.registerWorks=false; LabCvarBridge failed;
    auto ignore=[](const auto&) { return false; };
    expect(!failed.initializeBackend(api,rejected.backend(),ini,ignore)&&failed.blocked()&&rejected.creates==1,"unregistered variables stop initialization immediately");
    auto initialDiagnostics=diagnostics;
    for(unsigned attempt=0;attempt<50;++attempt) failed.initializeBackend(api,rejected.backend(),ini,ignore);
    expect(rejected.creates==1&&diagnostics==initialDiagnostics&&commands==0,"registration failure produces one diagnostic and no retry storm");
    EngineFixture foreign; foreign.names["labplus_path_show"]=foreign.names["com_debugHUD"]; LabCvarBridge collision;
    expect(!collision.initializeBackend(api,foreign.backend(),ini,ignore)&&collision.blocked()&&foreign.creates==0,"foreign names are detected before creating or changing any cvar");

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
