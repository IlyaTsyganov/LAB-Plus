#pragma once

struct SpeedrunTechRow { std::string key,label,state="unavailable",detail; bool verified=false; };
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
    uint32_t calculationSize=529,desiredOperand=0x129,scaleOperand=0x16a;
    uint32_t firstMillisecondsOperand=0x64,secondMillisecondsOperand=0xc2;
    uint32_t dexterityEpsilonOperand=0x1b,dexterityOneOperand=0x27;
};
static const ConfiguredBatsCode* configuredBatsCode(const GameProfile& profile) {
    static constexpr ConfiguredBatsCode audited[]{
        {0x5eeb0267, 0x1d396000, 0x16878cb, 0x1949310, 0x19c0b70, 0x189e540, 0x4c8a03b60a45d2cdULL, 0xdeccf3af8456230ULL, 0x7846c4c3a62faaa2ULL},
        {0x5f203d59, 0x1e1ef000, 0x168a15b, 0x194b8e0, 0x14ac8f0, 0x18a0740, 0x65597895c945ee69ULL, 0xcea1ff976ce5007ULL, 0xc2c8f373c96c779ULL},
        {0x5f8065da, 0x1c587000, 0x1773ceb, 0x1a8ea30, 0x146da80, 0x19e0a30, 0xdabaa8727c4e155ULL, 0x656ec0ee011be13cULL, 0x2e2f6e50dcd2c6ddULL},
        {0x5f90c129, 0x1e0c1000, 0x1774c0b, 0x1a8f120, 0xb5ef60, 0x19e1200, 0x9e294321652e74efULL, 0x6e4c9c86b76e1fcfULL, 0xbc2181ff35913cb5ULL},
        {0x5fb68f01, 0x1c7e9000, 0x178a14b, 0x1a25780, 0x1e32da0, 0x1976ec0, 0x39c721cbd3dc3426ULL, 0x3aac2de21193ae2dULL, 0x76f167b59b176ad2ULL},
        {0x5fc9da94, 0x1c2eb000, 0x178a09b, 0x1a24e30, 0x121e9e0, 0x1976190, 0x2aec7edce4860074ULL, 0xa720d13656809c1cULL, 0x9b187622e6c09af3ULL},
        {0x603896f3, 0x1c5bf000, 0x1815c2b, 0x1ac7df0, 0x462fe0, 0x1a10610, 0x53ab753bb84a4d8ULL, 0xbf9f0d1d8afbc334ULL, 0xa7032f0fe770853fULL},
        {0x60c98dd6, 0x1cb22000, 0x18f6fd0, 0x1bbf1d0, 0x4512c0, 0x1b00500, 0x1d0b54e850a14175ULL, 0xe398328045ec1723ULL, 0x1fe4149fc8d9ee3dULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x60dff532, 0x1bc55000, 0x18f6fd0, 0x1bbf1d0, 0x4512c0, 0x1b00500, 0xb2c3135c71e87de5ULL, 0xe398328045ec1723ULL, 0x1fe4149fc8d9ee3dULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x60f26b2f, 0x1bb06000, 0x18f73a0, 0x1bbf860, 0x4512c0, 0x1b00b80, 0x18f06b5949e85a73ULL, 0x3e89ca6ae239337fULL, 0x5141a10e139a5a2cULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x6116d6b5, 0x1cd60000, 0x18f7300, 0x1bbf7c0, 0x4512c0, 0x1b00ae0, 0x8b40138ef1448c8bULL, 0x29d05704b56e4b7fULL, 0xb1f7a4af7acd7ec3ULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x61202c76, 0x1d77e000, 0x18f72c0, 0x1bbf780, 0x4512c0, 0x1b00aa0, 0xedf551dfe68bb1ULL, 0xd0376b424ae6f758ULL, 0x405da12fc0cf1503ULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x5e794131,0x1e706000,0x161dbbb,0x18beea0,0x144a360,0x1818fd0,0x42140d67acd3cf3eULL,0x50fd559ab7522032ULL,0xc605486a842437a8ULL},
        {0x5ec7deca,0x1d551000,0x1659efb,0x1911620,0x1c83140,0x1868230,0x89e19155f199a6dfULL,0x2f6c144bb4a46ca5ULL,0x443df4f24e42cd99ULL},
        {0x607e69b0,0x1b5dd000,0x181377b,0x1ac6bc0,0x162c5b0,0x1a0f5b0,0xd06fb2ac87bdc8eeULL,0x56d24858d3fc4114ULL,0x43560282b09a104fULL},
        {0x6a7b9b8c, 0x7431000, 0x136b950, 0x163ec30, 0x5cea80, 0x156f760, 0xd002696aac5d4e82ULL, 0xc408482b5874127cULL, 0xf32520f9f7f4f7ccULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x61692c83, 0x1e4cd000, 0x19a9a40, 0x1c83af0, 0x4625b0, 0x1bbbbc0, 0x6d62e93584ca31b6ULL, 0x722329fca09e94f0ULL, 0xd07c69e9b8c2f559ULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x618dfc01, 0x1c835000, 0x19c3d70, 0x1c9dff0, 0x462670, 0x1bd60b0, 0xa24e64582d99a48dULL, 0xeb9bee9624c3b658ULL, 0x560b73440a9cb5e6ULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x61d7704e, 0x1c58a000, 0x19c3ff0, 0x1c9f200, 0x462860, 0x1bd6330, 0x3ee1a0675a9750b5ULL, 0xb82f22327f47211cULL, 0x2e5f682d71fa654cULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x6256614c, 0x1e69d000, 0x19e5e20, 0x1ccae80, 0x464240, 0x1bfc3e0, 0x25b69b2e8d39535fULL, 0xf7943f8087f9ff5fULL, 0x9d6252ba2bc70601ULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x64cc8a9b, 0x7421000, 0x19dc530, 0x1caa210, 0x463d80, 0x1bdba90, 0x417ea443f8d36cf4ULL, 0x6f8511de452bb67ULL, 0x7e2a1b2a2deac059ULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24},
        {0x669f0dcf, 0x744d000, 0x1361e90, 0x1633100, 0x5c9d00, 0x1563dc0, 0x56592253010caebaULL, 0x8eb507bf2c758cadULL, 0xf3505429daef38edULL, 516, 0x120, 0x161, 0x5c, 0xba, 0x18, 0x24}
    };
    for(const auto& code:audited) if(code.timestamp==profile.timestamp&&code.imageSize==profile.imageSize) return &code;
    return nullptr;
}
#include "bats_signature.h"
#include "bats_animation.h"
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
    return matches(code.calculation,code.calculationSize,code.calculationHash)&&matches(code.dexterity,44,code.dexterityHash)&&
        matches(code.declGetter,5,0x71fa801ff8c90d42ULL)&&matches(code.status,143,code.statusHash)&&
        scalar(code.calculation+5,code.calculation+9,1.f)&&scalar(code.calculation+code.firstMillisecondsOperand,code.calculation+code.firstMillisecondsOperand+4,.001f)&&
        scalar(code.calculation+code.secondMillisecondsOperand,code.calculation+code.secondMillisecondsOperand+4,.001f)&&
        scalar(code.dexterity+code.dexterityEpsilonOperand,code.dexterity+code.dexterityEpsilonOperand+4,std::numeric_limits<float>::epsilon())&&
        scalar(code.dexterity+code.dexterityOneOperand,code.dexterity+code.dexterityOneOperand+4,1.f)&&scalar(code.status+0x18,code.status+0x1c,1.f);
}
// Audited SetWeapon calculations:1.0 0x161dbbb..0x161ddcc;
//1.1 0x1659efb..0x165a10c; 5.1 0x181377b..0x181398c.
// Exact code fingerprints cover the mode branch,
// normal-rate operations, call targets and the final bring-up rate assignment.
// A zero requested bring-up duration leaves clip rate 1, multiplies the active
// fire-mode override, and divides by the clamped upgrade/status-effect speed.
// The no-status-effects case below computes that normal value, rather than
// comparing all weapons with a Ballista-specific rate learned from a demo.
static SpeedrunTechRow captureConfiguredBats(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                            const LevelIdentity& level,uintptr_t player,const GraphLookup& lookup) {
    SpeedrunTechRow row{"bats","BATS","inactive","No accelerated weapon appearance observed"};
    ConfiguredBatsCode discovered{};
    const auto* code=configuredBatsCode(profile);
    if(!code&&resolveBatsSignature(memory,base,profile,discovered)) code=&discovered;
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
    if(!memory.get(equipped+decl->offset,definition)||!definition||!memory.get(equipped+quickSlot->offset,weaponSlot)||weaponSlot< -1||weaponSlot>128) return row;
    const auto weaponName=techResourceName(memory,definition,lookup);
    if(weaponName.rfind("weapon/player/",0)!=0||weaponName.rfind("weapon/player/fists",0)==0||weaponName=="weapon/player/pointing_arm"||
       weaponName=="weapon/player/doomblade"||weaponName.rfind("weapon/player/equipment_",0)==0) return row;
    auto desired=techField(lookup,"idDeclWeapon","desiredBringupDurationMS"),milliseconds=techField(lookup,"milliToGameTime_t","milliseconds");
    if(!techInline(desired,"milliToGameTime_t",lookup)||!milliseconds||milliseconds->size!=4||
       milliseconds->type!="idTypesafeTime < int , millisecondUnique_t , 1000 >"||!milliseconds->ops.empty()) return row;
    int32_t desiredMS=-1;
    if(!memory.get(definition+desired->offset+milliseconds->offset,desiredMS)) return row;
    if(desiredMS<0) return row;
    auto mods=techField(lookup,"idWeapon","weaponModOverrides"),fireMode=techField(lookup,"idWeapon","fireMode");
    const std::string modType="idWeapon::weaponModOverrides_t"; const auto& modSchema=lookup(modType);
    auto scale=techField(lookup,modType,"bringUpRateScale");
    if(!mods||mods->type!=modType||mods->ops!="[2]"||!modSchema.complete||!modSchema.size||mods->size!=modSchema.size*2||
       !techFloat(scale)||!fireMode||fireMode->size!=4||fireMode->type!="weaponFireMode_t"||!fireMode->ops.empty()) return row;
    int32_t mode=-1; float modScale=0;
    if(!memory.get(equipped+fireMode->offset,mode)||mode<0||mode>1||
       !memory.get(equipped+mods->offset+size_t(mode)*modSchema.size+scale->offset,modScale)) return row;
    float clipScale=1;uintptr_t clipDefinition=0;
    if(desiredMS>0) {
        float clipSeconds=0;
        if(!batsModeDefinition(memory,equipped,definition,mode,lookup,clipDefinition)||
           !batsBringupClip(memory,handsAddress+animator->offset,clipDefinition,lookup,clipSeconds)) {
            row.detail="Weapon's normal bring-up animation could not be resolved";return row;
        }
        clipScale=clipSeconds/(float(desiredMS)*.001f);
    }
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
    uint32_t statusSwitchSlot=0;
    if(!memory.get(base+code->status+0x5f,statusSwitchSlot)||
       !batsNeutralStatusEffects(memory,base,profile,player+status->offset+effects->offset,effects->size,effectsCount,lookup,statusSwitchSlot)) {
        row.detail="Temporary status-effect animation multipliers are not yet supported";return row;
    }
    // Check the audited property's exact displacement and the configuration
    // branch. An alternate switch-rate mode uses a different normal formula.
    unsigned char load[7]{}; uintptr_t config=0; int32_t relative=0,rateMode=-1; uint32_t nativeOffset=0;
    if(!memory.read(base+code->calculation+9,load,7)||load[0]!=0x48||load[1]!=0x8b||load[2]!=0x0d) return row;
    memcpy(&relative,load+3,4);
    const uintptr_t configGlobal=base+code->calculation+16+relative;
    if(configGlobal<base||configGlobal+8>base+profile.imageSize||!memory.get(configGlobal,config)||!config||!memory.get(config+8,rateMode)||rateMode!=0||
       !memory.get(base+code->calculation+code->desiredOperand,nativeOffset)||nativeOffset!=desired->offset+milliseconds->offset||
       !memory.get(base+code->calculation+code->scaleOperand,nativeOffset)||nativeOffset!=mods->offset+scale->offset) return row;
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
    int32_t finalMode=-1;float finalRate=0;uintptr_t finalClipDefinition=0;
    if(!memory.get(equipped+fireMode->offset,finalMode)||finalMode!=mode||!memory.get(handsAddress+animator->offset+up->offset,finalRate)||finalRate!=actual||
       (desiredMS>0&&(!batsModeDefinition(memory,equipped,definition,mode,lookup,finalClipDefinition)||finalClipDefinition!=clipDefinition)))return row;
    const auto observed=configuredBatsState(actual,clipScale*modScale,speed,clamp);
    row.verified=observed!="unavailable";
    row.state=observed=="active"?"active":"inactive";
    row.detail=observed=="active"?"Weapon raise speed exceeds its current normal configuration":
        observed=="inactive"?"Weapon raise speed matches its current normal configuration":"Weapon animation has not reached a supported state";
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
    SpeedrunTechSnapshot result; result.rows={{"bats","BATS","inactive","No accelerated weapon appearance observed"},
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
        for(auto& row:result.rows) { row.state=row.key=="bats"?"inactive":"unavailable";row.verified=false;row.detail="Player inventory changed during observation"; }
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
