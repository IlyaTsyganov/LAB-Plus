#include "../src/vulkan_layer.cpp"

// Bounded read-only observer for user-performed interactions. No injection,
// engine calls, input generation or writes to the game process.
static void dumpSyncTimeline(std::ostream& out,const WallMemory& memory,uintptr_t sync,const GraphLookup& lookup) {
    auto member=[&](const char* type,const char* name){return timerField(lookup,type,name);};
    auto pointer=[&](uintptr_t object,const char* type,const char* name){uintptr_t value=0;const auto* f=member(type,name);if(f)memory.get(object+f->offset,value);return value;};
    auto integer=[&](uintptr_t object,const char* type,const char* name){int32_t value=-1;const auto* f=member(type,name);if(f)memory.get(object+f->offset,value);return value;};
    auto managed=[&](uintptr_t object,const char* type,const char* name){const auto* f=member(type,name);return f?pointer(object+f->offset,f->type.c_str(),"ptr"):0;};
    auto resource=[&](uintptr_t object){return timerResourceName(memory,object,lookup);};
    const auto animation=pointer(sync,"idSyncEntity","currentSyncEntityAnim");
    out<<"SYNC "<<std::hex<<sync<<std::dec<<" anim="<<resource(animation)<<" owner="<<std::hex<<managed(sync,"idSyncEntity","syncPlayer")<<std::dec<<'\n';
    const auto* animator=member("idSyncEntity","animatorSyncEntity");const auto* models=member("idAnimator_AnimWeb","modelAnimStates");
    const auto* declModels=member("idDeclAnimWeb","models");
    if(!animator||!models||!declModels)return;
    const auto anim=sync+animator->offset,web=pointer(anim,"idAnimator_AnimWeb","decl");
    const auto states=pointer(anim+models->offset,models->type.c_str(),"list"),decls=pointer(web+declModels->offset,declModels->type.c_str(),"list");
    const auto num=integer(anim+models->offset,models->type.c_str(),"num"),declNum=integer(web+declModels->offset,declModels->type.c_str(),"num");
    out<<"MODELS "<<num<<" decls="<<declNum<<'\n';if(num<0||num>64||num!=declNum||!states||!decls)return;
    const auto& state=lookup("idAnimator_AnimWeb::idModelAnimState");const auto& info=lookup("idDeclAnimWeb::idModelAnimInfo");
    for(int model=0;model<num;++model) {
        const auto root=pointer(states+size_t(model)*state.size,"idAnimator_AnimWeb::idModelAnimState","tree");
        const auto md6=pointer(decls+size_t(model)*info.size,"idDeclAnimWeb::idModelAnimInfo","model");
        out<<"MODEL "<<model<<" md6="<<resource(md6)<<" root="<<std::hex<<root<<std::dec<<'\n';
        std::vector<uintptr_t> pending{root},visited;
        while(!pending.empty()&&visited.size()<128) {
            const auto node=pending.back();pending.pop_back();if(!node||std::find(visited.begin(),visited.end(),node)!=visited.end())continue;visited.push_back(node);
            const auto* kind=member("idMD6Node","type");unsigned char type=255;if(!kind||!memory.get(node+kind->offset,type))continue;
            if(type==0||type==3||type==4) {pending.push_back(pointer(node,"idMD6Branch","left"));pending.push_back(pointer(node,"idMD6Branch","right"));continue;}
            if(type!=2)continue;
            const auto leafAnim=pointer(node,"idMD6Leaf","anim");const auto data=pointer(leafAnim,"idMD6Anim","animData");const auto name=resource(leafAnim);
            out<<"LEAF "<<std::hex<<node<<std::dec<<" anim="<<name;
            for(const char* field:{"wrapMode","lastTotalWeight","startTime","startTimeFraction","rateScale","lastTicksPerSec"}) {
                const auto* f=member((strcmp(field,"wrapMode")==0||strcmp(field,"lastTotalWeight")==0)?"idMD6Leaf":"idMD6LeafPlay",field);if(!f)continue;
                out<<' '<<field<<'=';
                if(f->type=="float") {float x=0;memory.get(node+f->offset,x);out<<x;}
                else if(f->size==8) {int64_t x=0;memory.get(node+f->offset,x);out<<x;}
                else {unsigned char x=0;memory.get(node+f->offset,x);out<<unsigned(x);}
            }
            for(const char* field:{"numFrames","frameRate"}) {const auto* f=member("idMD6AnimData",field);uint16_t x=0;if(f)memory.get(data+f->offset,x);out<<' '<<field<<'='<<x;}
            const auto* scales=member("idMD6LeafPlay","gameRateScales");float values[3]{};if(scales&&scales->size==12&&memory.read(node+scales->offset,values,12))out<<" scales="<<values[0]<<','<<values[1]<<','<<values[2];out<<'\n';
            const auto* events=member("idDeclMD6","animEvents"),*list=member("idAnimEvents","list");if(!events||!list)continue;
            const auto at=md6+events->offset+list->offset,records=pointer(at,list->type.c_str(),"list");const auto count=integer(at,list->type.c_str(),"num");if(!records||count<0||count>10000)continue;
            const auto& record=lookup("idAnimEvents::animEventInfo_t");const auto* ref=member("idAnimEvents::animEventInfo_t","animRef");
            const auto* eventList=member("idAnimEvents::animEventInfo_t","events");if(!ref||!eventList)continue;
            for(int i=0;i<count;++i) {
                const auto entry=records+size_t(i)*record.size;uintptr_t text=0;if(!memory.get(entry+ref->offset,text)||readEntityText(memory,text,512)!=name)continue;
                const auto ea=entry+eventList->offset,els=pointer(ea,eventList->type.c_str(),"list");const auto en=integer(ea,eventList->type.c_str(),"num");
                out<<"EVENTS "<<en<<'\n';if(!els||en<0||en>4096)continue;
                const auto& event=lookup("idMD6AnimEvent");
                for(int j=0;j<en;++j) {
                    const auto e=els+size_t(j)*event.size;float frame=0;int16_t number=-1;uint32_t flags=~0u;
                    const auto* f=member("idMD6AnimEvent","frameNum"),*n=member("idMD6AnimEvent","eventNum"),*fl=member("idMD6AnimEvent","flags");
                    if(f&&n&&fl&&memory.get(e+f->offset,frame)&&memory.get(e+n->offset,number)&&memory.get(e+fl->offset,flags))out<<"EVENT frame="<<frame<<" num="<<number<<" flags="<<flags<<'\n';
                }
            }
        }
    }
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=6){puts("Usage: SyncLiveProbe PID BASE metadata.tsv output.log seconds");return 2;}
    const DWORD pid=_wtoi(argv[1]);const auto base=_wcstoui64(argv[2],nullptr,0);const unsigned seconds=std::clamp<unsigned>(_wtoi(argv[5]),1,300);
    HANDLE handle=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!handle){printf("OpenProcess failed %lu\n",GetLastError());return 3;}
    int result=0;
    try {
        WallMemory memory{handle};const auto* profile=detectGameProfile(memory,base);if(!profile)throw std::runtime_error("Unknown executable");
        std::ifstream meta{std::filesystem::path(argv[3])};std::string line;if(!std::getline(meta,line)||line!="#LABPLUS_GRAPH_LAYOUTS_V1\t"+std::to_string(pid))throw std::runtime_error("Foreign metadata");
        std::unordered_map<std::string,uintptr_t> addresses;while(std::getline(meta,line)){auto tab=line.find('\t');if(tab!=std::string::npos)addresses[line.substr(0,tab)]=std::stoull(line.substr(tab+1),nullptr,0);}
        std::unordered_map<std::string,GraphType> schemas;
        GraphLookup lookup=[&](const std::string& name)->const GraphType& {auto f=schemas.find(name);if(f!=schemas.end())return f->second;auto a=addresses.find(name);static const GraphType empty;return a==addresses.end()?empty:schemas.emplace(name,readGraphType(memory,a->second,name)).first->second;};
        std::ofstream out{std::filesystem::path(argv[4])};out<<profile->name<<" PID="<<pid<<'\n';
        auto member=[&](const char* type,const char* name){return timerField(lookup,type,name);};
        auto managed=[&](uintptr_t object,const char* type,const char* name){const auto* f=member(type,name);uintptr_t value=0;if(f){const auto* p=timerField(lookup,f->type,"ptr");if(p)memory.get(object+f->offset+p->offset,value);}return value;};
        auto started=GetTickCount64();ULONGLONG nextDump=0;std::string previous;
        while(GetTickCount64()-started<seconds*1000ULL) {
            const auto now=GetTickCount64()-started;const auto level=readLevelIdentity(memory,base,*profile);uintptr_t player=0;if(level)memory.get(level.map+profile->mapPlayerOffset,player);
            if(player) {
                const auto* flame=member("idPlayer","flameBelchNextUseTime");const auto clock=flame?readGameTimerClock(memory,base,*profile,level,flame->offset):GameTimerClock{};
                const int release=syncReleaseEventNumber(memory,base,*profile,lookup);auto rows=captureSyncAnimationTimers(memory,player,clock,lookup,base,profile);
                std::ostringstream summary;summary<<"map="<<level.name<<" release="<<release<<" clock="<<clock.valid;
                for(const auto& row:rows)summary<<" | "<<row.key<<' '<<row.state<<' '<<row.remaining<<" / "<<row.maximum<<' '<<row.detail;
                std::vector<uintptr_t> candidates;
                for(const char* name:{"syncMaster","savedSyncEntity","mostRecentSyncEntity"}){auto p=managed(player,"idPlayer",name);summary<<' '<<name<<'='<<std::hex<<p<<std::dec;if(p)candidates.push_back(p);}
                if(summary.str()!=previous){out<<"SAMPLE "<<now<<" ticks="<<clock.ticks<<' '<<summary.str()<<'\n';previous=summary.str();}
                if(now>=nextDump) {
                    nextDump=now+500;
                    std::sort(candidates.begin(),candidates.end());candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
                    for(auto sync:candidates) {
                        unsigned char on=0;const auto* working=member("idSyncEntity","isCurrentlyWorking");if(!working||!memory.get(sync+working->offset,on)||!on)continue;
                        auto direct=captureSyncPlayerControlLock(memory,player,sync,clock,lookup,release,base,profile);
                        out<<"DIRECT "<<now<<' '<<direct.state<<' '<<direct.remaining<<' '<<direct.detail<<'\n';dumpSyncTimeline(out,memory,sync,lookup);
                    }
                }
            }else if(previous!="no-player"){out<<"SAMPLE "<<now<<" no-player\n";previous="no-player";}
            out.flush();Sleep(50);
        }
        puts("Read-only sync observation complete.");
    }catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());result=1;}
    CloseHandle(handle);return result;
}
