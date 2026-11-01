#include "../src/vulkan_layer.cpp"
static void expectSync(bool valid,const char* message) { if(!valid) { std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1); } }
template<class T> static void syncPut(uintptr_t at,size_t offset,T value) { std::memcpy(reinterpret_cast<void*>(at+offset),&value,sizeof(value)); }
static const GameTimerRow& syncRow(const std::vector<GameTimerRow>& rows,const std::string& key) {
    for(const auto& row:rows)if(row.key==key)return row;
    std::fprintf(stderr,"Missing row: %s\n",key.c_str());std::exit(1);
}
int main() {
    std::unordered_map<std::string,GraphType> schemas;
    auto schema=[&](const std::string& name,uint32_t size,std::initializer_list<GraphField> fields) { schemas[name]={size,"",fields,true}; };
    const std::string time="idTypesafeTime < long long , gameTimeUnique_t , 999960 >",syncType="idManagedClassPtr < idSyncEntity >",
        listType="idList < idManagedClassPtr < idSyncEntity > , TAG_IDLIST , false >";
    for(const char* target:{"idSyncEntity","idPlayer","idInteractable","idEntity","idAnimatedEntity"})
        schema(std::string("idManagedClassPtr < ")+target+" >",32,{{"ptr",target,"*",8,8},{"spawnId","idSpawnId","",0,4}});
    schema("idPlayer",1024,{{"syncMaster",syncType,"",0,32},{"syncMelee","idSyncAttack","",64,64},{"playerMechanicSync","idPlayerMechanicSync","",128,24},
        {"savedSyncEntity",syncType,"",200,32},{"mostRecentSyncEntity",syncType,"",232,32},{"lastUsedInteractable","idManagedClassPtr < idInteractable >","",264,32}});
    schema("idSyncAttack",64,{{"currentSyncEntity",syncType,"",0,32}});
    schema("idPlayerMechanicSync",24,{{"syncAnimEntities",listType,"",0,24}});
    schema(listType,24,{{"list",syncType,"*",0,8},{"num","int","",8,4}});
    schema("idSyncEntity",1024,{{"syncPlayer","idManagedClassPtr < idPlayer >","",256,32},{"isCurrentlyWorking","bool","",296,1},
        {"currentSyncEntityAnim","idMD6Anim","*",304,8},{"isInteractableSync","bool","",312,1}});
    schema("idInteractable",1024,{{"interaction","idInteractionComponent","",256,160}});
    schema("idInteractionComponent",160,{{"activeSyncEntity",syncType,"",0,32},{"playerActivator","idManagedClassPtr < idEntity >","",32,32},
        {"ownerEntity","idManagedClassPtr < idAnimatedEntity >","",64,32}});
    schema("idAnimatedEntity",1024,{{"animStack","idAnimStack","",320,64}});
    schema("idAnimStack",64,{{"tree","idMD6Node","*",0,8},{"pauseTime",time,"",8,8},{"useClientLocalTime","bool","",16,1}});
    schema("idMD6Node",24,{{"type","unsigned char","",0,1}});
    schema("idMD6Branch",64,{{"left","idMD6Node","*",8,8},{"right","idMD6Node","*",16,8},{"leftTimeOverride",time,"",24,8},{"rightTimeOverride",time,"",32,8}});
    schema("idMD6Leaf",24,{{"anim","idMD6Anim","*",8,8},{"wrapMode","unsigned char","",16,1},{"lastTotalWeight","float","",20,4}});
    schema("idMD6LeafPlay",96,{{"startTime",time,"",24,8},{"gameRateScales","idArray < float , 3 >","",32,12},{"rateScale","float","",44,4}});
    schema("idMD6Anim",32,{{"animData","idMD6AnimData","*",0,8}});
    schema("idMD6AnimData",16,{{"numFrames","unsigned short","",0,2},{"frameRate","unsigned short","",2,2}});
    schema("idResource",32,{{"name","idAtomicString","",8,8}});schema("idAtomicString",8,{{"str","char","*",0,8}});
    GraphLookup lookup=[&](const std::string& name)->const GraphType& { const auto found=schemas.find(name);if(found!=schemas.end())return found->second;static const GraphType empty;return empty; };
    std::array<unsigned char,1024> playerBytes{},meleeBytes{},interactionBytes{},targetBytes{};
    std::array<unsigned char,96> meleeLeaf{},interactionLeaf{};std::array<unsigned char,32> animation{};std::array<unsigned char,16> animationData{};
    const auto player=reinterpret_cast<uintptr_t>(playerBytes.data()),melee=reinterpret_cast<uintptr_t>(meleeBytes.data()),interaction=reinterpret_cast<uintptr_t>(interactionBytes.data()),
        target=reinterpret_cast<uintptr_t>(targetBytes.data()),anim=reinterpret_cast<uintptr_t>(animation.data()),data=reinterpret_cast<uintptr_t>(animationData.data());
    const int64_t second=999960;
    auto managed=[&](uintptr_t address,uintptr_t entity,uint32_t token=7) { syncPut(address,0,token);syncPut(address,8,entity); };
    auto init=[&](uintptr_t entity,uintptr_t leaf,bool isInteraction) {
        managed(entity+256,player);syncPut(entity,296,uint8_t(1));syncPut(entity,304,anim);syncPut(entity,312,uint8_t(isInteraction));syncPut(entity,320,leaf);
        syncPut(leaf,0,uint8_t(2));syncPut(leaf,8,anim);syncPut(leaf,16,uint8_t(0));syncPut(leaf,20,1.f);syncPut(leaf,24,9*second);
        for(unsigned index=0;index<3;++index)syncPut(leaf,32+index*4,1.f);syncPut(leaf,44,1.f);
    };
    init(melee,reinterpret_cast<uintptr_t>(meleeLeaf.data()),false);init(interaction,reinterpret_cast<uintptr_t>(interactionLeaf.data()),true);
    syncPut(anim,0,data);syncPut(anim,8,reinterpret_cast<uintptr_t>("sync/test"));syncPut(data,0,uint16_t(121));syncPut(data,2,uint16_t(60));
    GameTimerClock clock;clock.valid=true;clock.ticks=10*second;
    managed(player,melee);managed(player+64,melee);
    auto rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);
    expectSync(syncRow(rows,"sync_animation").state=="active"&&syncRow(rows,"sync_animation").remaining==1&&syncRow(rows,"sync_animation").maximum==2,"melee clip uses actual deadline/rate and maximum");
    expectSync(syncRow(rows,"interaction").state=="inactive","ordinary glory kill is never copied into the interaction timer");
    managed(player,0);managed(player+64,0);managed(player+264,target);managed(target+256,interaction);managed(target+288,player);managed(target+320,target);
    rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);
    expectSync(syncRow(rows,"interaction").state=="active"&&syncRow(rows,"interaction").remaining==1&&syncRow(rows,"interaction").maximum==2,"interactable component supplies its active sync even with empty player/melee lists");
    auto ordinary11=captureSyncAnimationTimers(WallMemory{},player,clock,lookup,0,&kProfile11);
    expectSync(syncRow(ordinary11,"interaction").state=="active"&&syncRow(ordinary11,"interaction").label=="Interaction (current clip)","other 1.1 interactions retain their existing clip countdown without requiring a Praetor release timeline");
    syncPut(anim,8,reinterpret_cast<uintptr_t>("md6/player/human/base/motion/mechanics/interact/preator_suit_token/preator_suit_token_sync11.md6anim"));
    expectSync(syncPraetorPlayerLock(WallMemory{},interaction,lookup,&kProfile11)&&!syncPraetorPlayerLock(WallMemory{},interaction,lookup,&kProfile10),"new lock semantics select only the verified Praetor asset and game profile");
    syncPut(anim,8,reinterpret_cast<uintptr_t>("sync/test"));
    expectSync(syncRow(rows,"sync_animation").state=="inactive","button/pickup interaction is not mislabeled as glory kill");
    managed(player,interaction);managed(player+200,interaction);managed(player+232,interaction);
    std::array<unsigned char,32> extraRef{};managed(reinterpret_cast<uintptr_t>(extraRef.data()),interaction);syncPut(player,128,reinterpret_cast<uintptr_t>(extraRef.data()));syncPut(player,136,int32_t(1));
    rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);
    expectSync(std::count_if(rows.begin(),rows.end(),[](const auto& row){return row.state=="active";})==1,"same active interaction reached through several engine routes has one countdown");
    managed(player+64,melee);rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);
    expectSync(syncRow(rows,"interaction").state=="active"&&syncRow(rows,"sync_animation").state=="active","simultaneous distinct syncs retain separate timelines");
    syncPut(interaction,328,9*second+second/2);rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);
    expectSync(syncRow(rows,"interaction").remaining==1.5,"paused sync uses its stack pause clock");syncPut(interaction,328,int64_t(0));
    syncPut(interaction,336,uint8_t(1));rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);
    expectSync(syncRow(rows,"interaction").state=="unavailable"&&syncRow(rows,"sync_animation").state=="active","unsupported local animation clock does not invent time or hide other syncs");syncPut(interaction,336,uint8_t(0));
    syncPut(reinterpret_cast<uintptr_t>(interactionLeaf.data()),16,uint8_t(1));rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);
    expectSync(syncRow(rows,"interaction").state=="unavailable","looped interaction is not given a fabricated end");syncPut(reinterpret_cast<uintptr_t>(interactionLeaf.data()),16,uint8_t(0));
    managed(interaction+256,player+1);rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);
    expectSync(syncRow(rows,"interaction").state=="inactive","another player's sync is excluded");managed(interaction+256,player);
    syncPut(interaction,296,uint8_t(0));rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);
    expectSync(syncRow(rows,"interaction").state=="inactive","finished sync in stale recent references does not remain active");syncPut(interaction,296,uint8_t(1));
    managed(player,0);managed(player+64,0);managed(player+200,0);managed(player+232,0);syncPut(player,136,int32_t(0));managed(target+288,player+1);
    rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);expectSync(syncRow(rows,"interaction").state=="inactive","component activated by a different player is excluded");managed(target+288,player);
    syncPut(target,0xa4,uint8_t(1));rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);expectSync(syncRow(rows,"interaction").state=="inactive","removed last-used interactable does not revive old animation");syncPut(target,0xa4,uint8_t(0));
    auto saved=schemas["idInteractionComponent"];schemas.erase("idInteractionComponent");managed(player+64,melee);rows=captureSyncAnimationTimers(WallMemory{},player,clock,lookup);
    expectSync(syncRow(rows,"sync_animation").state=="active"&&syncRow(rows,"interaction").state=="unavailable","missing optional interaction metadata leaves supported melee independent");schemas["idInteractionComponent"]=saved;
    // Real observed Praetor boundary: the authored player release occurs on
    // frame94, while all three models' raw clips contain200frames.
    {
        float scale[3]{1,1,1};int64_t deadline=0;
        expectSync(syncReleaseDeadline(200,30,94.f,22000876,-1.f,scale,1.f,999960,deadline)&&deadline==25134084,"release deadline uses the event frame rather than the longer NPC clip");
        expectSync(deadline<25136123&&25136123-deadline<999960/30,"recorded engine release dispatch follows the authored deadline within one game update");
        expectSync(!syncReleaseDeadline(200,30,94.f,22000876,0.f,scale,1.f,999960,deadline),"fractional-start path remains unavailable until its separate clock is audited");
        expectSync(!syncReleaseDeadline(200,30,94.f,22000876,-1.f,scale,1.f,1000,deadline)&&!syncReleaseDeadline(200,30,94.f,22000876,-1.f,scale,1.f,1999920,deadline),"foreign leaf tick domains cannot be compared with the fixed999960 game clock");
        expectSync(!syncReleaseDeadline(200,30,200.f,22000876,-1.f,scale,1.f,999960,deadline)&&!syncReleaseDeadline(200,30,94.f,22000876,-1.f,scale,0.f,999960,deadline),"invalid frame and stopped playback cannot invent a release");
        schema("idEventDef",96,{{"name","char","*",0,8},{"numargs","int","",44,4},{"eventnum","int","",52,4},{"type","eventType_t","",60,4}});
        auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,kProfile11.imageSize,MEM_RESERVE,PAGE_READWRITE));expectSync(image!=nullptr,"reserve sparse release-registry fixture");
        const auto imageBase=reinterpret_cast<uintptr_t>(image);
        auto commit=[&](size_t offset,size_t length) { const size_t first=offset&~size_t(4095),end=(offset+length+4095)&~size_t(4095);expectSync(VirtualAlloc(image+first,end-first,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit sparse release-registry fixture page"); };
        constexpr size_t getter=0x44f870,table=0x3d85c70,definition=0x6220240,dummy=definition+128,text=0x2b260c8;
        commit(getter,31);commit(table,4096*8);commit(definition,256);commit(text,128);
        unsigned char code[]{0x48,0x83,0xec,0x28,0x81,0xfa,0xff,0x0f,0,0,0x77,0x13,0x48,0x63,0xc2,0x48,0x8d,0x0d,0,0,0,0,0x48,0x8b,0x04,0xc1,0x48,0x83,0xc4,0x28,0xc3};
        const int32_t relative=static_cast<int32_t>(table-getter-22);memcpy(code+18,&relative,4);memcpy(image+getter,code,sizeof(code));
        strcpy_s(reinterpret_cast<char*>(image+text),64,"ae_releasePlayerFromSync");strcpy_s(reinterpret_cast<char*>(image+text+64),64,"unrelated");
        syncPut(imageBase+definition,0,imageBase+text);syncPut(imageBase+definition,44,int32_t(0));syncPut(imageBase+definition,52,int32_t(691));syncPut(imageBase+definition,60,int32_t(2));
        syncPut(imageBase+dummy,0,imageBase+text+64);
        for(unsigned index=0;index<692;++index)syncPut(imageBase+table,index*8,imageBase+(index==691?definition:dummy));
        expectSync(syncReleaseEventNumber(WallMemory{},imageBase,kProfile11,lookup)==691,"event lookup discovers the named release event through the verified engine registry");
        image[getter+22]=0;expectSync(syncReleaseEventNumber(WallMemory{},imageBase,kProfile11,lookup)<0,"changed event getter invalidates cached bindings");image[getter+22]=code[22];
        syncPut(imageBase+definition,60,int32_t(0));expectSync(syncReleaseEventNumber(WallMemory{},imageBase,kProfile11,lookup)<0,"non-animation event cannot masquerade as player release");syncPut(imageBase+definition,60,int32_t(2));
        syncPut(imageBase+definition,52,int32_t(198));expectSync(syncReleaseEventNumber(WallMemory{},imageBase,kProfile11,lookup)<0,"registry index and definition number must agree");syncPut(imageBase+definition,52,int32_t(691));
        expectSync(!syncPlayerLockProfile(&kProfile10)&&!syncPlayerLockProfile(&kProfile51)&&syncPlayerLockProfile(&kProfile11),"only the validated per-model player lock profile changes its display semantics");
        VirtualFree(image,0,MEM_RELEASE);
    }
    {
        const std::string stateType="idList < idAnimator_AnimWeb::idModelAnimState , TAG_ANIMWEB , false >",modelInfo="idDeclAnimWeb::idModelAnimInfo",
            modelList="idList < idDeclAnimWeb::idModelAnimInfo , TAG_DECL_ANIMWEB , false >",infoType="idAnimEvents::animEventInfo_t",
            infoList="idList < idAnimEvents::animEventInfo_t , TAG_MD6_ANIMEVENTS , false >",eventList="idList < idMD6AnimEvent , TAG_MD6_ANIMEVENTS , false >";
        schemas["idSyncEntity"].fields.push_back({"animatorSyncEntity","idAnimator_SyncEntity","",400,128});
        schema("idAnimator_SyncEntity",128,{});schema("idAnimator_AnimWeb",128,{{"modelAnimStates",stateType,"",0,24},{"decl","idDeclAnimWeb","*",24,8}});
        schema(stateType,24,{{"list","idAnimator_AnimWeb::idModelAnimState","*",0,8},{"num","int","",8,4}});
        schema("idAnimator_AnimWeb::idModelAnimState",16,{{"tree","idMD6Node","*",0,8}});
        schema("idDeclAnimWeb",32,{{"models",modelList,"",0,24}});schema(modelList,24,{{"list",modelInfo,"*",0,8},{"num","int","",8,4}});
        schema(modelInfo,8,{{"model","idDeclMD6","*",0,8}});schema("idDeclMD6",32,{{"animEvents","idAnimEvents","",0,32}});
        schema("idAnimEvents",32,{{"list",infoList,"",0,24}});schema(infoList,24,{{"list",infoType,"*",0,8},{"num","int","",8,4}});
        schema(infoType,32,{{"animRef","idAtomicStringT < md6AnimAtomicString_t >","",0,8},{"events",eventList,"",8,24}});
        schema(eventList,24,{{"list","idMD6AnimEvent","*",0,8},{"num","int","",8,4}});
        schema("idMD6AnimEvent",88,{{"flags","animEventFlags_t","",40,4},{"frameNum","float","",52,4},{"eventNum","short","",56,2},{"disabled","bool","",83,1}});
        schemas["idMD6LeafPlay"].fields.push_back({"startTimeFraction","float","",48,4});schemas["idMD6LeafPlay"].fields.push_back({"lastTicksPerSec",time,"",56,8});
        std::array<unsigned char,32> web{},modelStates{},npcDefinition{},playerDefinition{},npcInfo{},playerInfo{},playerAnimation{};
        std::array<unsigned char,16> modelDefinitions{},playerAnimationData{};std::array<unsigned char,96> playerLeaf{};
        std::array<unsigned char,88> npcEvent{},playerEvent{};
        const auto webAddress=reinterpret_cast<uintptr_t>(web.data()),statesAddress=reinterpret_cast<uintptr_t>(modelStates.data()),declsAddress=reinterpret_cast<uintptr_t>(modelDefinitions.data()),
            playerLeafAddress=reinterpret_cast<uintptr_t>(playerLeaf.data()),playerAnimationAddress=reinterpret_cast<uintptr_t>(playerAnimation.data()),playerDataAddress=reinterpret_cast<uintptr_t>(playerAnimationData.data()),
            playerEventAddress=reinterpret_cast<uintptr_t>(playerEvent.data());
        managed(player+200,interaction);managed(player,0);syncPut(interaction,400,statesAddress);syncPut(interaction,408,int32_t(2));syncPut(interaction,424,webAddress);
        syncPut(webAddress,0,declsAddress);syncPut(webAddress,8,int32_t(2));
        syncPut(statesAddress,0,reinterpret_cast<uintptr_t>(interactionLeaf.data()));syncPut(statesAddress,16,playerLeafAddress);
        syncPut(declsAddress,0,reinterpret_cast<uintptr_t>(npcDefinition.data()));syncPut(declsAddress,8,reinterpret_cast<uintptr_t>(playerDefinition.data()));
        syncPut(reinterpret_cast<uintptr_t>(npcDefinition.data()),0,reinterpret_cast<uintptr_t>(npcInfo.data()));syncPut(reinterpret_cast<uintptr_t>(npcDefinition.data()),8,int32_t(1));
        syncPut(reinterpret_cast<uintptr_t>(playerDefinition.data()),0,reinterpret_cast<uintptr_t>(playerInfo.data()));syncPut(reinterpret_cast<uintptr_t>(playerDefinition.data()),8,int32_t(1));
        syncPut(reinterpret_cast<uintptr_t>(npcInfo.data()),0,reinterpret_cast<uintptr_t>("sync/test"));syncPut(reinterpret_cast<uintptr_t>(npcInfo.data()),8,reinterpret_cast<uintptr_t>(npcEvent.data()));syncPut(reinterpret_cast<uintptr_t>(npcInfo.data()),16,int32_t(1));
        syncPut(reinterpret_cast<uintptr_t>(playerInfo.data()),0,reinterpret_cast<uintptr_t>("actual_player_channel"));syncPut(reinterpret_cast<uintptr_t>(playerInfo.data()),8,playerEventAddress);syncPut(reinterpret_cast<uintptr_t>(playerInfo.data()),16,int32_t(1));
        syncPut(reinterpret_cast<uintptr_t>(npcEvent.data()),52,94.f);syncPut(reinterpret_cast<uintptr_t>(npcEvent.data()),56,int16_t(198));
        syncPut(playerEventAddress,52,94.f);syncPut(playerEventAddress,56,int16_t(691));
        syncPut(playerAnimationAddress,0,playerDataAddress);syncPut(playerAnimationAddress,8,reinterpret_cast<uintptr_t>("actual_player_channel"));syncPut(playerDataAddress,0,uint16_t(200));syncPut(playerDataAddress,2,uint16_t(30));
        init(interaction,reinterpret_cast<uintptr_t>(interactionLeaf.data()),true);syncPut(playerLeafAddress,0,uint8_t(2));syncPut(playerLeafAddress,8,playerAnimationAddress);syncPut(playerLeafAddress,20,1.f);
        syncPut(playerLeafAddress,24,int64_t(22000876));for(unsigned index=0;index<3;++index)syncPut(playerLeafAddress,32+index*4,1.f);syncPut(playerLeafAddress,44,1.f);syncPut(playerLeafAddress,48,-1.f);syncPut(playerLeafAddress,56,int64_t(999960));
        // The NPC model also has a longer200-frame clip, but its unrelated
        // disable-clip event94 cannot define when player control returns.
        syncPut(reinterpret_cast<uintptr_t>(interactionLeaf.data()),48,-1.f);syncPut(reinterpret_cast<uintptr_t>(interactionLeaf.data()),56,int64_t(999960));
        syncPut(data,0,uint16_t(200));clock.ticks=22000876;
        auto lock=captureSyncPlayerControlLock(WallMemory{},player,interaction,clock,lookup,691);
        expectSync(lock.state=="active"&&fabs(lock.remaining-94./30)<.00001&&fabs(lock.maximum-94./30)<.00001,"pre-trigger player-model release declaration gives3.13s instead of6.63s NPC clip end");
        clock.ticks=24952174;lock=captureSyncPlayerControlLock(WallMemory{},player,interaction,clock,lookup,691);
        expectSync(lock.state=="active"&&fabs(lock.remaining-(25134084.-24952174)/999960)<.00001,"recorded pre-release clock replays through the production lock reader");
        managed(player+200,0);clock.ticks=25148326;lock=captureSyncPlayerControlLock(WallMemory{},player,interaction,clock,lookup,691);
        expectSync(lock.state=="inactive"&&lock.remaining==0&&interactionBytes[296]==1,"actual player release hides countdown while NPC sync keeps working");managed(player+200,interaction);
        syncPut(playerEventAddress,56,int16_t(198));lock=captureSyncPlayerControlLock(WallMemory{},player,interaction,clock,lookup,691);expectSync(lock.state=="unavailable","correlated NPC disable-clip event is not treated as player release");syncPut(playerEventAddress,56,int16_t(691));
        syncPut(playerEventAddress,83,uint8_t(1));lock=captureSyncPlayerControlLock(WallMemory{},player,interaction,clock,lookup,691);expectSync(lock.state=="unavailable","disabled release event does not create a countdown");syncPut(playerEventAddress,83,uint8_t(0));
        syncPut(playerLeafAddress,48,.5f);lock=captureSyncPlayerControlLock(WallMemory{},player,interaction,clock,lookup,691);expectSync(lock.state=="unavailable","unsupported fractional start stays explicit");syncPut(playerLeafAddress,48,-1.f);
        syncPut(interaction,336,uint8_t(1));lock=captureSyncPlayerControlLock(WallMemory{},player,interaction,clock,lookup,691);expectSync(lock.state=="unavailable","local animation clock cannot supply global player-release time");syncPut(interaction,336,uint8_t(0));
        std::vector<unsigned char> unrelatedNpcInfos(10000*32),largePlayerInfos(10000*32);
        const auto npcInfosAddress=reinterpret_cast<uintptr_t>(unrelatedNpcInfos.data()),playerInfosAddress=reinterpret_cast<uintptr_t>(largePlayerInfos.data());
        for(unsigned index=0;index<10000;++index) {
            syncPut(npcInfosAddress,size_t(index)*32,reinterpret_cast<uintptr_t>("unrelated_clip"));
            syncPut(playerInfosAddress,size_t(index)*32,reinterpret_cast<uintptr_t>("unrelated_clip"));
        }
        syncPut(playerInfosAddress,size_t(9999)*32,reinterpret_cast<uintptr_t>("actual_player_channel"));
        syncPut(playerInfosAddress,size_t(9999)*32+8,playerEventAddress);syncPut(playerInfosAddress,size_t(9999)*32+16,int32_t(1));
        syncPut(reinterpret_cast<uintptr_t>(npcDefinition.data()),0,npcInfosAddress);syncPut(reinterpret_cast<uintptr_t>(npcDefinition.data()),8,int32_t(10000));
        syncPut(reinterpret_cast<uintptr_t>(playerDefinition.data()),0,playerInfosAddress);syncPut(reinterpret_cast<uintptr_t>(playerDefinition.data()),8,int32_t(10000));
        clock.ticks=22000876;lock=captureSyncPlayerControlLock(WallMemory{},player,interaction,clock,lookup,691);
        expectSync(lock.state=="unavailable"&&lock.detail.find("budget")!=std::string::npos,"all per-leaf declaration scans share one budget instead of allowing an unbounded leaf-times-info product");
    }
    std::puts("PASS: independent player-owned interaction/melee sync routes, component ownership, deduplication, separate timelines, exact current clips, pause/local/loop guards and stale references.");
}
