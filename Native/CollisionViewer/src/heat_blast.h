#pragma once

struct HeatBlastSample {
    uintptr_t weapon=0;
    uint32_t generation=0;
    int32_t shots=0,activationFrame=-1;
    int64_t lastShot=0;
    bool heatBlast=false;
};

struct HeatBlastRun {
    uintptr_t map=0;
    uint32_t started=0;
    explicit operator bool() const { return map!=0; }
    bool operator!=(const HeatBlastRun& other) const { return map!=other.map||started!=other.started; }
};

static HeatBlastRun readHeatBlastRun(const WallMemory& memory,uintptr_t base,const GameProfile& profile) {
    // idGameSystemLocal::state, stateStartTime and mapInstance. The start
    // stamp distinguishes reloads that reuse the same map address.
    struct State {
        uint32_t reserved,state,started,padding;
        uintptr_t map;
    };
    uintptr_t system=0;
    State before{},after{};
    if(!memory.get(base+profile.gameSystemRva,system)||!system||
       !memory.get(system+0x40,before)||before.state!=2||!before.map||
       !memory.get(system+0x40,after)||after.state!=2||after.map!=before.map||after.started!=before.started) return {};
    return {after.map,after.started};
}

class HeatBlastTracker {
    HeatBlastRun run;
    uintptr_t player=0;
    int64_t lastBlast=0;
    int32_t lastActivation=-1;
    uint32_t result=0;
    HeatBlastSample previous{};
public:
    bool tracking() const { return bool(run); }
    uint32_t count() const { return result; }
    bool isCurrent(const HeatBlastRun& current) const { return current&&!(current!=run); }

    void scope(const HeatBlastRun& current) {
        // A missed sample, a different player or a rebuilt physics world
        // provides no evidence that a checkpoint was reloaded.
        if(current&&current!=run) {
            *this={};
            run=current;
        }
    }

    void bindPlayer(uintptr_t current) {
        if(current!=player) {
            previous={};
            lastActivation=-1;
            player=current;
        }
    }

    void observe(const HeatBlastSample& sample) {
        if(sample.weapon!=previous.weapon||sample.generation!=previous.generation||
           sample.heatBlast!=previous.heatBlast||sample.shots<previous.shots) {
            previous=sample;
            return;
        }
        if(sample.shots==previous.shots) return;

        // The engine increments its secondary counter after each actual shot.
        // In a swap series the individual shots share the initiating fire event.
        if(sample.heatBlast&&sample.lastShot>0&&sample.lastShot>=previous.lastShot) {
            if(sample.activationFrame<0||sample.activationFrame!=lastActivation||sample.lastShot<lastBlast) result=0;
            const auto delta=static_cast<uint32_t>(sample.shots-previous.shots);
            result+=std::min(delta,UINT32_MAX-result);
            lastBlast=sample.lastShot;
            lastActivation=sample.activationFrame;
        }
        previous=sample;
    }
};

static InvisibleTechRow captureHeatBlast(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
    const LevelIdentity& level,const GraphLookup& lookup,HeatBlastTracker& tracker) {
    InvisibleTechRow row{"heat_blast","Multi Heat Blast","unavailable","Waiting for weapon telemetry"};
    const auto run=readHeatBlastRun(memory,base,profile);
    if(run&&run.map!=level.map) return row;
    tracker.scope(run);
    if(tracker.count()) row.state=std::to_string(tracker.count());
    if(!run) return row;

    uintptr_t player=0,vtable=0;
    if(!memory.get(level.map+profile.mapPlayerOffset,player)||!player||!memory.get(player,vtable)||vtable!=base+profile.playerVtableRva)
        return row;
    tracker.bindPlayer(player);
    auto inventory=readTimerInventory(memory,player,lookup);
    if(!inventory.known) return row;
    row.detail="Blasts in the last activation. Resets on checkpoint reload.";
    const TimerInventoryItem* plasma=nullptr;
    for(const auto& item:inventory.items) {
        if(item.owner==player&&timerResourceName(memory,item.decl,lookup)=="weapon/player/plasma_rifle") {
            plasma=&item;
            break;
        }
    }
    if(!plasma) {
        tracker.observe({});
        row.state=std::to_string(tracker.count());
        return row;
    }

    constexpr const char* modType="idWeapon::weaponModOverrides_t";
    constexpr const char* timesType="idArray < idTypesafeTime < long long , gameTimeUnique_t , 999960 > , 2 >";
    auto mods=timerField(lookup,"idWeapon","weaponModOverrides"),modDecl=timerField(lookup,modType,"weaponDecl"),
         shots=timerField(lookup,"idWeapon","fireModeFireCount"),lastShot=timerField(lookup,"idWeapon","lastFireTime"),
         frame=timerField(lookup,"idWeapon","fireModeForFireEventFrame");
    const auto& modSchema=lookup(modType);
    if(!mods||mods->type!=modType||mods->ops!="[2]"||!modSchema.complete||!modSchema.size||
       mods->size!=modSchema.size*2||!timerPointer(modDecl,"idDeclWeapon")||
       !shots||shots->type!="idArray < int , 2 >"||!shots->ops.empty()||shots->size!=8||
       !lastShot||lastShot->type!=timesType||!lastShot->ops.empty()||lastShot->size!=16||!timerIntField(frame)) return row;

    HeatBlastSample sample;
    sample.weapon=plasma->address;
    sample.generation=plasma->token;
    uintptr_t secondary=0;
    const auto secondaryAddress=sample.weapon+mods->offset+modSchema.size+modDecl->offset;
    if(!memory.get(secondaryAddress,secondary)) return row;
    const auto name=timerResourceName(memory,secondary,lookup);
    if(secondary&&name.empty()) return row;
    sample.heatBlast=name=="weapon/player/plasma_rifle_secondary_aoe";
    const auto frameAddress=sample.weapon+frame->offset;
    const auto timeAddress=sample.weapon+lastShot->offset+sizeof(int64_t);
    const auto countAddress=sample.weapon+shots->offset+sizeof(int32_t);
    if(!memory.get(frameAddress,sample.activationFrame)||!memory.get(timeAddress,sample.lastShot)||
       !memory.get(countAddress,sample.shots)||sample.shots<0) return row;

    // Fire state can change during a worker read. Leave the previous sample
    // intact until the count, fire event and selected mod agree.
    int32_t confirmedShots=0,confirmedFrame=0;
    int64_t confirmedLastShot=0;
    uintptr_t confirmedMod=0;
    if(memory.get(countAddress,confirmedShots)&&confirmedShots==sample.shots&&
       memory.get(timeAddress,confirmedLastShot)&&confirmedLastShot==sample.lastShot&&
       memory.get(frameAddress,confirmedFrame)&&confirmedFrame==sample.activationFrame&&
       memory.get(secondaryAddress,confirmedMod)&&confirmedMod==secondary&&
       tracker.isCurrent(readHeatBlastRun(memory,base,profile))) {
        tracker.observe(sample);
        row.state=std::to_string(tracker.count());
    }
    return row;
}
