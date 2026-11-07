#pragma once

static bool syncRangeCvar(const WallMemory& memory,uintptr_t at,const char* expected,float& value) {
    uintptr_t data=0,name=0;
    return memory.get(at,data)&&data&&memory.get(data+40,name)&&readEntityText(memory,name,128)==expected&&
        memory.get(data+12,value)&&std::isfinite(value);
}
static bool syncRangeMonsterLimit(const WallMemory& memory,uintptr_t at,uint64_t monster,float& maximum,bool& matched) {
    SyncPreviewList list,after;if(!syncPreviewList(memory,at,64,list))return false;matched=false;
    for(int i=0;i<list.count;++i){uint64_t types=0;float value=0;
        if(!memory.get(list.data+size_t(i)*24,types)||!memory.get(list.data+size_t(i)*24+12,value))return false;
        if(types&monster){if(!std::isfinite(value)||value<=0||value>200)return false;maximum=value;matched=true;break;}}
    return memory.get(at,after)&&!memcmp(&list,&after,sizeof(list));
}
static SyncRangeLimits readSyncRangeLimits(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                           const GraphLookup& lookup,uintptr_t player,const EntityReference& target,
                                           const SyncFacingPose& pose,Vec3 viewPosition,Vec3 viewTarget) {
    SyncRangeLimits result;
    if(!syncPreviewProfile(profile)||!syncPreviewFinite(viewPosition)||!syncPreviewFinite(viewTarget))return result;
    uintptr_t vtable=0,monsterGetter=0,behavior=0;uint64_t monster=1;
    unsigned char expectedMonster[]{0x48,0x8b,0x81,0xf0,0x56,0,0,0x48,0x85,0xc0,0x74,8,0x48,0x8b,0x80,0xf0,1,0,0,0xc3};
    const uint32_t monsterField=uint32_t(syncPatch().monsterField),monsterValue=uint32_t(syncPatch().monsterValue);memcpy(expectedMonster+3,&monsterField,4);memcpy(expectedMonster+15,&monsterValue,4);
    unsigned char code[sizeof(expectedMonster)]{};
    if(!memory.get(target.address,vtable)||!memory.get(vtable+syncPatch().monsterSlot,monsterGetter)||monsterGetter!=base+syncPatch().monsterGetter||
       !memory.read(monsterGetter,code,sizeof(code))||memcmp(code,expectedMonster,sizeof(code))||
       !memory.get(target.address+syncPatch().monsterField,behavior)||(behavior&&!memory.get(behavior+syncPatch().monsterValue,monster))||!monster)return result;
    // Bound or offset models use another native transform branch. Do not
    // silently interpret a camera-space radius as a feet-space radius there.
    for(uintptr_t entity:{player,target.address}) {
        uintptr_t vt=0,getter=0;unsigned char bound=1,rotated=1;
        unsigned char bind[]{0x48,0x8d,0x81,0x20,0x1e,0,0,0xc3};unsigned char actual[8]{};const uint32_t bindField=uint32_t(syncPatch().bindField);memcpy(bind+3,&bindField,4);
        if(!memory.get(entity,vt)||!memory.get(vt+syncPatch().bindSlot,getter)||!memory.read(getter,actual,8)||memcmp(actual,bind,8)||
           !memory.get(entity+syncPatch().bindField+0x1db,bound)||!memory.get(entity+syncPatch().bindField+0x1dd,rotated)||bound||rotated) {result.detail="Bound model range unavailable";return result;}
    }
    Vec3 modelOffset{},playerOffset{};uintptr_t physics=0;
    if(!memory.get(player+syncPlayerOffset(0x1d0),physics)||physics!=player+syncPlayerOffset(0x7d28)||!memory.get(physics+0xb0,result.playerOrigin)||
       !memory.get(player+syncPatchOffset("idActor",0x1c30),modelOffset)||!memory.get(player+syncPlayerOffset(0x377c8),playerOffset)||
       modelOffset.x!=0||modelOffset.y!=0||modelOffset.z!=0||playerOffset.x!=0||playerOffset.y!=0||playerOffset.z!=0)return result;
    result.eyeOffset=subtract(viewPosition,result.playerOrigin);const auto offset=subtract(viewTarget,pose.origin);
    result.viewTargetOffset={dot(offset,{pose.axis[0],pose.axis[1],pose.axis[2]}),dot(offset,{pose.axis[3],pose.axis[4],pose.axis[5]}),dot(offset,{pose.axis[6],pose.axis[7],pose.axis[8]})};
    if(!syncPreviewFinite(result.eyeOffset)||dot(result.eyeOffset,result.eyeOffset)>16||dot(offset,offset)>100)return result;
    uintptr_t aim=0,weapon=0,decl=0,overrideDecl=0;uint32_t token=0,finalToken=0;int32_t mode=-1;
    const uintptr_t held=syncPlayerOffset(0xc5a0)+syncPatchOffset("idHands",0x2988)+0x30;
    if(!memory.get(player+syncPlayerOffset(0x70e8)+syncPatchOffset("idSyncAttack",0x210),aim))return result;
    if(!aim) {
        if(!memory.get(player+held+8,weapon)||!weapon||!memory.get(player+held,token)||!memory.get(weapon,vtable)||
           !comboChainWeaponClass(readEntityClass(memory,vtable,base,profile),lookup)||!memory.get(weapon+0x38,decl)||!decl||
           !memory.get(weapon+syncPatchOffset("idWeapon",0x19d0),mode)||mode<0||mode>1)return result;
        // Original 1.1 GetFireModeDecl: supercharge, then mod override, then
        // secondary default, then the base definition. Only two fire modes.
        if(!memory.get(weapon+syncPatchOffset("idWeapon",0x120)+size_t(mode)*0x38,overrideDecl))return result;
        if(!overrideDecl&&!memory.get(weapon+syncPatchOffset("idWeapon",0x3318)+size_t(mode)*0x3d8,overrideDecl))return result;
        if(overrideDecl)decl=overrideDecl;
        else if(mode==1){if(!memory.get(decl+syncPatchOffset("idDeclWeapon",0x748),overrideDecl))return result;if(overrideDecl)decl=overrideDecl;}
        if(!memory.get(decl+syncPatchOffset("idDeclWeapon",0x1f40),aim)||!aim)return result;
    }
    bool meleeEntry=false,chainsawEntry=false;
    // Native lunge validation starts with lungeMaxDistance and optionally
    // overrides it using the first matching monster row. A missing Chainsaw
    // row must never disable GK preview for a monster that cannot be sawn.
    if(!memory.get(aim+0x19c,result.lunge)||!std::isfinite(result.lunge)||result.lunge<0||result.lunge>200||
       !syncRangeMonsterLimit(memory,aim+0x1a8,monster,result.lunge,meleeEntry)||result.lunge<=0||
       !syncRangeMonsterLimit(memory,aim+0x1c0,monster,result.chainsawTarget,chainsawEntry))return result;
    float modifier=0,maxView=0,ignore=0;
    if(!memory.get(player+syncPlayerOffset(0x3b000),modifier)||!std::isfinite(modifier)||fabsf(modifier)>100||
       !syncRangeCvar(memory,base+syncPatch().maxDistance,"viewInfo_maxMeleeDistance",maxView)||maxView<=0||maxView>200||
       !syncRangeCvar(memory,base+syncPatch().ignoreChainsawLunge,"sync_quickChainsawIgnoreLungeDistValidation",ignore)||(ignore!=0&&ignore!=1))return result;
    result.gloryKillTarget=std::min(maxView,modifier>0?12.f:result.lunge+modifier);
    result.modifier=modifier;
    result.chainsawTarget=std::min(maxView,result.chainsawTarget);result.ignoreQuickLunge=ignore!=0;
    uintptr_t charge=0;if(!memory.get(player+syncPlayerOffset(0x3b078),charge))return result;
    if(charge){unsigned char active=2;if(!memory.get(charge+0x18a,active)||active){result.detail="Charge attack range unavailable";return result;}}
    if(weapon) {uintptr_t finalWeapon=0;if(!memory.get(player+held+8,finalWeapon)||finalWeapon!=weapon||!memory.get(player+held,finalToken)||finalToken!=token)return result;}
    result.known=result.gloryKillTarget>0;result.detail=result.known?"Native range and facing limits":"Range data unavailable";return result;
}
