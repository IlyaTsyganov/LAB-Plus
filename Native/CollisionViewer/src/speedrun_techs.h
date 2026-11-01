#pragma once

struct SpeedrunTechRow { std::string key,label,state="unavailable",detail; };
struct SpeedrunTechSnapshot { std::vector<SpeedrunTechRow> rows; };
static const GraphField* techField(const GraphLookup& lookup,const std::string& type,const char* name) {
    const auto& schema=lookup(type);
    if(!schema.complete||!schema.size||schema.size>0x200000) return nullptr;
    for(const auto& field:schema.fields) if(field.name==name&&uint64_t(field.offset)+field.size<=schema.size) return &field;
    return nullptr;
}
static bool techPointer(const GraphField* field,const char* type) { return field&&field->size==8&&field->type==type&&field->ops=="*"; }
static bool techFloat(const GraphField* field) { return field&&field->size==4&&field->type=="float"&&field->ops.empty(); }
static bool techTime(const GraphField* field) {
    return field&&field->size==8&&field->ops.empty()&&field->type=="idTypesafeTime < long long , gameTimeUnique_t , 999960 >";
}
static std::string techResourceName(const WallMemory& memory,uintptr_t resource,const GraphLookup& lookup) {
    const auto* name=techField(lookup,"idResource","name"); const auto* str=techField(lookup,"idAtomicString","str");
    uintptr_t text=0;
    if(!resource||!name||name->type!="idAtomicString"||!name->ops.empty()||!str||str->size!=8||str->type!="char"||str->ops!="*"||
       uint64_t(str->offset)+8>name->size||!memory.get(resource+name->offset+str->offset,text)) return {};
    return readEntityText(memory,text,512);
}
static bool techNear(float left,float right) { return std::isfinite(left)&&std::abs(left-right)<=.0001f; }
static bool techInline(const GraphField* field,const char* type,const GraphLookup& lookup) {
    const auto& schema=lookup(type);
    return field&&field->type==type&&field->ops.empty()&&schema.complete&&schema.size&&field->size==schema.size;
}
struct TechBoolBit { uint32_t byte=~0u; unsigned char mask=0; explicit operator bool() const { return byte!=~0u&&mask; } };
static TechBoolBit resolveTechBoolProperty(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                          const GraphLookup& lookup,const std::string& type,const char* name);
static std::string configuredBatsState(float actual,float modScale,float dexterity,float clamp) {
    if(!std::isfinite(actual)||!std::isfinite(modScale)||!std::isfinite(dexterity)||!std::isfinite(clamp)||
       actual<=0||modScale<=0||clamp<0) return "unavailable";
    const float divisor=std::max(std::max(dexterity,std::numeric_limits<float>::epsilon()),clamp);
    const float normal=modScale/divisor;
    if(!std::isfinite(normal)||normal<=0||normal>10000||actual>10000) return "unavailable";
    const float tolerance=std::max(.0001f,normal*.001f);
    if(actual>normal+tolerance) return "active";
    return std::abs(actual-normal)<=tolerance?"inactive":"unavailable";
}
struct ConfiguredBatsCode {
    uint32_t timestamp,imageSize;
    uintptr_t calculation,dexterity,declGetter,status;
    uint64_t calculationHash,dexterityHash,statusHash;
};
static const ConfiguredBatsCode* configuredBatsCode(const GameProfile& profile) {
    static constexpr ConfiguredBatsCode audited[]{
        {0x5e794131,0x1e706000,0x161dbbb,0x18beea0,0x144a360,0x1818fd0,0x42140d67acd3cf3eULL,0x50fd559ab7522032ULL,0xc605486a842437a8ULL},
        {0x5ec7deca,0x1d551000,0x1659efb,0x1911620,0x1c83140,0x1868230,0x89e19155f199a6dfULL,0x2f6c144bb4a46ca5ULL,0x443df4f24e42cd99ULL}
    };
    for(const auto& code:audited) if(code.timestamp==profile.timestamp&&code.imageSize==profile.imageSize) return &code;
    return nullptr;
}
static bool configuredBatsFingerprint(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const ConfiguredBatsCode& code) {
    auto matches=[&](uintptr_t rva,size_t length,uint64_t expected) {
        if(rva>=profile.imageSize||length>profile.imageSize-rva||length>529) return false;
        unsigned char bytes[529]{};if(!memory.read(base+rva,bytes,length)) return false;
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<length;++i) hash=(hash^bytes[i])*1099511628211ULL;
        return hash==expected;
    };
    auto scalar=[&](uintptr_t operand,uintptr_t instructionEnd,float expected) {
        int32_t displacement=0;float value=0;
        if(operand>=profile.imageSize||4>profile.imageSize-operand||!memory.get(base+operand,displacement)) return false;
        const int64_t target=static_cast<int64_t>(instructionEnd)+displacement;
        return target>=0&&uint64_t(target)+4<=profile.imageSize&&memory.get(base+static_cast<uintptr_t>(target),value)&&value==expected;
    };
    return matches(code.calculation,529,code.calculationHash)&&matches(code.dexterity,44,code.dexterityHash)&&
        matches(code.declGetter,5,0x71fa801ff8c90d42ULL)&&matches(code.status,143,code.statusHash)&&
        scalar(code.calculation+5,code.calculation+9,1.f)&&scalar(code.calculation+0xc2,code.calculation+0xc6,.001f)&&
        scalar(code.dexterity+0x1b,code.dexterity+0x1f,std::numeric_limits<float>::epsilon());
}
// Audited SetWeapon calculations:1.0 0x161dbbb..0x161ddcc;
//1.1 0x1659efb..0x165a10c. Exact code fingerprints cover the mode branch,
// normal-rate operations, call targets and the final bring-up rate assignment.
// A zero requested bring-up duration leaves clip rate 1, multiplies the active
// fire-mode override, and divides by the clamped upgrade/status-effect speed.
// The no-status-effects case below computes that normal value, rather than
// comparing all weapons with a Ballista-specific rate learned from a demo.
static SpeedrunTechRow captureConfiguredBats(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                            const LevelIdentity& level,uintptr_t player,const GraphLookup& lookup) {
    SpeedrunTechRow row{"bats","BATS","unavailable","Weapon animation metadata unavailable"};
    const auto* code=configuredBatsCode(profile);
    if(!code) { row.detail="Weapon animation-rate calculation is not yet verified for this build"; return row; }
    if(!configuredBatsFingerprint(memory,base,profile,*code)) { row.detail="Weapon animation-rate code does not match the verified build"; return row; }
    auto hands=techField(lookup,"idPlayer","hands"),right=techField(lookup,"idHands","rightItem"),held=techField(lookup,"idHandsItem","item");
    const std::string managedType="idManagedClassPtr < idInventoryItem >";
    auto ptr=techField(lookup,managedType,"ptr"),token=techField(lookup,managedType,"spawnId");
    auto animator=techField(lookup,"idHands","webAnimator"),up=techField(lookup,"idAnimWebHands","bringUpAnimRate");
    auto next=techField(lookup,"idHands","nextWeaponRight"),queued=techField(lookup,"idHands","queuedNextWeaponRight");
    if(!techInline(hands,"idHands",lookup)||!techInline(right,"idHandsItem",lookup)||!techInline(held,managedType.c_str(),lookup)||
       !techPointer(ptr,"idInventoryItem")||!token||token->size!=4||token->type!="idSpawnId"||!token->ops.empty()||!techInline(animator,"idAnimWebHands",lookup)||!techFloat(up)||
       !techPointer(next,"idWeapon")||!techPointer(queued,"idWeapon")) return row;
    uintptr_t handsAddress=player+hands->offset,heldAddress=handsAddress+right->offset+held->offset,equipped=0,nextWeapon=0,queuedWeapon=0;
    uint32_t heldToken=0; float actual=0;
    if(!memory.get(heldAddress+ptr->offset,equipped)||!equipped||!memory.get(heldAddress+token->offset,heldToken)||
       !memory.get(handsAddress+next->offset,nextWeapon)||!memory.get(handsAddress+queued->offset,queuedWeapon)||
       queuedWeapon||!memory.get(handsAddress+animator->offset+up->offset,actual)) return row;
    const uintptr_t heldItem=equipped;
    auto decl=techField(lookup,"idInventoryItem","decl"),quickSlot=techField(lookup,"idInventoryItem","quickSlot");
    if(!techPointer(decl,"idDeclInventory")||!quickSlot||quickSlot->type!="int"||quickSlot->size!=4||!quickSlot->ops.empty()) return row;
    auto previous=techField(lookup,"idHands","meleeD5PrevWeapon"),flags=techField(lookup,"idHands","handsFlags");
    TechBoolBit meleeBit; unsigned char meleeFlag=0; bool duringMelee=false;
    if(techPointer(previous,"idWeapon")&&techInline(flags,"idHands::handsFlags_t",lookup)) {
        meleeBit=resolveTechBoolProperty(memory,base,profile,lookup,"idHands::handsFlags_t","isMeleeAnimPlaying");
        if(meleeBit&&memory.get(handsAddress+flags->offset+meleeBit.byte,meleeFlag)&&(meleeFlag&meleeBit.mask)) {
            uintptr_t previousWeapon=0;
            auto ordinaryPunch=[&](uintptr_t item) {
                uintptr_t vtable=0,definition=0;
                return item&&memory.get(item,vtable)&&readEntityClass(memory,vtable,base,profile)=="idWeapon"&&
                    memory.get(item+decl->offset,definition)&&techResourceName(memory,definition,lookup)=="weapon/player/fists_doom5melee";
            };
            if(!memory.get(handsAddress+previous->offset,previousWeapon)||!previousWeapon||
               (heldItem!=previousWeapon&&!ordinaryPunch(heldItem))||
               (nextWeapon&&nextWeapon!=previousWeapon&&!ordinaryPunch(nextWeapon))) return row;
            // The engine temporarily equips its ordinary punch item. Compare
            // the live rate against the actual preceding inventory gun, rather
            // than retaining a cached active label through arbitrary failures.
            equipped=previousWeapon; duringMelee=true;
        }
    }
    if(!duringMelee&&nextWeapon&&nextWeapon!=equipped) return row;
    auto inventory=techField(lookup,"idActor","inventory"),items=techField(lookup,"idInventoryCollection","inventory");
    if(!techInline(inventory,"idInventoryCollection",lookup)||!items||items->ops!=""||!techInline(items,items->type.c_str(),lookup)) return row;
    auto list=techField(lookup,items->type,"list"),num=techField(lookup,items->type,"num");
    const auto& managed=lookup(managedType);
    if(!list||list->size!=8||list->ops!="*"||list->type!=managedType||!num||num->size!=4||num->type!="int"||!num->ops.empty()||!managed.complete) return row;
    uintptr_t listAddress=player+inventory->offset+items->offset,data=0; int32_t count=0;
    if(!memory.get(listAddress+list->offset,data)||!memory.get(listAddress+num->offset,count)||count<0||count>1024||(!data&&count)) return row;
    uintptr_t slot=0; uint32_t slotToken=0;
    for(int32_t i=0;i<count;++i) {
        uintptr_t candidate=0,address=data+size_t(i)*managed.size;
        if(memory.get(address+ptr->offset,candidate)&&candidate==equipped&&memory.get(address+token->offset,slotToken)) { slot=address; break; }
    }
    if(!slot) { row.detail="Equipped item is not in the current inventory"; return row; }
    uintptr_t vtable=0;
    if(!memory.get(equipped,vtable)||readEntityClass(memory,vtable,base,profile)!="idWeapon") return row;
    uintptr_t definition=0; int32_t weaponSlot=-1;
    if(!memory.get(equipped+decl->offset,definition)||!definition||!memory.get(equipped+quickSlot->offset,weaponSlot)||weaponSlot<0||weaponSlot>128) return row;
    auto desired=techField(lookup,"idDeclWeapon","desiredBringupDurationMS"),milliseconds=techField(lookup,"milliToGameTime_t","milliseconds");
    if(!techInline(desired,"milliToGameTime_t",lookup)||!milliseconds||milliseconds->size!=4||
       milliseconds->type!="idTypesafeTime < int , millisecondUnique_t , 1000 >"||!milliseconds->ops.empty()) return row;
    int32_t desiredMS=-1;
    if(!memory.get(definition+desired->offset+milliseconds->offset,desiredMS)) return row;
    if(desiredMS!=0) { row.detail="This weapon's non-default bring-up timing is not yet supported"; return row; }
    auto mods=techField(lookup,"idWeapon","weaponModOverrides"),fireMode=techField(lookup,"idWeapon","fireMode");
    const std::string modType="idWeapon::weaponModOverrides_t"; const auto& modSchema=lookup(modType);
    auto scale=techField(lookup,modType,"bringUpRateScale");
    if(!mods||mods->type!=modType||mods->ops!="[2]"||!modSchema.complete||!modSchema.size||mods->size!=modSchema.size*2||
       !techFloat(scale)||!fireMode||fireMode->size!=4||fireMode->type!="weaponFireMode_t"||!fireMode->ops.empty()) return row;
    int32_t mode=-1; float modScale=0,otherScale=0;
    if(!memory.get(equipped+fireMode->offset,mode)||mode<0||mode>1||
       !memory.get(equipped+mods->offset+size_t(mode)*modSchema.size+scale->offset,modScale)||
       !memory.get(equipped+mods->offset+size_t(1-mode)*modSchema.size+scale->offset,otherScale)||!techNear(modScale,otherScale)) return row;
    auto props=techField(lookup,"idPlayer","playerProps"),clamping=techField(lookup,"idDeclPlayerProps","playerSpeedClamping"),clampField=techField(lookup,"idDeclPlayerProps::playerSpeedClamping_t","weaponSwitchClampScale");
    auto dashboard=techField(lookup,"idPlayer","dashboard"),abilities=techField(lookup,"idPlayerDashboard","abilityUpgrades"),dexterity=techField(lookup,"idAbility_UpgradeHandler","dexterityWeaponSwitching");
    auto dexterityMods=techField(lookup,"idUpgradeMod_Abilities","dexterityMods"),speedField=techField(lookup,"idUpgradeMod_Abilities::dexterityMods_t","weaponChangeSpeed");
    if(!techPointer(props,"idDeclPlayerProps")||!techInline(clamping,"idDeclPlayerProps::playerSpeedClamping_t",lookup)||!techFloat(clampField)||
       !techInline(dashboard,"idPlayerDashboard",lookup)||!techInline(abilities,"idAbility_UpgradeHandler",lookup)||!techPointer(dexterity,"idUpgradeMod_Abilities")||
       !techInline(dexterityMods,"idUpgradeMod_Abilities::dexterityMods_t",lookup)||!techFloat(speedField)) return row;
    uintptr_t playerProps=0,upgrade=0; float clamp=0,speed=1;
    if(!memory.get(player+props->offset,playerProps)||!playerProps||!memory.get(playerProps+clamping->offset+clampField->offset,clamp)||
       !memory.get(player+dashboard->offset+abilities->offset+dexterity->offset,upgrade)||
       (upgrade&&!memory.get(upgrade+dexterityMods->offset+speedField->offset,speed))) return row;
    auto status=techField(lookup,"idActor","statusEffectComponent"),effects=techField(lookup,"idStatusEffectComponent","activeStatusEffects");
    auto effectCount=techField(lookup,"idList < idStatusEffect * , TAG_IDLIST , false >","num");
    int32_t effectsCount=-1;
    if(!techInline(status,"idStatusEffectComponent",lookup)||!effects||effects->type!="idStaticList < idStatusEffect * , 10 , false , TAG_IDLIST >"||
       !effects->ops.empty()||!effectCount||effectCount->size!=4||effectCount->type!="int"||!effectCount->ops.empty()||effectCount->offset+4>effects->size||
       !memory.get(player+status->offset+effects->offset+effectCount->offset,effectsCount)) return row;
    if(effectsCount!=0) { row.detail="Temporary status-effect animation multipliers are not yet supported"; return row; }
    // Check the audited property's exact displacement and the configuration
    // branch. An alternate switch-rate mode uses a different normal formula.
    unsigned char load[7]{}; uintptr_t config=0; int32_t relative=0,rateMode=-1; uint32_t nativeOffset=0;
    if(!memory.read(base+code->calculation+9,load,7)||load[0]!=0x48||load[1]!=0x8b||load[2]!=0x0d) return row;
    memcpy(&relative,load+3,4);
    const uintptr_t configGlobal=base+code->calculation+16+relative;
    if(configGlobal<base||configGlobal+8>base+profile.imageSize||!memory.get(configGlobal,config)||!config||!memory.get(config+8,rateMode)||rateMode!=0||
       !memory.get(base+code->calculation+0x129,nativeOffset)||nativeOffset!=desired->offset+milliseconds->offset||
       !memory.get(base+code->calculation+0x16a,nativeOffset)||nativeOffset!=mods->offset+scale->offset) return row;
    uintptr_t finalEquipped=0,finalItem=0,finalData=0,system=0,map=0,currentPlayer=0,finalNext=0,finalQueued=0; uint32_t finalHeldToken=0,finalSlotToken=0; int32_t finalCount=0; unsigned char removed=1;
    if(!memory.get(heldAddress+ptr->offset,finalEquipped)||finalEquipped!=heldItem||!memory.get(heldAddress+token->offset,finalHeldToken)||finalHeldToken!=heldToken||
       !memory.get(handsAddress+next->offset,finalNext)||finalNext!=nextWeapon||!memory.get(handsAddress+queued->offset,finalQueued)||finalQueued!=queuedWeapon||
       !memory.get(slot+ptr->offset,finalItem)||finalItem!=equipped||!memory.get(slot+token->offset,finalSlotToken)||finalSlotToken!=slotToken||
       !memory.get(listAddress+list->offset,finalData)||finalData!=data||!memory.get(listAddress+num->offset,finalCount)||finalCount!=count||
       !memory.get(base+profile.gameSystemRva,system)||!memory.get(system+0x50,map)||map!=level.map||
       !memory.get(map+profile.mapPlayerOffset,currentPlayer)||currentPlayer!=player||!memory.get(player+0xa4,removed)||removed) return row;
    if(duringMelee) {
        uintptr_t finalPrevious=0; unsigned char finalFlag=0;
        if(!memory.get(handsAddress+previous->offset,finalPrevious)||finalPrevious!=equipped||
           !memory.get(handsAddress+flags->offset+meleeBit.byte,finalFlag)||(finalFlag&meleeBit.mask)==0) return row;
    }
    row.state=configuredBatsState(actual,modScale,speed,clamp);
    row.detail=row.state=="active"?"Weapon raise speed exceeds its current normal configuration":
        row.state=="inactive"?"Weapon raise speed matches its current normal configuration":"Weapon animation has not reached a supported state";
    return row;
}
static bool fastDestroyerBladeArmed(int64_t once,int64_t nominal) { return once>0&&nominal>once&&nominal<=999960LL*60; }
static TechBoolBit decodeTechBoolGetter(const unsigned char* code,size_t length,uint32_t size) {
    size_t at=0; int32_t byte=-1;
    if(length>=3&&code[0]==0x0f&&code[1]==0xb6&&code[2]==0x01) { byte=0; at=3; }
    else if(length>=4&&code[0]==0x0f&&code[1]==0xb6&&code[2]==0x41) { byte=static_cast<int8_t>(code[3]); at=4; }
    else if(length>=7&&code[0]==0x0f&&code[1]==0xb6&&code[2]==0x81) { memcpy(&byte,code+3,4); at=7; }
    if(byte<0||uint32_t(byte)>=size) return {};
    unsigned shift=0; bool masked=false;
    if(at+2<=length&&code[at]==0xd1&&code[at+1]==0xe8) { shift=1; at+=2; }
    else if(at+3<=length&&code[at]==0xc1&&code[at+1]==0xe8) { shift=code[at+2]; at+=3; }
    if(shift>7) return {};
    if(at+3<=length&&code[at]==0x83&&code[at+1]==0xe0&&code[at+2]==1) { masked=true; at+=3; }
    if(at>=length||code[at]!=0xc3||(!masked&&shift!=7)) return {};
    return {static_cast<uint32_t>(byte),static_cast<unsigned char>(1u<<shift)};
}
static TechBoolBit resolveTechBoolProperty(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                          const GraphLookup& lookup,const std::string& type,const char* name) {
    const auto& schema=lookup(type);
    if(!schema.complete||!schema.size||schema.size>0x200000) return {};
    for(const auto& property:schema.properties) if(property.name==name&&property.type=="bool"&&property.ops.empty()) {
        if(property.getter<base||property.getter-base>=profile.imageSize||16>profile.imageSize-(property.getter-base)) return {};
        unsigned char code[16]{};
        if(!memory.read(property.getter,code,sizeof(code))) return {};
        return decodeTechBoolGetter(code,sizeof(code),schema.size);
    }
    return {};
}
struct FastBladeIdentity {
    uintptr_t map=0,world=0,player=0,weapon=0; uint32_t token=0;
    bool operator==(const FastBladeIdentity& other) const { return map==other.map&&world==other.world&&player==other.player&&weapon==other.weapon&&token==other.token; }
};
struct FastBladeObservation {
    FastBladeIdentity identity;
    std::string module;
    int64_t once=-2,nominal=-2,afterHit=-2,secondaryShot=-1;
    int32_t chargeState=-1;
    bool held=false,flagKnown=false,interruptedModChange=false;
};
class FastBladeTracker {
    FastBladeIdentity identity;
    bool valid=false,armed=false;
    int64_t once=-2,shot=-1;
public:
    void clear() { valid=false; armed=false; once=-2; shot=-1; identity={}; }
    SpeedrunTechRow observe(const FastBladeObservation& current) {
        SpeedrunTechRow row{"fast_dblade","Fast DBlade","unavailable","Next-charge state unavailable"};
        if(!valid||!(identity==current.identity)) { clear(); identity=current.identity; valid=true; }
        if(current.once!=once||current.secondaryShot!=shot) armed=false;
        once=current.once; shot=current.secondaryShot;
        if(once==-1||once==0) { armed=false; row.state="inactive"; row.detail="No prepared next-charge override"; return row; }
        if(once<0||once>999960LL*60) { armed=false; return row; }
        const bool destroyer=current.module=="weapon/player/gauss_rifle_destroyer";
        const bool arbalest=current.module=="weapon/player/gauss_rifle_ballista";
        if(destroyer&&fastDestroyerBladeArmed(once,current.nominal)) armed=true;
        // Ordinary mastered Arbalest also receives a one-second override during
        // its automatic recharge. It is NOT Fast DBlade. In both controlled
        // prepared hits, the engine still had modChangeAnimPlaying set while
        // the actual hands node was firing/recharging Arbalest. Preserve that
        // engine proof while the same one-shot override survives in inventory.
        if(arbalest&&once==999960&&current.afterHit==once&&current.held&&current.flagKnown&&current.interruptedModChange&&
           (current.chargeState==1||current.chargeState==2||current.chargeState==3||current.chargeState==5)) armed=true;
        if(armed) { row.state="active"; row.detail="Prepared Arbalest hit accelerated the next Destroyer Blade charge"; return row; }
        if(arbalest&&current.held&&current.flagKnown&&!current.interruptedModChange&&current.chargeState!=0) {
            row.state="inactive"; row.detail="Ordinary Arbalest mastery recharge";
        } else if(arbalest) row.detail="Prepared-hit activation was not observed";
        else if(destroyer) { row.state="inactive"; row.detail="Next Destroyer Blade charge has normal duration"; }
        return row;
    }
};
static FastBladeTracker& fastBladeTracker() { static auto* tracker=new FastBladeTracker; return *tracker; }
static std::mutex& fastBladeTrackerMutex() { static auto* mutex=new std::mutex; return *mutex; }
static SpeedrunTechSnapshot captureSpeedrunTechs(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                               const LevelIdentity& level,const GraphLookup& lookup) {
    SpeedrunTechSnapshot result; result.rows={{"bats","BATS","unavailable","Weapon animation metadata unavailable"},
        {"fast_dblade","Fast DBlade","unavailable","Weapon charge metadata unavailable"}};
    auto& bats=result.rows[0]; auto& blade=result.rows[1];
    uintptr_t system=0,map=0,player=0,playerVtable=0; unsigned char removed=1;
    if(!level||!memory.get(base+profile.gameSystemRva,system)||!memory.get(system+0x50,map)||map!=level.map||
       !memory.get(map+profile.mapPlayerOffset,player)||!player||!memory.get(player,playerVtable)||playerVtable!=base+profile.playerVtableRva||
       !memory.get(player+0xa4,removed)||removed) return result;
    bats=captureConfiguredBats(memory,base,profile,level,player,lookup);
    auto inventory=techField(lookup,"idActor","inventory");
    auto items=techField(lookup,"idInventoryCollection","inventory");
    const auto& inventorySchema=lookup("idInventoryCollection");
    const std::string managedType="idManagedClassPtr < idInventoryItem >";
    auto ptr=techField(lookup,managedType,"ptr"),token=techField(lookup,managedType,"spawnId");
    const auto& managed=lookup(managedType);
    auto decl=techField(lookup,"idInventoryItem","decl");
    auto mods=techField(lookup,"idWeapon","weaponModOverrides");
    const std::string modType="idWeapon::weaponModOverrides_t";
    const auto& modSchema=lookup(modType);
    auto modDecl=techField(lookup,modType,"weaponDecl"),once=techField(lookup,modType,"chargeGameTimeOnce"),nominal=techField(lookup,modType,"chargeGameTime");
    if(!inventory||inventory->type!="idInventoryCollection"||!inventory->ops.empty()||!inventorySchema.complete||inventory->size!=inventorySchema.size||!items||items->size!=24||!items->ops.empty()||
       !managed.complete||!managed.size||managed.size>128||!techPointer(ptr,"idInventoryItem")||!token||token->size!=4||
       !techPointer(decl,"idDeclInventory")||!mods||mods->type!=modType||mods->ops!="[2]"||!modSchema.complete||
       !modSchema.size||mods->size!=modSchema.size*2||!techPointer(modDecl,"idDeclWeapon")||!techTime(once)||!techTime(nominal)) return result;
    const auto& listSchema=lookup(items->type);
    auto list=techField(lookup,items->type,"list"),num=techField(lookup,items->type,"num");
    if(!listSchema.complete||listSchema.size!=items->size||!list||list->size!=8||list->ops!="*"||list->type!=managedType||
       !num||num->size!=4||num->type!="int"||!num->ops.empty()) return result;
    uintptr_t listAddress=player+inventory->offset+items->offset,data=0; int32_t count=0;
    if(!memory.get(listAddress+list->offset,data)||!memory.get(listAddress+num->offset,count)||count<0||count>1024||(!data&&count)) return result;
    uintptr_t ballista=0,ballistaLink=0,ballistaDecl=0; uint32_t ballistaToken=0;
    bool unavailable=false;
    for(int32_t i=0;i<count;++i) {
        uintptr_t link=data+size_t(i)*managed.size,item=0,definition=0,vtable=0; uint32_t itemToken=0;
        if(!memory.get(link+ptr->offset,item)||!memory.get(link+token->offset,itemToken)) { unavailable=true; continue; }
        if(!item) continue;
        if(!memory.get(item,vtable)) { unavailable=true; continue; }
        auto className=readEntityClass(memory,vtable,base,profile);
        if(className.empty()) { unavailable=true; continue; }
        if(className!="idWeapon") continue;
        if(!memory.get(item+decl->offset,definition)) { unavailable=true; continue; }
        auto resourceName=techResourceName(memory,definition,lookup);
        if(resourceName.empty()) { unavailable=true; continue; }
        if(resourceName!="weapon/player/gauss_rifle") continue;
        uintptr_t finalItem=0; uint32_t finalToken=0;
        if(!memory.get(link+ptr->offset,finalItem)||finalItem!=item||!memory.get(link+token->offset,finalToken)||finalToken!=itemToken) { unavailable=true; continue; }
        ballista=item; ballistaLink=link; ballistaToken=itemToken; ballistaDecl=definition; break;
    }
    if(!ballista) {
        if(!unavailable) { blade.state="inactive"; blade.detail="Ballista is not in the current inventory"; std::lock_guard<std::mutex> lock(fastBladeTrackerMutex()); fastBladeTracker().clear(); }
        return result;
    }
    FastBladeObservation observed; observed.identity={level.map,level.world,player,ballista,ballistaToken};
    bool observedValid=false;
    uintptr_t secondary=ballista+mods->offset+modSchema.size,definition=0;
    auto afterHit=techField(lookup,modType,"chargeGameTimeAfterDirectHit"),lastFire=techField(lookup,"idWeapon","lastFireTime");
    auto charge=techField(lookup,"idWeapon","chargeParms"),chargeState=techField(lookup,"idWeapon::chargeParms_t","chargeState");
    const std::string times="idArray < idTypesafeTime < long long , gameTimeUnique_t , 999960 > , 2 >";
    if(techTime(afterHit)&&lastFire&&lastFire->type==times&&lastFire->size==16&&lastFire->ops.empty()&&
       techInline(charge,"idWeapon::chargeParms_t",lookup)&&chargeState&&chargeState->type=="idWeapon::chargeState_t"&&chargeState->size==4&&chargeState->ops.empty()&&
       memory.get(secondary+modDecl->offset,definition)&&memory.get(secondary+once->offset,observed.once)&&memory.get(secondary+nominal->offset,observed.nominal)&&
       memory.get(secondary+afterHit->offset,observed.afterHit)&&memory.get(ballista+lastFire->offset+8,observed.secondaryShot)&&
       memory.get(ballista+charge->offset+chargeState->offset,observed.chargeState)) {
        observed.module=techResourceName(memory,definition,lookup); observedValid=true;
        auto hands=techField(lookup,"idPlayer","hands"),right=techField(lookup,"idHands","rightItem"),held=techField(lookup,"idHandsItem","item");
        auto flags=techField(lookup,"idHands","handsFlags");
        if(techInline(hands,"idHands",lookup)&&techInline(right,"idHandsItem",lookup)&&techInline(held,managedType.c_str(),lookup)&&
           techInline(flags,"idHands::handsFlags_t",lookup)) {
            auto bit=resolveTechBoolProperty(memory,base,profile,lookup,"idHands::handsFlags_t","modChangeAnimPlaying");
            uintptr_t heldAddress=player+hands->offset+right->offset+held->offset,equipped=0,finalEquipped=0;
            uint32_t heldToken=0,finalHeldToken=0; unsigned char flag=0;
            if(bit&&memory.get(heldAddress+ptr->offset,equipped)&&memory.get(heldAddress+token->offset,heldToken)&&
               memory.get(player+hands->offset+flags->offset+bit.byte,flag)&&memory.get(heldAddress+ptr->offset,finalEquipped)&&
               equipped==finalEquipped&&memory.get(heldAddress+token->offset,finalHeldToken)&&heldToken==finalHeldToken) {
                observed.flagKnown=true; observed.held=equipped==ballista; observed.interruptedModChange=(flag&bit.mask)!=0;
            }
        }
    }
    uintptr_t finalData=0,finalMap=0,finalPlayer=0,finalBallista=0,finalDecl=0; int32_t finalCount=0; uint32_t finalToken=0;
    if(!memory.get(system+0x50,finalMap)||finalMap!=map||!memory.get(map+profile.mapPlayerOffset,finalPlayer)||finalPlayer!=player||
       !memory.get(listAddress+list->offset,finalData)||finalData!=data||!memory.get(listAddress+num->offset,finalCount)||finalCount!=count||
       !memory.get(ballistaLink+ptr->offset,finalBallista)||finalBallista!=ballista||!memory.get(ballistaLink+token->offset,finalToken)||finalToken!=ballistaToken||
       !memory.get(ballista+decl->offset,finalDecl)||finalDecl!=ballistaDecl) {
        for(auto& row:result.rows) { row.state="unavailable"; row.detail="Player inventory changed during observation"; }
        std::lock_guard<std::mutex> lock(fastBladeTrackerMutex()); fastBladeTracker().clear();
        return result;
    }
    if(observedValid) {
        int64_t finalOnce=-2,finalShot=-2; int32_t finalState=-1; uintptr_t finalDefinition=0;
        if(memory.get(secondary+once->offset,finalOnce)&&finalOnce==observed.once&&memory.get(ballista+lastFire->offset+8,finalShot)&&finalShot==observed.secondaryShot&&
           memory.get(ballista+charge->offset+chargeState->offset,finalState)&&finalState==observed.chargeState&&
           memory.get(secondary+modDecl->offset,finalDefinition)&&finalDefinition==definition) {
            std::lock_guard<std::mutex> lock(fastBladeTrackerMutex()); blade=fastBladeTracker().observe(observed);
        }
    }
    return result;
}
