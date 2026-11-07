// External read-only finisher probe. Arguments: PID moduleBase metadata.tsv [--replay-aim].
// Replay changes an observer-local camera only; no input, remote calls or writes.
#include "../src/vulkan_layer.cpp"
int wmain(int argc,wchar_t** argv) {
 if(argc!=4&&argc!=5)return 2;
 const bool replayRequested=argc==5&&std::wstring(argv[4])==L"--replay-aim";
 HANDLE process=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,std::stoul(argv[1]));if(!process)return 3;
 WallMemory memory{process};uintptr_t base=std::stoull(argv[2],nullptr,0);const auto* profile=detectGameProfile(memory,base);if(!profile||!syncPreviewProfile(*profile)){CloseHandle(process);return 4;}
 SyncPatchScope scope(*profile);
 std::ifstream input{std::filesystem::path(argv[3])};std::unordered_map<std::string,uintptr_t> addresses;std::string line;
 while(std::getline(input,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto tab=line.find('\t');if(tab!=std::string::npos)addresses[line.substr(0,tab)]=base+std::stoull(line.substr(tab+1),nullptr,0);}
 std::unordered_map<std::string,GraphType> schemas;GraphLookup lookup=[&](const std::string& name)->const GraphType& {auto f=schemas.find(name);if(f!=schemas.end())return f->second;auto at=addresses.find(name);static const GraphType empty;return at==addresses.end()?empty:schemas.emplace(name,readGraphType(memory,at->second,name)).first->second;};
 printf("GUARDS code=%d layout=%d authored=%d\n",syncPreviewCode(memory,base),syncPreviewLayouts(lookup),syncAuthoredNodeLayouts(lookup));
 auto level=readLevelIdentity(memory,base,*profile);GameCamera camera{};printf("WORLD %d CAMERA %d\n",bool(level),readGameCamera(memory,base,*profile,camera));
 uintptr_t player=0;memory.get(level.map+profile->mapPlayerOffset,player);
 SyncPreviewList entities;unsigned alive=0,readable=0;GameCamera replay=camera;bool replayReady=false;
 if(syncPreviewList(memory,level.map+profile->mapEntitiesOffset,32768,entities))for(int i=0;i<entities.count;++i){
  struct Entry{uint32_t id,generation;uintptr_t entity,previous,next;};Entry e{};uintptr_t vt=0;
  if(!memory.get(entities.data+i*sizeof(Entry),e)||!e.entity||!memory.get(e.entity,vt)||readEntityClass(memory,vt,base,*profile)!="idAI2")continue;
  unsigned char dead=1;if(!memory.get(e.entity+syncEntityOffset(0x6488),dead)||dead)continue;++alive;
  EntityReference ref{e.entity,level.map,entities.data+i*sizeof(Entry),e.id,e.generation,"idAI2"};if(!enemyTelemetryIdentity(memory,base,*profile,ref))continue;
  SyncFacingPose pose{};std::vector<SyncHitSphere> spheres;auto source=readSyncAuthoredSource(memory,ref,lookup);auto aim=readSyncAimFrame(memory,base,ref,camera);
  bool facing=readSyncFacingPose(memory,base,ref,pose),shot=readSyncHitSpheres(memory,base,ref,spheres);
  auto ranges=readSyncRangeLimits(memory,base,*profile,lookup,player,ref,pose,{camera.position[0],camera.position[1],camera.position[2]},pose.origin);
  uintptr_t def=0;memory.get(e.entity+0xa8,def);std::string name;syncPreviewString(memory,def+8,name);
  printf("AI %s identity=1 facing=%d shot=%d spheres=%zu source=%d rows=%zu range=%d %s aim=%d\n",name.c_str(),facing,shot,spheres.size(),source.known,source.rows.size(),ranges.known,ranges.detail.c_str(),aim.known);
  std::vector<EntityReference> blueprints;
  for(int j=0;j<entities.count;++j){Entry b{};uintptr_t definition=0;if(!memory.get(entities.data+j*sizeof(Entry),b)||!b.entity||!memory.get(b.entity+0xa8,definition)||std::find(source.blueprintDefinitions.begin(),source.blueprintDefinitions.end(),definition)==source.blueprintDefinitions.end())continue;
    EntityReference br{b.entity,level.map,entities.data+j*sizeof(Entry),b.id,b.generation,"idSyncEntity"};if(enemyTelemetryIdentity(memory,base,*profile,br))blueprints.push_back(br);
  }
  auto geometry=readSyncAuthoredGeometry(memory,base,source,blueprints,ranges.modifier,lookup);size_t priorityKnown=0;for(auto& region:geometry.regions)priorityKnown+=region.priorityKnown;
  printf("GEOMETRY blueprints=%zu known=%d complete=%d regions=%zu priorityKnown=%zu\n",blueprints.size(),geometry.known,geometry.complete,geometry.regions.size(),priorityKnown);
  if(facing&&shot&&source.known&&ranges.known)++readable;
  if(replayRequested&&name=="ai/fodder/carcass"&&!spheres.empty()){
    Vec3 eye{camera.position[0],camera.position[1],camera.position[2]},direction=subtract(spheres[0].center,eye);direction=motionScale(direction,1.f/sqrtf(dot(direction,direction)));auto right=cross(direction,{0,0,1});right=motionScale(right,1.f/sqrtf(dot(right,right)));auto up=cross(right,direction);memset(replay.matrix,0,sizeof(replay.matrix));
    const Vec3 rows[]{right,up,direction};const int starts[]{0,4,12};for(int r=0;r<3;++r){auto at=starts[r];replay.matrix[at]=rows[r].x;replay.matrix[at+1]=rows[r].y;replay.matrix[at+2]=rows[r].z;replay.matrix[at+3]=-dot(rows[r],eye);}replayReady=true;
  }
  if(alive>=12)break;
 }
 printf("AI SUMMARY alive=%u readable=%u\n",alive,readable);
 SyncTargetReader reader(0);auto f=reader.capture(memory,base,*profile,level,camera,50,&lookup);
 printf("FRAME phase=%s enemy=%s spheres=%zu ranges=%d detail=%s facing=%zu rules=%d candidates=%d aim=%d focus=%d\n",f.phase.c_str(),f.enemyDefinition.c_str(),f.spheres.size(),f.ranges.known,f.ranges.detail.c_str(),f.regions.size(),f.rulesCoherent,f.candidatesReady,f.aim.known,f.jointFocus.known);
 for(auto& s:syncPositionGuideLines(f))puts(s.c_str());
 if(replayReady)for(int action:{0,1}){
    syncTargetAction.store(action);
    auto frame=reader.capture(memory,base,*profile,level,replay,50,&lookup);
    syncRefreshAimMesh(replay,frame);
    std::vector<Vertex> lines,fill;
    appendSyncTargetGeometry(replay,frame,lines,true);
    appendSyncTargetGeometry(replay,frame,fill,false);
    printf("OBSERVER CAMERA REPLAY (no game input): action=%d phase=%s spheres=%zu ranges=%d regions=%zu aim=%d nearest=%d bodyMesh=%zu outline=%zu fill=%zu\n",
        action,frame.phase.c_str(),frame.spheres.size(),frame.ranges.known,frame.regions.size(),frame.aim.known,frame.nearestChoiceKnown,
        frame.aimPartMesh?frame.aimPartMesh->fill.size():0,lines.size(),fill.size());
 }


 CloseHandle(process);return readable?0:5;
}
