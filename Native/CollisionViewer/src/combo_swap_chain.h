#pragma once

// CSC retains the native mod-change flag after an interrupted weapon handoff.
// An ordinary mod animation alone is not sufficient evidence of a chain.
struct ComboChainSample {
    uintptr_t map=0,world=0,player=0,weapon=0;
    uint32_t generation=0;
    bool modChange=false,ghostFire=false;
    uint64_t observed=0;
};
class ComboChainTracker {
    ComboChainSample previous{};
    bool known=false,active=false;
public:
    void clear() { previous={};known=active=false; }
    bool update(const ComboChainSample& sample) {
        const bool continuous=known&&previous.map==sample.map&&previous.world==sample.world&&previous.player==sample.player&&
            sample.observed>=previous.observed&&sample.observed-previous.observed<=500;
        if(!continuous||!sample.modChange) active=false;
        if(sample.modChange) {
            const bool handoff=continuous&&previous.modChange&&(previous.weapon!=sample.weapon||previous.generation!=sample.generation);
            if(handoff||sample.ghostFire) active=true;
        }
        previous=sample;known=true;return active;
    }
};
static ComboChainTracker& comboChainTracker() { static ComboChainTracker value;return value; }
static bool comboChainWeaponClass(std::string type,const GraphLookup& lookup) {
    std::unordered_set<std::string> seen;
    uint32_t childSize=0x200000;
    for(unsigned depth=0;depth<32&&!type.empty();++depth) {
        if(!seen.insert(type).second)return false;
        const auto& schema=lookup(type);
        if(!schema.complete||!schema.size||schema.size>childSize)return false;
        if(type=="idWeapon")return true;
        childSize=schema.size;type=schema.parent;
    }
    return false;
}
static SpeedrunTechRow captureComboSwapChain(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                            const LevelIdentity& level,uintptr_t player,const GraphLookup& lookup) {
    SpeedrunTechRow row{"csc","CSC","unavailable","Combo Swap Chain metadata unavailable"};
    auto& tracker=comboChainTracker();
    auto fail=[&](const char* detail) { tracker.clear();row.detail=detail;return row; };
    // Warm common reflected layouts before a first rapid swap. These are
    // prefetch hints only: acceptance still follows the actual parent chain.
    for(const char* type:{"idWeapon","idPlasmaRifleCharged","idChaingun","idDoubleBarrelShotgun","idBFG","idChainsaw"})
        (void)lookup(type);
    auto hands=techField(lookup,"idPlayer","hands"),right=techField(lookup,"idHands","rightItem"),held=techField(lookup,"idHandsItem","item"),
        flags=techField(lookup,"idHands","handsFlags"),ptr=techField(lookup,"idManagedClassPtr < idInventoryItem >","ptr"),
        token=techField(lookup,"idManagedClassPtr < idInventoryItem >","spawnId");
    if(!techInline(hands,"idHands",lookup)||!techInline(right,"idHandsItem",lookup)||!techInline(held,"idManagedClassPtr < idInventoryItem >",lookup)||
       !techInline(flags,"idHands::handsFlags_t",lookup)||!techPointer(ptr,"idInventoryItem")||!token||token->type!="idSpawnId"||token->size!=4||!token->ops.empty())return fail("CSC metadata unavailable");
    const uintptr_t address=player+hands->offset+right->offset+held->offset,flagAddress=player+hands->offset+flags->offset;
    ComboChainSample sample;sample.map=level.map;sample.world=level.world;sample.player=player;sample.observed=GetTickCount64();
    uintptr_t vtable=0;
    if(!memory.get(address+ptr->offset,sample.weapon)||!sample.weapon||!memory.get(address+token->offset,sample.generation)||
       !memory.get(sample.weapon,vtable))return fail("Held weapon identity unreadable");
    // Plasma, Chaingun and several other weapons have their own native types.
    // Verify reflected inheritance instead of rejecting valid idWeapon subclasses.
    if(!comboChainWeaponClass(readEntityClass(memory,vtable,base,profile),lookup))return fail("Held weapon type is not verified");
    const char* names[]{"modChangeAnimPlaying","isFireAnimPlaying","isWeaponFiring","weaponWillFire","weaponWillDryfire","weaponFireAborted"};
    bool values[6]{};TechBoolBit bits[6];unsigned char bytes[6]{};
    for(size_t i=0;i<6;++i) {
        bits[i]=resolveTechBoolProperty(memory,base,profile,lookup,"idHands::handsFlags_t",names[i]);
        if(!bits[i]||!memory.get(flagAddress+bits[i].byte,bytes[i]))return fail("CSC animation flags unavailable");
        values[i]=(bytes[i]&bits[i].mask)!=0;
    }
    uintptr_t finalWeapon=0;uint32_t finalGeneration=0;
    if(!memory.get(address+ptr->offset,finalWeapon)||sample.weapon!=finalWeapon||!memory.get(address+token->offset,finalGeneration)||
       sample.generation!=finalGeneration||readLevelIdentity(memory,base,profile)!=level)return fail("Weapon or level changed during sampling");
    for(size_t i=0;i<6;++i) { unsigned char final=0;if(!memory.get(flagAddress+bits[i].byte,final)||final!=bytes[i])return fail("Animation flags changed during sampling"); }
    sample.modChange=values[0];
    sample.ghostFire=values[0]&&values[1]&&!values[2]&&values[3]&&!values[4]&&values[5];
    row.state=tracker.update(sample)?"active":"inactive";row.verified=true;
    row.detail=row.state=="active"?"Combo Swap Chain: retained mod-change across a weapon handoff or observed ghost fire":
        sample.modChange?"Mod-change is present; waiting for chain evidence":"No retained mod-change chain";
    return row;
}
