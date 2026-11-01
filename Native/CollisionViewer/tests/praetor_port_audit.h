#pragma once

// The declaration constructor is audited as data, so an invented zero-argument
// test fixture cannot silently disagree with retail again. ASLR/call addresses
// and the registered event index are the only normalized operands.
static bool auditSyncReleaseRegistration(const WallMemory& memory,uintptr_t base,const GameProfile& profile) {
    struct Site {uint32_t timestamp,size;uintptr_t rva;};
    static constexpr Site sites[]{
        {0x5eeb0267, 0x1d396000, 0x8832640},
        {0x5f203d59, 0x1e1ef000, 0x1af95a40},
        {0x5f8065da, 0x1c587000, 0x833b0f0},
        {0x5f90c129, 0x1e0c1000, 0x1b8bd140},
        {0x5fb68f01, 0x1c7e9000, 0x84d3000},
        {0x5fc9da94, 0x1c2eb000, 0x8678000},
        {0x603896f3, 0x1c5bf000, 0x8886340},
        {0x60c98dd6, 0x1cb22000, 0x1a32a530},
        {0x60dff532, 0x1bc55000, 0x117aac00},
        {0x60f26b2f, 0x1bb06000, 0x83072f0},
        {0x6116d6b5, 0x1cd60000, 0x1ade3cf0},
        {0x61202c76, 0x1d77e000, 0x1aca41f0},
        {0x5e794131,0x1e706000,0x1c283d40},{0x5ec7deca,0x1d551000,0x88fdaa0},{0x607e69b0,0x1b5dd000,0x19485e70},
        {0x61692c83,0x1e4cd000,0x8b20400},{0x618dfc01,0x1c835000,0x881f760},{0x61d7704e,0x1c58a000,0x877b5f0},
        {0x6256614c,0x1e69d000,0x8a3cbc0},{0x64cc8a9b,0x7421000,0x297910},{0x669f0dcf,0x744d000,0x2970f0},{0x6a7b9b8c,0x7431000,0x29b560}
    };
    uintptr_t rva=0;for(const auto& site:sites)if(site.timestamp==profile.timestamp&&site.size==profile.imageSize)rva=site.rva;
    unsigned char code[121]{};if(!rva||rva>=profile.imageSize||sizeof(code)>profile.imageSize-rva||!memory.read(base+rva,code,sizeof(code)))return false;
    const auto hash=batsCodeHash(code,sizeof(code),{18,29,40,52,69,80,91,107,112});
    if(hash!=0x10e6b03967d590faULL&&hash!=0xaa8ffd742ab0d732ULL)return false;
    auto text=[&](size_t operand,const char* expected) {
        int32_t displacement=0;memcpy(&displacement,code+operand,4);const int64_t target=int64_t(rva+operand+4)+displacement;
        return target>=0&&uint64_t(target)<profile.imageSize&&readEntityText(memory,base+uintptr_t(target),128)==expected;
    };
    return text(29,"ae_releasePlayerFromSync")&&text(80,"idMD6Anim*;")&&text(69,"anim;")&&text(91,"4")&&text(52,";");
}

// Exercise the complete interaction -> model -> animation-event -> HUD path
// with the retail executable's REAL reflected layouts and synthetic objects.
// No methods are invoked and no game process is involved.
static void auditPraetorPort(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const GraphLookup& lookup,int releaseNumber) {
    std::deque<std::vector<unsigned char>> buffers;
    auto make=[&](const std::string& type) {
        const auto& schema=lookup(type);requirePort(schema.complete&&schema.size&&schema.size<=0x200000,"Praetor object schema missing");
        buffers.emplace_back(schema.size,0);return reinterpret_cast<uintptr_t>(buffers.back().data());
    };
    auto field=[&](const std::string& type,const char* name)->const GraphField& {
        const auto* value=timerField(lookup,type,name);
        if(!value) {printf("Missing Praetor field: %s.%s\n",type.c_str(),name);throw std::runtime_error("Praetor field missing");}
        return *value;
    };
    auto put=[&](uintptr_t object,const std::string& type,const char* name,auto value) {
        const auto& member=field(type,name);requirePort(sizeof(value)<=member.size,"Praetor fixture write exceeds field");portPut(object+member.offset,value);
    };
    auto managed=[&](uintptr_t object,const std::string& type,const char* name,uintptr_t target) {
        const auto& member=field(type,name);put(object+member.offset,member.type,"ptr",target);put(object+member.offset,member.type,"spawnId",uint32_t(17));
    };
    auto list=[&](uintptr_t object,const std::string& type,const char* name,uintptr_t data,int count) {
        const auto& member=field(type,name);put(object+member.offset,member.type,"list",data);put(object+member.offset,member.type,"num",int32_t(count));
    };
    const auto player=make("idPlayer"),sync=make("idSyncEntity"),web=make("idDeclAnimWeb"),state=make("idAnimator_AnimWeb::idModelAnimState"),
        model=make("idDeclAnimWeb::idModelAnimInfo"),md6=make("idDeclMD6"),info=make("idAnimEvents::animEventInfo_t"),
        event=make("idMD6AnimEvent"),leaf=make("idMD6LeafPlay"),animation=make("idMD6Anim"),data=make("idMD6AnimData");
    const char* animationName="md6/player/human/base/motion/mechanics/interact/preator_suit_token/preator_suit_token_sync.md6anim";
    const auto& resourceName=field("idResource","name");put(animation+resourceName.offset,"idAtomicString","str",reinterpret_cast<uintptr_t>(animationName));
    put(animation,"idMD6Anim","animData",data);put(data,"idMD6AnimData","numFrames",uint16_t(200));put(data,"idMD6AnimData","frameRate",uint16_t(30));
    managed(player,"idPlayer","savedSyncEntity",sync);managed(player,"idPlayer","syncMaster",sync);
    managed(sync,"idSyncEntity","syncPlayer",player);put(sync,"idSyncEntity","isCurrentlyWorking",uint8_t(1));put(sync,"idSyncEntity","isInteractableSync",uint8_t(1));
    put(sync,"idSyncEntity","currentSyncEntityAnim",animation);
    const auto& stack=field("idAnimatedEntity","animStack");put(sync+stack.offset,"idAnimStack","pauseTime",int64_t(-1));
    const auto& animator=field("idSyncEntity","animatorSyncEntity");list(sync+animator.offset,"idAnimator_AnimWeb","modelAnimStates",state,1);
    put(sync+animator.offset,"idAnimator_AnimWeb","decl",web);list(web,"idDeclAnimWeb","models",model,1);
    put(state,"idAnimator_AnimWeb::idModelAnimState","tree",leaf);put(model,"idDeclAnimWeb::idModelAnimInfo","model",md6);
    const auto& animEvents=field("idDeclMD6","animEvents");list(md6+animEvents.offset,"idAnimEvents","list",info,1);
    const auto& animRef=field("idAnimEvents::animEventInfo_t","animRef");uint32_t refStringOffset=0;
    requirePort(syncAnimationRefLayout(lookup,&animRef,refStringOffset),"Retail animation reference layout rejected");
    portPut(info+animRef.offset+refStringOffset,reinterpret_cast<uintptr_t>(animationName));
    list(info,"idAnimEvents::animEventInfo_t","events",event,1);put(event,"idMD6AnimEvent","frameNum",94.f);put(event,"idMD6AnimEvent","eventNum",int16_t(releaseNumber));
    put(leaf,"idMD6Node","type",uint8_t(2));put(leaf,"idMD6Leaf","anim",animation);put(leaf,"idMD6Leaf","wrapMode",uint8_t(0));put(leaf,"idMD6Leaf","lastTotalWeight",1.f);
    put(leaf,"idMD6LeafPlay","startTime",int64_t(0));put(leaf,"idMD6LeafPlay","startTimeFraction",-1.f);put(leaf,"idMD6LeafPlay","rateScale",1.f);
    put(leaf,"idMD6LeafPlay","lastTicksPerSec",int64_t(999960));const auto& scales=field("idMD6LeafPlay","gameRateScales");
    const float scaleValues[]{1,1,1};requirePort(scales.size==sizeof(scaleValues),"Retail scale field size mismatch");memcpy(reinterpret_cast<void*>(leaf+scales.offset),scaleValues,sizeof(scaleValues));
    GameTimerClock clock;clock.valid=true;clock.ticks=0;
    requirePort(syncPlayerLockProfile(&profile),"Player-owned interaction route rejected");
    auto direct=captureSyncPlayerControlLock(memory,player,sync,clock,lookup,releaseNumber,base,&profile);
    if(direct.state!="active")printf("Praetor direct: %s / %s\n",direct.state.c_str(),direct.detail.c_str());
    requirePort(direct.state=="active"&&fabs(direct.remaining-94./30)<.00001,"Retail layouts cannot produce release timer");
    auto rows=captureSyncAnimationTimers(memory,player,clock,lookup,base,&profile);
    const auto found=std::find_if(rows.begin(),rows.end(),[](const GameTimerRow& row){return row.key=="interaction"&&row.label=="Player lock";});
    if(found==rows.end())for(const auto& row:rows)printf("Praetor routed: %s %s %s\n",row.key.c_str(),row.state.c_str(),row.detail.c_str());
    requirePort(found!=rows.end()&&found->state=="active"&&fabs(found->remaining-94./30)<.00001,"Production interaction routing lost the release timer");
    const std::vector<InvisibleTimerRow> visible{{found->key,found->group,found->label,found->state,found->detail,found->remaining,true,found->maximum}};
    const auto lines=timerHudLines(visible);requirePort(lines.size()==2&&lines[1].find("Player lock")!=std::string::npos,"Release timer was hidden by HUD filtering");
    // A Gore Nest's NPC clip lasts 232/30s, but its actual release is at
    // frame81. Exercise the generic route, rather than a token-name whitelist.
    const char* goreName="md6/player/human/base/motion/mechanics/interact/gore_nest/gore_nest_sync11.md6anim";
    put(animation+resourceName.offset,"idAtomicString","str",reinterpret_cast<uintptr_t>(goreName));
    portPut(info+animRef.offset+refStringOffset,reinterpret_cast<uintptr_t>(goreName));
    put(data,"idMD6AnimData","numFrames",uint16_t(233));put(event,"idMD6AnimEvent","frameNum",81.f);
    auto goreRows=captureSyncAnimationTimers(memory,player,clock,lookup,base,&profile);
    const auto gore=std::find_if(goreRows.begin(),goreRows.end(),[](const GameTimerRow& row){return row.key=="interaction";});
    requirePort(gore!=goreRows.end()&&gore->label=="Player lock"&&gore->state=="active"&&fabs(gore->remaining-2.7)<.00001&&fabs(gore->maximum-2.7)<.00001,
        "Gore Nest was assigned the NPC clip instead of its player-release boundary");
    const char* goldenName="md6/player/human/base/motion/mechanics/interact/golden_gore_nest/golden_gore_nest_sync11.md6anim";
    put(animation+resourceName.offset,"idAtomicString","str",reinterpret_cast<uintptr_t>(goldenName));
    portPut(info+animRef.offset+refStringOffset,reinterpret_cast<uintptr_t>(goldenName));
    put(data,"idMD6AnimData","numFrames",uint16_t(83));put(event,"idMD6AnimEvent","frameNum",65.f);
    auto goldenRows=captureSyncAnimationTimers(memory,player,clock,lookup,base,&profile);
    const auto golden=std::find_if(goldenRows.begin(),goldenRows.end(),[](const GameTimerRow& row){return row.key=="interaction";});
    requirePort(golden!=goldenRows.end()&&golden->state=="active"&&fabs(golden->remaining-65./30)<.00001,
        "Golden Gore Nest borrowed the ordinary Gore Nest duration instead of its own release frame");
    // The algorithm is authored-event driven for any player-owned interaction.
    const char* otherName="test/another_player_owned_interaction";
    put(animation+resourceName.offset,"idAtomicString","str",reinterpret_cast<uintptr_t>(otherName));
    portPut(info+animRef.offset+refStringOffset,reinterpret_cast<uintptr_t>(otherName));put(event,"idMD6AnimEvent","frameNum",45.f);
    auto otherRows=captureSyncAnimationTimers(memory,player,clock,lookup,base,&profile);
    const auto other=std::find_if(otherRows.begin(),otherRows.end(),[](const GameTimerRow& row){return row.key=="interaction";});
    requirePort(other!=otherRows.end()&&other->state=="active"&&fabs(other->remaining-1.5)<.00001,"Unlisted interaction borrowed a token/Gore Nest constant or NPC clip");
    put(sync,"idSyncEntity","isInteractableSync",uint8_t(0));
    auto scriptedRows=captureSyncAnimationTimers(memory,player,clock,lookup,base,&profile);
    const auto scripted=std::find_if(scriptedRows.begin(),scriptedRows.end(),[](const GameTimerRow& row){return row.key=="sync_other_1";});
    requirePort(scripted!=scriptedRows.end()&&scripted->label=="Player lock"&&scripted->state=="active"&&fabs(scripted->remaining-1.5)<.00001,
        "Script-started non-melee sync reverted to its NPC clip duration");
    put(sync,"idSyncEntity","isInteractableSync",uint8_t(1));
    put(animation+resourceName.offset,"idAtomicString","str",reinterpret_cast<uintptr_t>(animationName));
    portPut(info+animRef.offset+refStringOffset,reinterpret_cast<uintptr_t>(animationName));put(data,"idMD6AnimData","numFrames",uint16_t(200));put(event,"idMD6AnimEvent","frameNum",94.f);
    clock.ticks=999960;auto advanced=captureSyncPlayerControlLock(memory,player,sync,clock,lookup,releaseNumber,base,&profile);
    requirePort(advanced.state=="active"&&fabs(advanced.remaining-(94./30-1))<.00001,"Release timer does not follow engine clock");
    put(sync+stack.offset,"idAnimStack","pauseTime",int64_t(999960));clock.ticks=9999600;
    auto paused=captureSyncPlayerControlLock(memory,player,sync,clock,lookup,releaseNumber,base,&profile);
    requirePort(paused.state=="active"&&paused.remaining==advanced.remaining,"Paused release timer drifted with global clock");
    put(sync+stack.offset,"idAnimStack","pauseTime",int64_t(-1));clock.ticks=0;
    const auto* disabled=timerField(lookup,"idMD6AnimEvent","disabled");
    if(disabled)portPut(event+disabled->offset,uint8_t(1));else put(event,"idMD6AnimEvent","flags",uint32_t(2));
    requirePort(captureSyncPlayerControlLock(memory,player,sync,clock,lookup,releaseNumber,base,&profile).state=="unavailable","Disabled event produced a timer");
    if(disabled)portPut(event+disabled->offset,uint8_t(0));else put(event,"idMD6AnimEvent","flags",uint32_t(0));
    managed(player,"idPlayer","savedSyncEntity",0);managed(player,"idPlayer","syncMaster",0);
    requirePort(captureSyncPlayerControlLock(memory,player,sync,clock,lookup,releaseNumber,base,&profile).state=="inactive","NPC clip retained timer after player release");
    printf("PASS interactions %s: real reflection; Praetor/Gore Nest/unlisted event routes; disabled, pause and ownership controls\n",profile.name);
}
