#include "../src/vulkan_layer.cpp"
// Read-only production capture. Module base and metadata addresses are explicit
// so the probe needs only VM_READ / QUERY_LIMITED_INFORMATION access.
int wmain(int argc,wchar_t** argv) {
    if(argc!=5) { puts("Usage: NumbersLiveProbe PID BASE metadata-addresses.tsv output-directory"); return 2; }
    DWORD pid=static_cast<DWORD>(_wtoi(argv[1])); uintptr_t base=_wcstoui64(argv[2],nullptr,0);
    HANDLE handle=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    if(!handle) { printf("OpenProcess failed: %lu\n",GetLastError()); return 3; }
    int code=0;
    try {
        WallMemory memory{handle}; auto* profile=detectGameProfile(memory,base);
        if(!profile) throw std::runtime_error("Unsupported executable");
        std::ifstream input{std::filesystem::path(argv[3])}; std::string line;
        if(!std::getline(input,line)||line!="#LABPLUS_GRAPH_LAYOUTS_V1\t"+std::to_string(pid)) throw std::runtime_error("Foreign metadata addresses");
        std::unordered_map<std::string,uintptr_t> addresses;
        while(std::getline(input,line)) { auto tab=line.find('\t'); if(tab!=std::string::npos) addresses.emplace(line.substr(0,tab),std::stoull(line.substr(tab+1),nullptr,0)); }
        std::unordered_map<std::string,GraphType> schemas; std::unordered_set<std::string> missing,rejected;
        auto lookup=[&](const std::string& type)->const GraphType& {
            auto found=schemas.find(type); if(found!=schemas.end()) return found->second;
            auto address=addresses.find(type); if(address==addresses.end()) { missing.insert(type); static const GraphType empty; return empty; }
            auto value=readGraphType(memory,address->second,type); if(!value.complete) rejected.insert(type);
            return schemas.emplace(type,std::move(value)).first->second;
        };
        auto begin=GetTickCount64(); auto level=readLevelIdentity(memory,base,*profile);
        auto snapshot=captureInvisibleNumbers(memory,base,*profile,level,lookup); snapshot.process=pid;
        if(!gameplayVisible(memory,base,*profile)&&snapshot.phase=="live") snapshot.phase="paused";
        printf("Profile: %s\nMap: %s\nPhase: %s; enemies=%s/%zu; timers=%s/%zu; techs=%zu; unavailable=%u; elapsed=%llums\n",profile->name,level.name.c_str(),snapshot.phase.c_str(),snapshot.enemiesState.c_str(),snapshot.enemies.size(),snapshot.timersState.c_str(),snapshot.timers.size(),snapshot.techs.size(),snapshot.unavailable,GetTickCount64()-begin);
        for(const auto& timer:snapshot.timers) printf("Timer: %s | %s | %.6f | %s\n",timer.key.c_str(),timer.state.c_str(),timer.known?timer.seconds:-1,timer.detail.c_str());
        for(const auto& tech:snapshot.techs) printf("Tech: %s | %s | %s\n",tech.key.c_str(),tech.state.c_str(),tech.detail.c_str());
        for(const auto& type:missing) printf("MISSING\t%s\n",type.c_str());
        for(const auto& type:rejected) printf("REJECTED\t%s\n",type.c_str());
        if(!writeInvisibleNumbers(snapshot,std::filesystem::path(argv[4]))) throw std::runtime_error("Could not save production snapshot");
        if(!missing.empty()||!rejected.empty()) code=4;
    } catch(const std::exception& error) { fprintf(stderr,"%s\n",error.what()); code=1; }
    CloseHandle(handle); return code;
}
