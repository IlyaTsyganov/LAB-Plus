#pragma once

// Read-only timer observations. A value is published only when its reflected
// type and time domain are known; a missing source never becomes a stopwatch.
struct GameTimerRow {
    std::string key,group,label,state="unavailable",detail;
    double remaining=-1;
    double maximum=-1;
};
struct GameTimerSnapshot { std::string phase="unavailable"; std::vector<GameTimerRow> rows; };
static constexpr double gameTimerTicksPerSecond=999960.0;
static const GraphField* timerField(const GraphLookup& lookup,const std::string& type,const char* name) {
    const auto& schema=lookup(type);
    if(!schema.complete||!schema.size||schema.size>0x200000) return nullptr;
    for(const auto& field:schema.fields) if(field.name==name&&uint64_t(field.offset)+field.size<=schema.size) return &field;
    return nullptr;
}
static bool timerTimeField(const GraphField* field) {
    return field&&field->size==8&&field->ops.empty()&&field->type=="idTypesafeTime < long long , gameTimeUnique_t , 999960 >";
}
static bool timerBoolField(const GraphField* field) { return field&&field->size==1&&field->ops.empty()&&field->type=="bool"; }
static bool timerIntField(const GraphField* field) { return field&&field->size==4&&field->ops.empty()&&field->type=="int"; }
static bool timerInline(const GraphField* field,const std::string& type,const GraphLookup& lookup) {
    const auto& schema=lookup(type);
    return field&&field->type==type&&field->ops.empty()&&schema.complete&&schema.size&&schema.size==field->size;
}
static bool timerPointer(const GraphField* field,const char* type) { return field&&field->size==8&&field->type==type&&field->ops=="*"; }
static std::string timerResourceName(const WallMemory& memory,uintptr_t resource,const GraphLookup& lookup) {
    auto name=timerField(lookup,"idResource","name"),str=timerField(lookup,"idAtomicString","str");
    if(!resource||!name||name->type!="idAtomicString"||!str||str->type!="char"||str->ops!="*"||str->size!=8||uint64_t(str->offset)+8>name->size) return {};
    uintptr_t pointer=0;
    return memory.get(resource+name->offset+str->offset,pointer)?readEntityText(memory,pointer,512):std::string{};
}
static std::string timerPowerupLabel(const std::string& name) {
    std::string lower=name;
    std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    if(lower.find("berserk")!=std::string::npos) return "Berserk";
    if(lower.find("overdrive")!=std::string::npos||lower.find("haste")!=std::string::npos) return "Overdrive";
    if(lower.find("quad")!=std::string::npos) return "Quad damage";
    auto slash=name.find_last_of('/'); return slash==std::string::npos?name:name.substr(slash+1);
}
struct GameTimerClock { bool valid=false; int64_t ticks=0; std::string detail="Game clock has not been audited for this executable"; };
// The audited callers are the game's own flame-belch recharge check.
// They select time domain 1 (scaled game time), not Windows/renderer time.
// Validate the caller, field displacement, getter and map identity before
// following the clock. No engine code or property getter is executed.
static GameTimerClock readGameTimerClock(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                        const LevelIdentity& level,uint32_t flameOffset) {
    GameTimerClock result; uintptr_t caller=0;
    if(profile.timestamp==0x5e794131&&profile.imageSize==0x1e706000) caller=0x18db163;
    if(profile.timestamp==0x5ec7deca&&profile.imageSize==0x1d551000) caller=0x192c8d3;
    if(profile.timestamp==0x607e69b0&&profile.imageSize==0x1b5dd000) caller=0x1ae67a3;
    const auto* audited=observationCodeProfile(profile);
    if(audited) caller=audited->clock;
    if(!caller) return result;
    result.detail="Audited game clock could not be read";
    unsigned char code[37]{},getter[54]{};
    if(!memory.read(base+caller,code,sizeof(code))) return result;
    const unsigned char first[]{0x48,0x8b,0x0d},stack[]{0x48,0x8d,0x54,0x24,0x30},load[]{0x48,0x8b,0x9f},add[]{0x48,0x81,0xc1},kind[]{0x41,0xb8,1,0,0,0};
    uint32_t offset=0,manager=0; int32_t mapRelative=0,callRelative=0;
    memcpy(&mapRelative,code+3,4); memcpy(&offset,code+15,4); memcpy(&manager,code+22,4); memcpy(&callRelative,code+33,4);
    if(memcmp(code,first,3)||(memcmp(code+7,stack,4)||code[11]!=(audited?audited->clockStack:0x30))||memcmp(code+12,load,3)||offset!=flameOffset||memcmp(code+19,add,3)||
        manager>0x200000||memcmp(code+26,kind,6)||code[32]!=0xe8) return result;
    uintptr_t getterAddress=base+caller+37+callRelative;
    if(getterAddress<base||getterAddress+sizeof(getter)>=base+profile.imageSize||!memory.read(getterAddress,getter,sizeof(getter))) return result;
    const unsigned char body[]{0x4c,0x8b,0xc9}; // Compilers use mov r9,rcx as 4c 8b c9.
    const unsigned char tail[]{0x49,0x8b,0x41,0x10,0x48,0x8b,0x4c,0xc8,0x18};
    const unsigned char finish[]{0x48,0x8b,0xc2,0x48,0x89,0x0a,0xc3};
    const unsigned char fixedBody[]{0x83,0x78,0x08,0x00,0x75,0x0f,0x48,0x8b,0x41,0x10,0x48,0x8b,0x48,0x18,0x48,0x8b,0xc2,0x48,0x89,0x0a,0xc3,0x49,0x63,0xc0,0x48,0x8d,0x0c,0xc0};
    const unsigned char compactBody[]{0x4c,0x8b,0x49,0x10,0x83,0x78,0x08,0x00,0x75,0x0b,0x49,0x8b,0x41,0x18,0x48,0x89,0x02,0x48,0x8b,0xc2,0xc3,
        0x49,0x63,0xc0,0x48,0x8d,0x0c,0xc0,0x49,0x8b,0x44,0xc9,0x18,0x48,0x89,0x02,0x48,0x8b,0xc2,0xc3};
    const bool legacy=!memcmp(getter+7,body,3)&&!memcmp(getter+10,fixedBody,sizeof(fixedBody))&&
        !memcmp(getter+38,tail,sizeof(tail))&&!memcmp(getter+47,finish,sizeof(finish));
    const bool compact=audited&&audited->clockStack==0x50&&!memcmp(getter+7,compactBody,sizeof(compactBody));
    if(getter[0]!=0x48||getter[1]!=0x8b||getter[2]!=0x05||(!legacy&&!compact)) return result;
    int32_t flagRelative=0; memcpy(&flagRelative,getter+3,4);
    uintptr_t map=0,flag=0,data=0; int32_t timeScalingEnabled=0; int64_t ticks=0;
    uintptr_t mapGlobal=base+caller+7+mapRelative,flagGlobal=getterAddress+7+flagRelative;
    if(mapGlobal<base||mapGlobal+8>base+profile.imageSize||flagGlobal<base||flagGlobal+8>base+profile.imageSize||
       !memory.get(mapGlobal,map)||map!=level.map||!memory.get(flagGlobal,flag)||!flag||!memory.get(flag+8,timeScalingEnabled)||
       !memory.get(map+manager+16,data)||!data||!memory.get(data+24+(timeScalingEnabled?72:0),ticks)||ticks<0||ticks>int64_t(gameTimerTicksPerSecond*86400*365)) return result;
    result.valid=true; result.ticks=ticks; result.detail="Scaled game time (domain 1), 999960 ticks/s"; return result;
}
static bool timerRemaining(int64_t deadline,int64_t now,double& remaining,double limit=86400) {
    if(deadline<0||now<0) return false;
    remaining=deadline<=now?0:double(deadline-now)/gameTimerTicksPerSecond;
    return std::isfinite(remaining)&&remaining<=limit;
}
static void timerSetMaximum(GameTimerRow& row,double duration) {
    if(std::isfinite(duration)&&duration>0&&duration<=86400&&row.remaining>=0&&duration+.001>=row.remaining)
        row.maximum=std::max(duration,row.remaining);
}
static double timerConfiguredDuration(const WallMemory& memory,uintptr_t object,const GraphField* field,const GraphLookup& lookup) {
    auto milliseconds=timerField(lookup,"milliToGameTime_t","milliseconds");int32_t value=0;
    if(!timerInline(field,"milliToGameTime_t",lookup)||!milliseconds||milliseconds->size!=4||!milliseconds->ops.empty()||
       milliseconds->type!="idTypesafeTime < int , millisecondUnique_t , 1000 >"||!memory.get(object+field->offset+milliseconds->offset,value)||
       value<0||value>3600000)return -1;
    return double(value)/1000;
}
static bool readPowerupLifetimeOverride(const WallMemory& memory,uintptr_t base,const GameProfile& profile,double& seconds) {
    uintptr_t site=0;
    if(profile.timestamp==0x5e794131&&profile.imageSize==0x1e706000)site=0x1817fd1;
    if(profile.timestamp==0x5ec7deca&&profile.imageSize==0x1d551000)site=0x18671f1;
    if(profile.timestamp==0x607e69b0&&profile.imageSize==0x1b5dd000)site=0x1a0e261;
    if(const auto* code=observationCodeProfile(profile))site=code->powerup;
    if(!site)return false;
    unsigned char code[15]{};const unsigned char prefix[]{0x48,0x8b,0x05},tail[]{0x0f,0x57,0xff,0xf3,0x0f,0x10,0x48,0x0c};
    int32_t relative=0;uintptr_t data=0;float value=0;
    if(!memory.read(base+site,code,sizeof(code))||memcmp(code,prefix,3)||memcmp(code+7,tail,sizeof(tail)))return false;
    memcpy(&relative,code+3,4);uintptr_t global=base+site+7+relative;
    if(global<base||global+8>base+profile.imageSize||!memory.get(global,data)||!data||!memory.get(data+12,value)||!std::isfinite(value)||fabsf(value)>86400)return false;
    // statusEffect_lifetimeSec is the first override tested by activation.
    seconds=value;return true;
}
struct TimerInventoryItem { uintptr_t address=0,decl=0,owner=0,slot=0;uint32_t token=0;int32_t count=0;bool mapAllowed=false; };
struct TimerInventorySnapshot { bool known=false;std::vector<TimerInventoryItem> items; };
static TimerInventorySnapshot readTimerInventory(const WallMemory& memory,uintptr_t player,const GraphLookup& lookup) {
    TimerInventorySnapshot result;
    auto inventory=timerField(lookup,"idActor","inventory"),collection=timerField(lookup,"idInventoryCollection","inventory");
    const std::string listType="idList < idManagedClassPtr < idInventoryItem > , TAG_IDLIST , false >",managedType="idManagedClassPtr < idInventoryItem >";
    const auto& managed=lookup(managedType);
    auto data=timerField(lookup,listType,"list"),num=timerField(lookup,listType,"num"),ptr=timerField(lookup,managedType,"ptr"),
         spawn=timerField(lookup,managedType,"spawnId"),decl=timerField(lookup,"idInventoryItem","decl"),count=timerField(lookup,"idInventoryItem","count"),
         allowed=timerField(lookup,"idInventoryItem","mapUseAllowed"),owner=timerField(lookup,"idInventoryItem","owner"),
         ownerPtr=timerField(lookup,"idManagedClassPtr < idEntity >","ptr");
    if(!timerInline(inventory,"idInventoryCollection",lookup)||!timerInline(collection,listType,lookup)||!managed.complete||!managed.size||
       !data||data->size!=8||data->type!=managedType||data->ops!="*"||!timerIntField(num)||!timerPointer(ptr,"idInventoryItem")||
       !spawn||spawn->size!=4||spawn->type!="idSpawnId"||!spawn->ops.empty()||!timerPointer(decl,"idDeclInventory")||!timerIntField(count)||
       !timerBoolField(allowed)||!timerInline(owner,"idManagedClassPtr < idEntity >",lookup)||!timerPointer(ownerPtr,"idEntity"))return result;
    uintptr_t at=player+inventory->offset+collection->offset,list=0;int32_t length=0;
    if(!memory.get(at+data->offset,list)||!memory.get(at+num->offset,length)||length<0||length>1024||(!list&&length))return result;
    for(int32_t i=0;i<length;++i) {
        TimerInventoryItem item;item.slot=list+size_t(i)*managed.size;unsigned char mapAllowed=0;
        if(!memory.get(item.slot+ptr->offset,item.address)||!memory.get(item.slot+spawn->offset,item.token))return TimerInventorySnapshot{};
        if(!item.address)continue;
        if(!memory.get(item.address+decl->offset,item.decl)||!item.decl||!memory.get(item.address+count->offset,item.count)||
           !memory.get(item.address+allowed->offset,mapAllowed)||mapAllowed>1||!memory.get(item.address+owner->offset+ownerPtr->offset,item.owner))return TimerInventorySnapshot{};
        item.mapAllowed=mapAllowed!=0;result.items.push_back(item);
    }
    uintptr_t finalList=0;int32_t finalLength=0;
    if(!memory.get(at+data->offset,finalList)||finalList!=list||!memory.get(at+num->offset,finalLength)||finalLength!=length)return TimerInventorySnapshot{};
    for(const auto& item:result.items) {uintptr_t finalItem=0,finalDecl=0;uint32_t finalToken=0;
        if(!memory.get(item.slot+ptr->offset,finalItem)||finalItem!=item.address||!memory.get(item.slot+spawn->offset,finalToken)||finalToken!=item.token||
           !memory.get(item.address+decl->offset,finalDecl)||finalDecl!=item.decl)return TimerInventorySnapshot{};}
    result.known=true;return result;
}
struct TimerOwnership { std::string state="unavailable",detail="Inventory ownership unavailable"; };
static TimerOwnership timerItemOwnership(const TimerInventorySnapshot& inventory,uintptr_t declaration,uintptr_t player) {
    if(!inventory.known||!declaration)return {};
    TimerOwnership result{"not_acquired","Not acquired"};
    for(const auto& item:inventory.items)if(item.decl==declaration) {
        if(item.count<0){result={"unavailable","Inventory item count is unsupported"};continue;}
        if(!item.count)continue;
        if(item.owner!=player){result={"unavailable","Inventory item owner is not ready"};continue;}
        if(!item.mapAllowed){result={"unavailable","Unavailable on this map"};continue;}
        return {"owned","Acquired and available on this map"};
    }
    return result;
}
static std::array<TimerOwnership,3> readEquipmentOwnership(const WallMemory& memory,uintptr_t player,const TimerInventorySnapshot& inventory,const GraphLookup& lookup) {
    std::array<TimerOwnership,3> result{};
    const std::string entryType="idDeclPlayerProps::validEquipmentEntry_t",listType="idList < idDeclPlayerProps::validEquipmentEntry_t , TAG_IDLIST , false >";
    auto props=timerField(lookup,"idPlayer","playerProps"),valid=timerField(lookup,"idDeclPlayerProps","validEquipmentList"),
         data=timerField(lookup,listType,"list"),num=timerField(lookup,listType,"num"),decl=timerField(lookup,entryType,"item"),type=timerField(lookup,entryType,"type");
    const auto& entry=lookup(entryType);
    if(!inventory.known||!timerPointer(props,"idDeclPlayerProps")||!timerInline(valid,listType,lookup)||!entry.complete||!entry.size||
       !data||data->size!=8||data->ops!="*"||data->type!=entryType||!timerIntField(num)||!timerPointer(decl,"idDeclInventory")||
       !type||type->size!=4||type->type!="equipmentType_t"||!type->ops.empty())return result;
    uintptr_t definition=0,list=0;int32_t count=0;
    if(!memory.get(player+props->offset,definition)||!definition||!memory.get(definition+valid->offset+data->offset,list)||
       !memory.get(definition+valid->offset+num->offset,count)||count<0||count>256||(!list&&count))return result;
    std::array<bool,3> bound{};
    for(int32_t i=0;i<count;++i) {
        uintptr_t item=0;int32_t kind=-1;
        if(!memory.get(list+size_t(i)*entry.size+decl->offset,item)||!item||!memory.get(list+size_t(i)*entry.size+type->offset,kind))return std::array<TimerOwnership,3>{};
        if(kind<0||kind>=3)continue;
        auto ownership=timerItemOwnership(inventory,item,player);
        if(!bound[kind]||ownership.state=="owned"||(result[kind].state=="not_acquired"&&ownership.state=="unavailable"))result[kind]=std::move(ownership);
        bound[kind]=true;
    }
    uintptr_t finalDefinition=0,finalList=0;int32_t finalCount=0;
    if(!memory.get(player+props->offset,finalDefinition)||finalDefinition!=definition||!memory.get(definition+valid->offset+data->offset,finalList)||finalList!=list||
       !memory.get(definition+valid->offset+num->offset,finalCount)||finalCount!=count)return std::array<TimerOwnership,3>{};
    return result;
}
static TimerOwnership readWeaponModGrant(const WallMemory& memory,uintptr_t weapon,uintptr_t selectedDecl,const GraphLookup& lookup) {
    TimerOwnership result{"unavailable","Selected weapon mod ownership is not verified"};
    const std::string activeType="idUpgradeComponent::activeUpgradeInfo_t",activeList="idList < idUpgradeComponent::activeUpgradeInfo_t , TAG_IDLIST , false >",
        modifierType="idUpgradeMod_Weapon",modifierList="idList < idUpgradeMod_Weapon , TAG_IDLIST , false >";
    auto component=timerField(lookup,"idWeapon","upgradeComponent"),active=timerField(lookup,"idUpgradeComponent","activeUpgrades"),
         data=timerField(lookup,activeList,"list"),num=timerField(lookup,activeList,"num"),decl=timerField(lookup,activeType,"upgradeDecl"),
         count=timerField(lookup,activeType,"upgradeCount"),cheats=timerField(lookup,activeType,"cheatCount"),
         modifiers=timerField(lookup,"idDeclUpgrade","modifiersWeapon"),modData=timerField(lookup,modifierList,"list"),modNum=timerField(lookup,modifierList,"num"),
         type=timerField(lookup,modifierType,"type"),mode=timerField(lookup,modifierType,"fireMode"),value=timerField(lookup,modifierType,"data"),
         target=timerField(lookup,"upgradeDataValue_t","valueDecl");
    const auto& componentType=lookup("idInventoryItemUpgrade");const auto& entry=lookup(activeType);const auto& modifier=lookup(modifierType);
    if(!selectedDecl||!timerInline(component,"idInventoryItemUpgrade",lookup)||componentType.parent!="idUpgradeComponent"||
       !timerInline(active,activeList,lookup)||!entry.complete||!entry.size||!data||data->size!=8||data->ops!="*"||data->type!=activeType||!timerIntField(num)||
       !timerPointer(decl,"idDeclUpgrade")||!timerIntField(count)||!timerIntField(cheats)||!timerInline(modifiers,modifierList,lookup)||
       !modifier.complete||!modifier.size||!modData||modData->size!=8||modData->ops!="*"||modData->type!=modifierType||!timerIntField(modNum)||
       !type||type->size!=4||type->ops!=""||type->type!="idUpgradeMod_Weapon::weaponModType_t"||!mode||mode->size!=4||mode->ops!=""||mode->type!="weaponFireMode_t"||
       !timerInline(value,"upgradeDataValue_t",lookup)||!timerPointer(target,"idGameDeclTypeInfo"))return result;
    uintptr_t address=weapon+component->offset+active->offset,list=0;int32_t length=0;unsigned work=0;bool granted=false;
    if(!memory.get(address+data->offset,list)||!memory.get(address+num->offset,length)||length<0||length>256||(!list&&length))return result;
    for(int32_t i=0;i<length;++i) {
        uintptr_t at=list+size_t(i)*entry.size,upgrade=0;int32_t used=0,cheat=0;
        if(!memory.get(at+decl->offset,upgrade)||!upgrade||!memory.get(at+count->offset,used)||!memory.get(at+cheats->offset,cheat)||used<0||cheat<0)return result;
        if(!used&&!cheat)continue;
        uintptr_t modList=0;int32_t modCount=0;
        if(!memory.get(upgrade+modifiers->offset+modData->offset,modList)||!memory.get(upgrade+modifiers->offset+modNum->offset,modCount)||
           modCount<0||modCount>256||(!modList&&modCount)||work+unsigned(modCount)>4096)return result;
        work+=unsigned(modCount);
        for(int32_t j=0;j<modCount;++j) {
            uintptr_t mod=modList+size_t(j)*modifier.size,pointer=0;int32_t kind=0,fireMode=-1;
            if(!memory.get(mod+type->offset,kind)||!memory.get(mod+mode->offset,fireMode))return result;
            // WMT_WEAPON_DECL_WEAPON=40 on the audited timer profiles.
            // Ownership is the active grant of this exact weapon declaration.
            // Its fire-mode application can be inherited from the upgrade or
            // apply to both modes; it is not an unlock identifier.
            if(kind!=40)continue;
            if(!memory.get(mod+value->offset+target->offset,pointer))return result;
            if(pointer==selectedDecl)granted=true;
        }
        uintptr_t finalMods=0,finalUpgrade=0;int32_t finalModCount=0,finalUsed=0,finalCheat=0;
        if(!memory.get(upgrade+modifiers->offset+modData->offset,finalMods)||finalMods!=modList||!memory.get(upgrade+modifiers->offset+modNum->offset,finalModCount)||finalModCount!=modCount||
           !memory.get(at+decl->offset,finalUpgrade)||finalUpgrade!=upgrade||!memory.get(at+count->offset,finalUsed)||finalUsed!=used||!memory.get(at+cheats->offset,finalCheat)||finalCheat!=cheat)return result;
    }
    uintptr_t finalList=0;int32_t finalCount=0;
    if(!memory.get(address+data->offset,finalList)||finalList!=list||!memory.get(address+num->offset,finalCount)||finalCount!=length)return result;
    return granted?TimerOwnership{"owned","Selected mod has an active weapon-declaration grant"}:result;
}
// This is the engine's end-time calculation for a clamped idMD6LeafPlay,
// audited in 5.1 at 0x627900 / 0x605e60. It describes the current clip, not
// future nodes in an animation web or a predicted end of a whole sequence.
static bool timerLeafEnd(uint16_t frames,uint16_t fps,int64_t start,const float (&scales)[3],float rateScale,int64_t& end) {
    if(frames<2||!fps||fps>1000||start<0||!std::isfinite(rateScale)) return false;
    for(float value:scales) if(!std::isfinite(value)||fabsf(value)>100) return false;
    float effective=(((scales[2]*scales[0])-1.f)*scales[1]+1.f)*rateScale;
    if(!std::isfinite(effective)||effective<=0||effective>100) return false;
    float seconds=(float(frames)-1.f)/float(fps);
    float ticks=float(gameTimerTicksPerSecond)*seconds;
    if(!std::isfinite(ticks)||ticks<=0||ticks>=float(INT32_MAX)) return false;
    float duration=(1.f/effective)*float(static_cast<int32_t>(ticks));
    if(!std::isfinite(duration)||duration<0||duration>=float(INT32_MAX)||start>INT64_MAX-static_cast<int32_t>(duration)) return false;
    end=start+static_cast<int32_t>(duration); return true;
}
struct StickyChargeTimers { bool valid=false; int capacity=0,available=0; double nextCharge=0,fullReload=0; };
static double stickyReloadMaximum(double configured,int heatFireMode,const std::array<double,2>& overrides) {
    auto mode=[&](unsigned index){double value=overrides[index];
        if(!std::isfinite(value))return -1.;
        if(value==-1)value=configured;
        return std::isfinite(value)&&value>=0&&value<=3600?value:-1.;};
    if(heatFireMode==0||heatFireMode==1)return mode(static_cast<unsigned>(heatFireMode));
    if(heatFireMode==2) {double first=mode(0),second=mode(1);if(first>=0&&second>=0&&fabs(first-second)<.001)return std::max(first,second);}
    return -1;
}
static StickyChargeTimers stickyChargeTimers(float heat,float increment,float maximum,float decrement,int64_t cooling,int64_t overheated) {
    StickyChargeTimers result;
    if(!std::isfinite(heat)||!std::isfinite(increment)||!std::isfinite(maximum)||!std::isfinite(decrement)||
       heat<0||increment<=0||maximum<=0||heat>maximum+.0001f||decrement<=0||cooling<0||overheated<0||
       cooling>int64_t(gameTimerTicksPerSecond*3600)||overheated>int64_t(gameTimerTicksPerSecond*3600)) return result;
    float count=maximum/increment,rounded=roundf(count);
    if(!std::isfinite(count)||rounded<1||rounded>100||fabsf(count-rounded)>.001f) return result;
    result.capacity=static_cast<int>(rounded);
    // Same single-precision ceil used by the weapon's heat/charge HUD.
    int used=std::clamp(static_cast<int>(ceilf(heat/increment)),0,result.capacity);
    result.available=result.capacity-used;
    if(overheated>0) { result.available=0; result.fullReload=double(overheated)/gameTimerTicksPerSecond; }
    else if(used>0) {
        float untilNext=std::max(0.f,heat-float(used-1)*increment);
        result.nextCharge=double(cooling)/gameTimerTicksPerSecond+double(untilNext)/decrement;
        if(!std::isfinite(result.nextCharge)||result.nextCharge>3600) return StickyChargeTimers{};
    }
    result.valid=true; return result;
}
static std::vector<GameTimerRow> stickyChargeQueue(const StickyChargeTimers& timers,float increment,float decrement,double coolingMaximum,double reloadMaximum) {
    std::vector<GameTimerRow> result;
    if(!timers.valid||timers.available<0||timers.available>timers.capacity||timers.capacity>100||
       !std::isfinite(increment)||!std::isfinite(decrement)||increment<=0||decrement<=0)return result;
    const double interval=double(increment)/decrement;
    if(!std::isfinite(interval)||interval<=0||interval>3600)return result;
    const int missing=timers.capacity-timers.available;
    for(int i=0;i<missing;++i) {
        const bool shared=timers.fullReload>0;
        GameTimerRow row;row.key="sticky_charge_"+std::to_string(i+1);row.group="sticky_charges";row.label="Charge "+std::to_string(i+1);
        row.state="cooldown";row.remaining=shared?timers.fullReload:timers.nextCharge+double(i)*interval;
        row.detail=shared?"Shared full reload: all missing charges become ready together":"Sequential recovery from the shared heat value; current recharge rate";
        if(shared)timerSetMaximum(row,reloadMaximum);
        else if(coolingMaximum>=0)timerSetMaximum(row,coolingMaximum+double(i+1)*interval);
        result.push_back(std::move(row));
    }
    return result;
}
static std::vector<GameTimerRow> captureStickyTimers(const WallMemory& memory,uintptr_t base,const GameProfile& profile,uintptr_t player,
                                                   const GameTimerClock& clock,const GraphLookup& lookup,const TimerInventorySnapshot* ownership=nullptr) {
    std::vector<GameTimerRow> rows{{"sticky_bomb","Weapon mods","Sticky bombs: next charge","unavailable","Weapon-mod timer metadata unavailable",-1},
        {"sticky_bomb_reload","Weapon mods","Sticky bombs: full reload","unavailable","Weapon-mod timer metadata unavailable",-1}};
    TimerInventorySnapshot localInventory;if(!ownership){localInventory=readTimerInventory(memory,player,lookup);ownership=&localInventory;}
    if(!ownership->known)return rows;
    auto inventory=timerField(lookup,"idActor","inventory"),items=timerField(lookup,"idInventoryCollection","inventory");
    if(!timerInline(inventory,"idInventoryCollection",lookup)||!items||!items->ops.empty()) return rows;
    auto resourceName=timerField(lookup,"idResource","name"),atomicText=timerField(lookup,"idAtomicString","str");
    if(!resourceName||resourceName->type!="idAtomicString"||!resourceName->ops.empty()||!atomicText||atomicText->type!="char"||atomicText->ops!="*"||atomicText->size!=8||
       uint64_t(atomicText->offset)+8>resourceName->size)return rows;
    const auto& itemList=lookup(items->type); auto itemData=timerField(lookup,items->type,"list"),itemCount=timerField(lookup,items->type,"num");
    const std::string managedType="idManagedClassPtr < idInventoryItem >";const auto& managed=lookup(managedType);
    auto ptr=timerField(lookup,managedType,"ptr"),decl=timerField(lookup,"idInventoryItem","decl"),mods=timerField(lookup,"idWeapon","weaponModOverrides"),
         supers=timerField(lookup,"idWeapon","superchargeParms"),mode=timerField(lookup,"idWeapon","fireMode"),heat=timerField(lookup,"idWeapon","heatPercent"),
         cooling=timerField(lookup,"idWeapon","heatCoolingDelay"),overheated=timerField(lookup,"idWeapon","overheatedDelay");
    const std::string modType="idWeapon::weaponModOverrides_t",superType="idWeapon::superchargeParms_t",heatType="idDeclWeapon::heatInfo_t";
    const auto& modSchema=lookup(modType);const auto& superSchema=lookup(superType);
    auto modDecl=timerField(lookup,modType,"weaponDecl"),superDecl=timerField(lookup,superType,"weaponDecl"),secondary=timerField(lookup,"idDeclWeapon","secondaryFireDecl"),
         heatInfo=timerField(lookup,"idDeclWeapon","heatInfo"),force=timerField(lookup,heatType,"forceUseSecondary"),
         increment=timerField(lookup,heatType,"heatIncrement"),maximum=timerField(lookup,heatType,"maxHeatPercent"),decrement=timerField(lookup,heatType,"heatDecrementRate"),
         overrideIncrement=timerField(lookup,modType,"heatIncrement"),overrideDecrement=timerField(lookup,modType,"heatDecrementRate"),overrideMaximum=timerField(lookup,modType,"heatMaxPercent");
    auto coolingDefinition=timerField(lookup,heatType,"coolingDelayMS"),reloadDefinition=timerField(lookup,heatType,"overheatDelayMS"),heatFireMode=timerField(lookup,heatType,"overheatFireMode"),
         reloadOverride=timerField(lookup,modType,"overheatDelayTime");
    auto scalar=[](const GraphField* f){return f&&f->type=="float"&&f->ops.empty()&&f->size==4;};
    if(!itemList.complete||itemList.size!=items->size||!itemData||itemData->ops!="*"||itemData->type!=managedType||itemData->size!=8||!timerIntField(itemCount)||
       !managed.complete||!managed.size||!timerPointer(ptr,"idInventoryItem")||!timerPointer(decl,"idDeclInventory")||!mods||mods->type!=modType||mods->ops!="[2]"||
       !modSchema.complete||!modSchema.size||mods->size!=modSchema.size*2||!supers||supers->type!=superType||supers->ops!="[2]"||
       !superSchema.complete||!superSchema.size||supers->size!=superSchema.size*2||!timerPointer(modDecl,"idDeclWeapon")||!timerPointer(superDecl,"idDeclWeapon")||
       !mode||mode->type!="weaponFireMode_t"||mode->size!=4||!mode->ops.empty()||!scalar(heat)||!timerTimeField(cooling)||!timerTimeField(overheated)||
       !timerPointer(secondary,"idDeclWeapon")||!timerInline(heatInfo,heatType,lookup)||!timerBoolField(force)||!scalar(increment)||!scalar(maximum)||!scalar(decrement)||
       !scalar(overrideIncrement)||!scalar(overrideDecrement)||!scalar(overrideMaximum)) return rows;
    uintptr_t address=player+inventory->offset+items->offset,list=0;int32_t count=0;
    if(!memory.get(address+itemData->offset,list)||!memory.get(address+itemCount->offset,count)||count<0||count>1024||(!list&&count)) return rows;
    for(int32_t i=0;i<count;++i) {
        uintptr_t entry=list+size_t(i)*managed.size,weapon=0,vtable=0,baseDecl=0,secondaryMod=0;
        if(!memory.get(entry+ptr->offset,weapon))return rows;
        if(!weapon)continue;
        if(!memory.get(weapon,vtable))return rows;
        const auto itemClass=readEntityClass(memory,vtable,base,profile);
        if(itemClass.empty())return rows;
        if(itemClass!="idWeapon")continue;
        if(!memory.get(weapon+decl->offset,baseDecl))return rows;
        auto baseName=timerResourceName(memory,baseDecl,lookup);
        if(baseName.empty())return rows;
        if(baseName!="weapon/player/shotgun")continue;
        auto baseOwnership=timerItemOwnership(*ownership,baseDecl,player);
        if(baseOwnership.state!="owned"){for(auto& row:rows){row.state=baseOwnership.state;row.detail=baseOwnership.detail;row.remaining=baseOwnership.state=="not_acquired"?0:-1;}return rows;}
        if(!memory.get(weapon+mods->offset+modSchema.size+modDecl->offset,secondaryMod)) return rows;
        const auto secondaryName=timerResourceName(memory,secondaryMod,lookup);
        if(secondaryMod&&secondaryName.empty())return rows;
        if(secondaryName!="weapon/player/shotgun_secondary_pop_rockets") {
            for(auto& row:rows){row.state="inactive";row.detail="Sticky bomb mod is not selected";row.remaining=0;}return rows;
        }
        if(!clock.valid){for(auto& row:rows)row.detail=clock.detail;return rows;}
        auto modOwnership=readWeaponModGrant(memory,weapon,secondaryMod,lookup);
        if(modOwnership.state!="owned"){for(auto& row:rows){row.state="unavailable";row.detail=modOwnership.detail;row.remaining=-1;}return rows;}
        int32_t fireMode=-1;float heatValue=0,inc=0,max=0,dec=0;int64_t cool=0,over=0;uintptr_t selected=0,secondarySuper=0,originalSecondary=0;
        if(!memory.get(weapon+mode->offset,fireMode)||fireMode<0||fireMode>1||!memory.get(weapon+heat->offset,heatValue)||
           !memory.get(weapon+cooling->offset,cool)||!memory.get(weapon+overheated->offset,over)||
           !memory.get(weapon+supers->offset+superSchema.size+superDecl->offset,secondarySuper)||!memory.get(baseDecl+secondary->offset,originalSecondary)) return rows;
        uintptr_t forced=secondarySuper?secondarySuper:secondaryMod?secondaryMod:originalSecondary;
        unsigned char forceValue=0;float forceIncrement=0;
        if(forced&&(!memory.get(forced+heatInfo->offset+force->offset,forceValue)||forceValue>1||!memory.get(forced+heatInfo->offset+increment->offset,forceIncrement)))return rows;
        if(forced&&forceValue&&forceIncrement>0) selected=forced;
        else {
            uintptr_t super=0,mod=0;
            if(!memory.get(weapon+supers->offset+superSchema.size*fireMode+superDecl->offset,super)||
               !memory.get(weapon+mods->offset+modSchema.size*fireMode+modDecl->offset,mod))return rows;
            selected=super?super:mod?mod:(fireMode?originalSecondary:baseDecl);
        }
        float incOverride=0,decOverride=0,maxOverride=0;
        if(!selected||!memory.get(selected+heatInfo->offset+increment->offset,inc)||!memory.get(selected+heatInfo->offset+maximum->offset,max)||
           !memory.get(selected+heatInfo->offset+decrement->offset,dec)||
           !memory.get(weapon+mods->offset+modSchema.size*fireMode+overrideIncrement->offset,incOverride)||
           !memory.get(weapon+mods->offset+modSchema.size*fireMode+overrideDecrement->offset,decOverride)||
           !memory.get(weapon+mods->offset+modSchema.size*fireMode+overrideMaximum->offset,maxOverride))return rows;
        if(incOverride>=0)inc=incOverride;
        if(decOverride>=0)dec=decOverride;
        if(maxOverride>=0)max=maxOverride;
        if(dec==0) { // Engine falls back to the other fire mode when current rate is zero.
            int32_t other=1-fireMode;float otherOverride=0;uintptr_t otherSuper=0,otherMod=0;
            if(!memory.get(weapon+mods->offset+modSchema.size*other+overrideDecrement->offset,otherOverride)||
               !memory.get(weapon+supers->offset+superSchema.size*other+superDecl->offset,otherSuper)||
               !memory.get(weapon+mods->offset+modSchema.size*other+modDecl->offset,otherMod))return rows;
            if(otherOverride>=0)dec=otherOverride;
            else {uintptr_t otherDecl=otherSuper?otherSuper:otherMod?otherMod:(other?originalSecondary:baseDecl);if(!otherDecl||!memory.get(otherDecl+heatInfo->offset+decrement->offset,dec))return rows;}
        }
        double coolingMaximum=timerConfiguredDuration(memory,selected+heatInfo->offset,coolingDefinition,lookup),
            configuredReload=timerConfiguredDuration(memory,selected+heatInfo->offset,reloadDefinition,lookup),reloadMaximum=-1;
        std::array<double,2> reloadOverrides{-2,-2};int32_t heatingMode=-1;
        if(timerTimeField(reloadOverride))for(unsigned index=0;index<2;++index) {int64_t value=0;
            if(memory.get(weapon+mods->offset+modSchema.size*index+reloadOverride->offset,value)) {
                if(value==-1)reloadOverrides[index]=-1;
                else if(value>=0&&value<=int64_t(gameTimerTicksPerSecond*3600))reloadOverrides[index]=double(value)/gameTimerTicksPerSecond;
            }
        }
        if(heatFireMode&&heatFireMode->type=="weaponFireMode_t"&&heatFireMode->size==4&&heatFireMode->ops.empty()&&
           memory.get(selected+heatInfo->offset+heatFireMode->offset,heatingMode))reloadMaximum=stickyReloadMaximum(configuredReload,heatingMode,reloadOverrides);
        auto timers=stickyChargeTimers(heatValue,inc,max,dec,cool,over);
        uintptr_t finalWeapon=0,finalSecondary=0,finalList=0;int32_t finalCount=0,finalMode=-1;
        if(!timers.valid||!memory.get(entry+ptr->offset,finalWeapon)||finalWeapon!=weapon||
           !memory.get(weapon+mods->offset+modSchema.size+modDecl->offset,finalSecondary)||finalSecondary!=secondaryMod||
           !memory.get(address+itemData->offset,finalList)||finalList!=list||!memory.get(address+itemCount->offset,finalCount)||finalCount!=count||
           !memory.get(weapon+mode->offset,finalMode)||finalMode!=fireMode)return rows;
        const auto charges=std::to_string(timers.available)+" / "+std::to_string(timers.capacity)+" charges available";
        rows[0].label="Sticky bombs ("+std::to_string(timers.available)+"/"+std::to_string(timers.capacity)+"): next charge";
        rows[1].label="Sticky bombs ("+std::to_string(timers.available)+"/"+std::to_string(timers.capacity)+"): full reload";
        rows[0].remaining=timers.nextCharge;rows[0].state=over>0?"inactive":timers.nextCharge>0?"cooldown":"ready";
        rows[0].detail=charges+(over>0?"; full reload in progress":"; next individual charge at current recharge rate");
        rows[1].remaining=timers.fullReload;rows[1].state=over>0?"cooldown":"inactive";rows[1].detail=charges+(over>0?"; all charges reload together":"; full reload is not active");
        if(coolingMaximum>=0)timerSetMaximum(rows[0],coolingMaximum+double(inc)/dec);
        timerSetMaximum(rows[1],reloadMaximum);
        auto queue=stickyChargeQueue(timers,inc,dec,coolingMaximum,reloadMaximum);
        for(auto& charge:queue)rows.push_back(std::move(charge));
        return rows;
    }
    for(auto& row:rows){row.state="not_acquired";row.detail="Combat shotgun not acquired";row.remaining=0;}return rows;
}
#include "sync_animation_timers.h"

static GameTimerSnapshot captureGameTimers(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                          const LevelIdentity& level,const GraphLookup& lookup) {
    GameTimerSnapshot result;
    auto add=[&](const char* key,const char* group,const char* label,const std::string& detail)->GameTimerRow& {
        result.rows.push_back({key,group,label,"unavailable",detail,-1}); return result.rows.back();
    };
    uintptr_t system=0,map=0,player=0,vtable=0; unsigned char deleted=1;
    if(!level||!memory.get(base+profile.gameSystemRva,system)||!memory.get(system+0x50,map)||map!=level.map||
       !memory.get(map+profile.mapPlayerOffset,player)||!player||!memory.get(player,vtable)||vtable!=base+profile.playerVtableRva||
       !memory.get(player+0xa4,deleted)||deleted) { result.phase="loading"; return result; }
    auto flame=timerField(lookup,"idPlayer","flameBelchNextUseTime");
    auto clock=timerTimeField(flame)?readGameTimerClock(memory,base,profile,level,flame->offset):GameTimerClock{};
    auto ownedInventory=readTimerInventory(memory,player,lookup);auto equipmentOwnership=readEquipmentOwnership(memory,player,ownedInventory,lookup);
    auto equipment=timerField(lookup,"idPlayer","equipmentInfo");
    const auto equipmentType=std::string("idPlayer::equipmentInfo_t"),itemType=std::string("idPlayer::equipmentInfo_t::itemData_t");
    auto items=timerField(lookup,equipmentType,"equipmentItems"),recharge=timerField(lookup,itemType,"equipmentRechargeTime"),
         reuse=timerField(lookup,itemType,"equipmentReuseTime"),uses=timerField(lookup,itemType,"equipmentUsesAvailable"),duration=timerField(lookup,itemType,"equipmentRechargeDuration");
    const auto& itemSchema=lookup(itemType);
    const uint32_t equipmentCount=itemSchema.size&&items?items->size/itemSchema.size:0;
    bool equipmentLayout=timerInline(equipment,equipmentType,lookup)&&items&&items->ops.empty()&&
        (equipmentCount==3||equipmentCount==4)&&items->type=="idArray < idPlayer::equipmentInfo_t::itemData_t , "+std::to_string(equipmentCount)+" >"&&
        itemSchema.complete&&itemSchema.size&&items->size==itemSchema.size*equipmentCount&&
        timerTimeField(recharge)&&timerTimeField(reuse)&&timerIntField(uses);
    // equipmentType_t is audited as FRAG=0, ICE=1, FLAME=2 on the clock profiles.
    const char* keys[]{"frag_grenade","ice_bomb","flame_belch"}; const char* labels[]{"Frag grenade","Ice bomb","Flame belch"};
    for(unsigned slot=0;slot<3;++slot) {
        auto& row=add(keys[slot],"Equipment",labels[slot],equipmentLayout?clock.detail:"Equipment timer metadata unavailable");
        if(equipmentOwnership[slot].state!="owned"){row.state=equipmentOwnership[slot].state;row.detail=equipmentOwnership[slot].detail;row.remaining=row.state=="not_acquired"?0:-1;continue;}
        if(!equipmentLayout||!clock.valid) continue;
        uintptr_t address=player+equipment->offset+items->offset+itemSchema.size*slot;
        int64_t deadline=0,reuseTime=0; int32_t available=0;
        if(!memory.get(address+recharge->offset,deadline)||!memory.get(address+reuse->offset,reuseTime)||
           !memory.get(address+uses->offset,available)||available<0||available>16) { row.detail="Equipment timer read failed"; continue; }
        // flameBelchNextUseTime is also written after a rejected button
        // attempt. It identifies the clock domain, not a consumed charge.
        // The equipment recharge/reuse records track the actual cooldown.
        if(!timerRemaining(std::max(deadline,reuseTime),clock.ticks,row.remaining,3600)) { row.detail="Equipment timer value outside supported range"; continue; }
        if(row.remaining==0&&available==0){row.state="unavailable";row.remaining=-1;row.detail="No charge available and no active recharge timer";continue;}
        row.state=row.remaining>0?"cooldown":"ready";
        row.detail=std::to_string(available)+" charge(s) available; engine recharge/reuse deadline";
        int64_t fullDuration=0;
        if(timerTimeField(duration)&&memory.get(address+duration->offset,fullDuration)&&fullDuration>0&&fullDuration<=int64_t(gameTimerTicksPerSecond*3600))
            timerSetMaximum(row,double(fullDuration)/gameTimerTicksPerSecond);
        if(slot==2&&row.maximum<0) {
            // Flame Belch can use only equipmentReuseTime, leaving the grenade
            // recharge-duration record zero. Its configured cooldown is stored
            // in the active player declaration in milliseconds.
            auto props=timerField(lookup,"idPlayer","playerProps"),cooldown=timerField(lookup,"idDeclPlayerProps","flameBelchCooldownTimeMS");
            uintptr_t definition=0;int32_t milliseconds=0;
            if(timerPointer(props,"idDeclPlayerProps")&&cooldown&&cooldown->ops.empty()&&cooldown->size==4&&
               cooldown->type=="idTypesafeTime < int , millisecondUnique_t , 1000 >"&&memory.get(player+props->offset,definition)&&definition&&
               memory.get(definition+cooldown->offset,milliseconds)&&milliseconds>0&&milliseconds<=3600000)
                timerSetMaximum(row,double(milliseconds)/1000);
        }
    }
    auto status=timerField(lookup,"idActor","statusEffectComponent");
    auto active=timerField(lookup,"idStatusEffectComponent","activeStatusEffects");
    const std::string listType="idList < idStatusEffect * , TAG_IDLIST , false >";
    auto list=timerField(lookup,listType,"list"),num=timerField(lookup,listType,"num"),left=timerField(lookup,"idStatusEffect","timeLeft"),
         decl=timerField(lookup,"idStatusEffect","decl"),inactive=timerField(lookup,"idStatusEffect","inActive"),paused=timerField(lookup,"idStatusEffect","isPaused"),
         isPowerup=timerField(lookup,"idDeclStatusEffect","isPowerUp");
    bool statusLayout=timerInline(status,"idStatusEffectComponent",lookup)&&active&&active->type=="idStaticList < idStatusEffect * , 10 , false , TAG_IDLIST >"&&
        active->ops.empty()&&list&&list->size==8&&list->type=="idStatusEffect"&&list->ops=="**"&&timerIntField(num)&&
        uint64_t(list->offset)+8<=active->size&&uint64_t(num->offset)+4<=active->size&&timerTimeField(left)&&timerPointer(decl,"idDeclStatusEffect")&&
        timerBoolField(inactive)&&timerBoolField(paused)&&timerBoolField(isPowerup);
    bool statusRead=false;
    if(statusLayout) {
        uintptr_t address=player+status->offset+active->offset,elements=0; int32_t count=0;
        if(memory.get(address+list->offset,elements)&&memory.get(address+num->offset,count)&&count>=0&&count<=10&&(!count||elements)) {
            statusRead=true;
            auto lifetime=timerField(lookup,"idDeclStatusEffect","lifeTimeMS"),overrides=timerField(lookup,"idStatusEffect","overrides"),
                overrideTime=timerField(lookup,"idStatusEffect::upgradeOverride_t","lifeTime");
            double globalLifetime=0;const bool lifetimeOverrideKnown=readPowerupLifetimeOverride(memory,base,profile,globalLifetime);
            for(int32_t i=0;i<count;++i) {
                uintptr_t effect=0,definition=0,final=0; unsigned char off=1,isPaused=0,powerup=0; int64_t remaining=0;
                if(!memory.get(elements+size_t(i)*8,effect)||!effect||!memory.get(effect+decl->offset,definition)||!definition||
                   !memory.get(effect+inactive->offset,off)||off>1||!memory.get(effect+paused->offset,isPaused)||isPaused>1||
                   !memory.get(definition+isPowerup->offset,powerup)||powerup>1) { statusRead=false; break; }
                if(!powerup||off) continue;
                auto name=timerResourceName(memory,definition,lookup);
                if(name.empty()||!memory.get(effect+left->offset,remaining)||!memory.get(effect+decl->offset,final)||final!=definition) { statusRead=false; break; }
                GameTimerRow row; row.key="powerup_"+std::to_string(i); row.group="Powerups"; row.label=timerPowerupLabel(name); row.detail=name+(isPaused?" (paused)":"");
                if(remaining>=0&&remaining<=int64_t(gameTimerTicksPerSecond*86400)) { row.remaining=double(remaining)/gameTimerTicksPerSecond; row.state="active"; }
                else { row.state="active"; row.detail+="; unlimited or unsupported timer"; }
                if(lifetimeOverrideKnown) {
                    if(globalLifetime>0)timerSetMaximum(row,globalLifetime);
                    else if(timerInline(overrides,"idStatusEffect::upgradeOverride_t",lookup)&&timerTimeField(overrideTime)) {
                        int64_t overrideTicks=0;
                        if(memory.get(effect+overrides->offset+overrideTime->offset,overrideTicks)) {
                            if(overrideTicks>=0&&overrideTicks<=int64_t(gameTimerTicksPerSecond*86400))timerSetMaximum(row,double(overrideTicks)/gameTimerTicksPerSecond);
                            else if(overrideTicks==-1&&lifetime&&lifetime->size==4&&lifetime->ops.empty()&&lifetime->type=="idTypesafeTime < int , millisecondUnique_t , 1000 >") {
                                int32_t milliseconds=0;
                                if(memory.get(definition+lifetime->offset,milliseconds)&&milliseconds>0&&milliseconds<=86400000)timerSetMaximum(row,double(milliseconds)/1000);
                            }
                        }
                    }
                }
                result.rows.push_back(std::move(row));
            }
            uintptr_t finalElements=0; int32_t finalCount=0;
            if(!memory.get(address+list->offset,finalElements)||finalElements!=elements||!memory.get(address+num->offset,finalCount)||finalCount!=count) statusRead=false;
        }
    }
    if(!statusRead) {
        result.rows.erase(std::remove_if(result.rows.begin(),result.rows.end(),[](const auto& r){return r.group=="Powerups";}),result.rows.end());
        add("powerups","Powerups","Powerups","Status-effect timer metadata or stable data unavailable");
    } else if(std::none_of(result.rows.begin(),result.rows.end(),[](const auto& r){return r.group=="Powerups";})) {
        auto& row=add("powerups","Powerups","Powerups","No active engine powerup status effects"); row.state="inactive"; row.remaining=0;
    }
    auto body=timerField(lookup,"idPlayer","bodyAnimInfo"),ledge=timerField(lookup,"idPlayer::playerBodyAnimInfo_t","ledgeGrabAnimPlaying"),
         mechanic=timerField(lookup,"idPlayer","playerMechanicLedgeGrab"),
         end=timerField(lookup,"idPlayerMechanicLedgeGrab","grabEndTime"),start=timerField(lookup,"idPlayerMechanicLedgeGrab","grabStartTime");
    bool bodyLayout=timerInline(body,"idPlayer::playerBodyAnimInfo_t",lookup);
    auto& mantle=add("mantle","Sync animations","Mantle","Mantle animation timer metadata unavailable");
    if(bodyLayout&&timerBoolField(ledge)) {
        unsigned char on=0;
        if(memory.get(player+body->offset+ledge->offset,on)&&on<=1) {
            if(!on) { mantle.state="inactive"; mantle.remaining=0; mantle.detail="Ledge-grab animation inactive"; }
            else if(clock.valid&&timerInline(mechanic,"idPlayerMechanicLedgeGrab",lookup)&&timerTimeField(end)) {
                int64_t deadline=0;
                if(memory.get(player+mechanic->offset+end->offset,deadline)&&timerRemaining(deadline,clock.ticks,mantle.remaining,60)) {
                    mantle.state="active"; mantle.detail="Engine grabEndTime";
                    int64_t started=0;
                    if(timerTimeField(start)&&memory.get(player+mechanic->offset+start->offset,started)&&started>=0&&deadline>=started)
                        timerSetMaximum(mantle,double(deadline-started)/gameTimerTicksPerSecond);
                }
            }
        }
    }
    auto syncTimers=captureSyncAnimationTimers(memory,player,clock,lookup,base,&profile);
    result.rows.insert(result.rows.end(),syncTimers.begin(),syncTimers.end());
    auto sticky=captureStickyTimers(memory,base,profile,player,clock,lookup,&ownedInventory);result.rows.insert(result.rows.end(),sticky.begin(),sticky.end());
    uintptr_t finalMap=0,finalPlayer=0;
    if(!memory.get(system+0x50,finalMap)||finalMap!=map||!memory.get(map+profile.mapPlayerOffset,finalPlayer)||finalPlayer!=player||
       !memory.get(player+0xa4,deleted)||deleted) { result.rows.clear(); result.phase="loading"; return result; }
    result.phase=std::any_of(result.rows.begin(),result.rows.end(),[](const auto& row){return row.remaining>=0;})?"partial":"unavailable";
    return result;
}
