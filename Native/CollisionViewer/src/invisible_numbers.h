#pragma once

using InvisibleEnemyRow=EnemyTelemetryRow;
struct InvisibleTimerRow {
    std::string key,group,label,state="unavailable",detail;
    double seconds=0;
    bool known=false;
    double maximum=-1;
};
struct InvisibleTechRow { std::string key,label,state,detail; };
struct InvisibleNumbersSnapshot {
    DWORD process=GetCurrentProcessId(); uint64_t captured=0;
    LevelIdentity level;
    std::string phase="waiting_meathook",enemiesState="metadata_pending",timersState="metadata_pending",detail;
    uint32_t unavailable=0,gameClockFlameOffset=~0u;
    std::vector<InvisibleEnemyRow> enemies,barEnemies;
    std::vector<InvisibleTimerRow> timers;
    std::vector<InvisibleTechRow> techs;
};
static InvisibleNumbersSnapshot captureInvisibleNumbers(const WallMemory&,uintptr_t,const GameProfile&,const LevelIdentity&,const GraphLookup&,unsigned features=7);
static bool numbersRequestFresh(const std::filesystem::path& directory,uint64_t now) {
    std::ifstream input(directory/(L"invisible-numbers-"+std::to_wstring(GetCurrentProcessId())+L".request"));
    uint64_t requested=0;
    return bool(input>>requested)&&requested<=now+1000&&now<=requested+2000;
}
static std::string numberText(double value,unsigned decimals=2) {
    std::ostringstream output; output.imbue(std::locale::classic()); output.setf(std::ios::fixed); output.precision(decimals); output<<value; return output.str();
}
static std::string cleanNumberLabel(std::string value) {
    for(char& c:value) if(static_cast<unsigned char>(c)<32||c==127||c=='%'||c=='^') c='?';
    if(value.size()>1000) value.resize(1000); return value;
}
static std::string timerCountdownText(const InvisibleTimerRow& row) {
    if((row.state!="active"&&row.state!="cooldown")||!row.known) return row.state;
    return numberText(row.seconds)+(std::isfinite(row.maximum)&&row.maximum>0?" / "+numberText(row.maximum):"")+" s";
}
static std::vector<std::string> timerHudLines(const std::vector<InvisibleTimerRow>& timers) {
    std::vector<std::string> lines{"LAB Plus | Game timers"};
    for(const auto& row:timers) {
        if(row.group=="sticky_charges"||row.state=="inactive"||row.state=="unavailable"||row.state=="not_acquired") continue;
        auto line=row.label+": "+timerCountdownText(row);
        if(row.key=="sticky_bomb") {
            unsigned index=0;
            for(const auto& charge:timers) if(charge.group=="sticky_charges"&&charge.known) {
                line+="   ["+std::to_string(++index)+": "+timerCountdownText(charge)+"]";
            }
        }
        lines.push_back(cleanNumberLabel(std::move(line)));
    }
    if(lines.size()==1) lines.push_back("No available timer values");
    return lines;
}
static bool writeInvisibleNumbers(const InvisibleNumbersSnapshot& snapshot,const std::filesystem::path& directory) {
    if(directory.empty()) return false;
    try {
        std::filesystem::create_directories(directory);
        auto path=directory/(L"invisible-numbers-"+std::to_wstring(snapshot.process)+L".tsv"),temporary=path; temporary+=L".tmp";
        std::ofstream output(temporary,std::ios::binary|std::ios::trunc); output.imbue(std::locale::classic()); output.precision(12);
        output<<"#LABPLUS_NUMBERS_V1\t"<<snapshot.process<<'\t'<<snapshot.captured<<'\t'<<snapshot.phase<<'\t'<<cleanNumberLabel(snapshot.level.name)
            <<'\t'<<snapshot.enemiesState<<'\t'<<snapshot.timersState<<'\t'<<cleanNumberLabel(snapshot.detail)<<'\t'<<snapshot.unavailable<<'\n';
        for(const auto& row:snapshot.enemies) {
            output<<"E\t"<<row.encounterId<<'\t'<<row.encounterGeneration<<'\t'<<cleanNumberLabel(row.encounter)<<'\t'<<row.entity.id<<'\t'<<row.entity.generation
                <<'\t'<<cleanNumberLabel(row.entity.className)<<'\t'<<cleanNumberLabel(row.name)<<'\t'<<row.health<<'\t';
            if(row.maximumKnown) output<<row.maximum; else output<<'?'; output<<'\n';
        }
        for(const auto& row:snapshot.timers) {
            output<<"T\t"<<row.key<<'\t'<<cleanNumberLabel(row.group)<<'\t'<<cleanNumberLabel(row.label)<<'\t'<<row.state<<'\t';
            if(row.known) output<<row.seconds; else output<<'?'; output<<'\t'<<cleanNumberLabel(row.detail)<<'\t';
            if(std::isfinite(row.maximum)&&row.maximum>0) output<<row.maximum; else output<<'?'; output<<'\n';
        }
        for(const auto& row:snapshot.techs) output<<"S\t"<<row.key<<'\t'<<cleanNumberLabel(row.label)<<'\t'<<row.state<<'\t'<<cleanNumberLabel(row.detail)<<'\n';
        output.close(); return bool(output)&&MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING)!=0;
    } catch(...) { return false; }
}
class InvisibleNumbersBridge {
    const MeathookHudApi* api=nullptr;
    const GameProfile* profile=nullptr;
    std::mutex exchange;
    std::unordered_map<std::string,std::shared_ptr<const GraphType>> schemas;
    std::vector<std::string> requested;
    std::shared_ptr<const InvisibleNumbersSnapshot> latest;
    std::atomic<bool> frameReady{false},wanted{false};
    bool started=false,registered=false,hudRequested=false,lastEnemies=false,lastTimers=false,lastTechs=false;
    std::shared_ptr<const InvisibleNumbersSnapshot> rendered;
    ULONGLONG retryAt=0;
    static void callback(void* self) { static_cast<InvisibleNumbersBridge*>(self)->onFrame(); }
    void clearHud() { debugHudStack().clear(DebugHudBlock::Enemies); debugHudStack().clear(DebugHudBlock::Timers); debugHudStack().clear(DebugHudBlock::Techs); hudRequested=false; rendered.reset(); }
    void onFrame() {
        if(!labAppAvailable()) { wanted=false; clearHud(); return; }
        frameReady=true;
        for(unsigned i=0;i<4;++i) {
            std::string type;
            { std::lock_guard<std::mutex> lock(exchange); if(requested.empty()) break; type=std::move(requested.back()); requested.pop_back(); }
            auto schema=std::make_shared<const GraphType>(readGraphType(WallMemory{},reinterpret_cast<uintptr_t>(api->findClass(type.c_str())),type));
            std::lock_guard<std::mutex> lock(exchange); schemas[type]=std::move(schema);
        }
        bool enemies=showEnemyHealthHud.load(),timers=showGameTimersHud.load(),techs=showSpeedrunTechsHud.load();
        auto now=encounterEpochMillis();
        // Request-file I/O is confined to the worker.
        if(!wanted.load()) { clearHud(); return; }
        if((!enemies&&!timers&&!techs)||!profile) { clearHud(); return; }
        uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        auto current=snapshot();
        if(!current||now>current->captured+500||current->captured>now+1000||!gameplayVisible(WallMemory{},base,*profile)||
            readConsoleFraction(WallMemory{},base,*profile)>0||readLevelIdentity(WallMemory{},base,*profile)!=current->level) { clearHud(); return; }
        if(rendered==current&&lastEnemies==enemies&&lastTimers==timers&&lastTechs==techs) return;
        rendered=current; lastEnemies=enemies; lastTimers=timers; lastTechs=techs;
        if(!hudRequested&&api->executeCommand) { api->executeCommand("com_debugHUD 1"); hudRequested=true; }
        if(enemies) {
            std::vector<std::string> lines{"LAB Plus | Living enemies"};
            uint32_t previous=~0u; std::string encounter;
            for(const auto& row:current->enemies) {
                if(previous!=row.encounterId||encounter!=row.encounter) {
                    previous=row.encounterId; encounter=row.encounter; lines.push_back(encounter);
                }
                lines.push_back("  "+(row.name.empty()?row.entity.className:row.name)+"  "+numberText(row.health,1)+(row.maximumKnown?" / "+numberText(row.maximum,1):""));
            }
            if(current->enemies.empty()) lines.push_back(current->enemiesState=="ready"?"No living enemies in active encounters":current->enemiesState);
            for(auto& line:lines) line=cleanNumberLabel(line);
            debugHudStack().publish(DebugHudBlock::Enemies,api,lines,16,.72f,0xffffd040u);
        } else debugHudStack().clear(DebugHudBlock::Enemies);
        if(timers) {
            auto lines=timerHudLines(current->timers);
            debugHudStack().publish(DebugHudBlock::Timers,api,lines,16,.72f,0xffffd040u);
        } else debugHudStack().clear(DebugHudBlock::Timers);
        if(techs) {
            std::vector<std::string> lines{"LAB Plus | Speedrun techniques"};
            for(const auto& row:current->techs) lines.push_back(cleanNumberLabel(row.label+": "+row.state));
            debugHudStack().publish(DebugHudBlock::Techs,api,lines,16,.72f,0xffffd040u);
        } else debugHudStack().clear(DebugHudBlock::Techs);
    }
    void run() {
        uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); const auto directory=researchDirectory(); unsigned iterations=0;
        for(;;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if(!labAppAvailable()) { wanted=false; continue; }
            const bool window=numbersRequestFresh(directory,encounterEpochMillis());
            const unsigned features=(window?7u:((showEnemyHealthHud.load()?1u:0u)|(showGameTimersHud.load()?2u:0u)|(showSpeedrunTechsHud.load()?4u:0u)))|(showEnemyHealthBars.load()?8u:0u);
            wanted=features!=0;
            if(!wanted.load()) continue;
            InvisibleNumbersSnapshot value; value.captured=encounterEpochMillis();
            try {
                if(!profile) value.phase="unsupported_profile";
                else if(!frameReady.load()) value.phase="waiting_meathook";
                else {
                    std::unordered_map<std::string,std::shared_ptr<const GraphType>> copied;
                    { std::lock_guard<std::mutex> lock(exchange);
                        if(++iterations%50==0) for(auto it=schemas.begin();it!=schemas.end();) { if(!it->second->complete) it=schemas.erase(it); else ++it; }
                        copied=schemas;
                    }
                    std::vector<std::string> missing;
                    auto lookup=[&](const std::string& type)->const GraphType& {
                        auto found=copied.find(type); if(found!=copied.end()) return *found->second;
                        if(missing.size()<256&&std::find(missing.begin(),missing.end(),type)==missing.end()) missing.push_back(type);
                        static const GraphType empty; return empty;
                    };
                    auto level=readLevelIdentity(WallMemory{},base,*profile);
                    if(!level) value.phase="loading";
                    else {
                        value=captureInvisibleNumbers(WallMemory{},base,*profile,level,lookup,features);
                        if(readLevelIdentity(WallMemory{},base,*profile)!=level) { value.phase="loading"; value.enemies.clear(); value.barEnemies.clear(); value.timers.clear(); value.techs.clear(); }
                        else if(value.phase=="live"&&!gameplayVisible(WallMemory{},base,*profile)) value.phase="paused";
                    }
                    { std::lock_guard<std::mutex> lock(exchange);
                        for(const auto& type:missing) if(schemas.find(type)==schemas.end()&&std::find(requested.begin(),requested.end(),type)==requested.end()) requested.push_back(type);
                    }
                }
            } catch(...) { value.phase="read_error"; value.enemies.clear(); value.barEnemies.clear(); value.timers.clear(); value.techs.clear(); }
            { std::lock_guard<std::mutex> lock(exchange); latest=std::make_shared<const InvisibleNumbersSnapshot>(value); }
            writeInvisibleNumbers(value,directory);
        }
    }
public:
    std::shared_ptr<const InvisibleNumbersSnapshot> snapshot() { std::lock_guard<std::mutex> lock(exchange); return latest; }
    void connect() {
        if(!labAppAvailable()) return;
        HMODULE pinned=nullptr;
        if(!started) {
            if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(&callback),&pinned)) return;
            profile=currentGameProfile(); started=true; std::thread([this] { run(); }).detach();
        }
        if(registered||GetTickCount64()<retryAt) return; retryAt=GetTickCount64()+2000;
        HMODULE module=GetModuleHandleW(L"XINPUT1_3.dll"); if(!module) return;
        auto getter=reinterpret_cast<const MeathookHudApi* (*)()>(GetProcAddress(module,"mh_api_get")); if(!getter) return;
        const auto* candidate=getter(); MeathookHudApi copy{};
        if(!candidate||!WallMemory{}.get(reinterpret_cast<uintptr_t>(candidate),copy)||!copy.findClass||!copy.registerPreFrame||!copy.addText||!copy.removeText) return;
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(getter),&pinned)) return;
        api=candidate; registered=true; debugHudStack().connect(api,DebugHudProducer::Numbers,callback,this);
    }
};
static InvisibleNumbersBridge& invisibleNumbers() { static auto* value=new InvisibleNumbersBridge; return *value; }
