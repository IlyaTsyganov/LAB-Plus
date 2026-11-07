#include "../src/vulkan_layer.cpp"

// External, bounded, read-only check of the exact production reader. This
// executable never invokes a game function, injects code or generates input.
int wmain(int argc,wchar_t** argv) {
    if(argc!=6) {std::fprintf(stderr,"PID base layouts.tsv output.tsv seconds\n");return 2;}
    const DWORD pid=std::stoul(argv[1]);const uintptr_t base=std::stoull(argv[2],nullptr,0);
    const unsigned seconds=std::stoul(argv[5]);if(!seconds||seconds>120)return 2;
    if(std::filesystem::exists(argv[4]))return 2;
    HANDLE process=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,pid);if(!process)return 3;
    int status=0;
    try {
        WallMemory memory{process};const auto* profile=detectGameProfile(memory,base);
        if(!profile||profile->timestamp!=kProfile11.timestamp||profile->imageSize!=kProfile11.imageSize)throw std::runtime_error("Expected audited 1.1");
        std::ifstream input{std::filesystem::path(argv[3])};if(!input)throw std::runtime_error("Missing reflection index");
        std::unordered_map<std::string,uintptr_t> addresses;std::string line;
        while(std::getline(input,line)) {if(!line.empty()&&line.back()=='\r')line.pop_back();auto tab=line.find('\t');if(tab!=std::string::npos)addresses[line.substr(0,tab)]=base+std::stoull(line.substr(tab+1),nullptr,0);}
        std::unordered_map<std::string,GraphType> schemas;
        GraphLookup lookup=[&](const std::string& name)->const GraphType& {
            auto f=schemas.find(name);if(f!=schemas.end())return f->second;
            auto at=addresses.find(name);static const GraphType empty;
            return at==addresses.end()?empty:schemas.emplace(name,readGraphType(memory,at->second,name)).first->second;
        };
        if(!syncPreviewLayouts(lookup))throw std::runtime_error("Production reflection guard rejected layout");
        SyncTargetReader reader;std::ofstream output{std::filesystem::path(argv[4])};
        output<<"#SYNC_TARGET_READ_ONLY\t"<<pid<<"\t"<<profile->name<<'\n';
        unsigned samples=0,targets=0;double total=0,maximum=0;const auto started=GetTickCount64();
        while(GetTickCount64()-started<seconds*1000ULL) {
            const auto begin=std::chrono::steady_clock::now();const auto level=readLevelIdentity(memory,base,*profile);GameCamera camera{};
            if(!level||!readGameCamera(memory,base,*profile,camera)){Sleep(25);continue;}
            const auto value=reader.capture(memory,base,*profile,level,camera,50,&lookup);
            const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();total+=ms;maximum=std::max(maximum,ms);++samples;if(value.entity)++targets;
            output<<GetTickCount64()-started<<'\t'<<value.phase<<'\t'<<value.entity.id<<'\t'<<value.group<<'\t'<<value.distance<<'\t'<<value.spheres.size()<<'\t'<<value.candidatesReady;
            for(const auto& s:value.gloryKills)output<<"\tGK="<<s;for(const auto& s:value.chainsaw)output<<"\tCS="<<s;
            output<<"\tFACING="<<value.regionState;
            output<<"\tRANGE="<<value.ranges.detail<<":"<<value.ranges.gloryKillTarget<<":"<<value.ranges.chainsawTarget;
            for(const auto& r:value.regions)output<<"\tREGION="<<r.name<<":"<<syncFacingYaw(value.facing,r.incoming)*180/syncFacingPi<<":"<<r.halfYaw*180/syncFacingPi;
            std::vector<Vertex> fill,lines;appendSyncTargetGeometry(camera,value,fill,false);appendSyncTargetGeometry(camera,value,lines,true);
            auto chainsawValue=value;chainsawValue.action=1;std::vector<Vertex> chainsawFill;appendSyncTargetGeometry(camera,chainsawValue,chainsawFill,false);
            output<<"\tTRI="<<fill.size()/3<<"\tLINES="<<lines.size()/2<<"\tCS_TRI="<<chainsawFill.size()/3<<"\tCPU_ms="<<ms<<'\n';output.flush();Sleep(25);
        }
        std::printf("1.1 production layout accepted; samples=%u targetFrames=%u external-reader mean=%.3fms max=%.3fms\n",samples,targets,samples?total/samples:0,maximum);
    } catch(const std::exception& e) {std::fprintf(stderr,"%s\n",e.what());status=1;}
    CloseHandle(process);return status;
}
