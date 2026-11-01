#pragma once

// Read-only encounter membership and health. Every offset comes from reflected
// fields, including the health-component stride (88 bytes in 1.0, 96 in 5.1).
struct EnemyTelemetryRow {
    EntityReference entity;
    uint32_t encounterId=0,encounterGeneration=0;
    std::string encounter,name;
    double health=0,maximum=0;
    bool staggerKnown=false,staggerActive=false;
    double staggerSeconds=0;
    int64_t staggerDeadline=0,staggerDuration=0;
    uint32_t staggerVolatileOffset=~0u,staggerPainOffset=~0u,staggerReactionOffset=~0u,staggerFinishOffset=~0u,staggerLengthOffset=~0u;
    bool maximumKnown=false,positionKnown=false;
    Vec3 top{};
    uint32_t physicsOffset=~0u,worldBoundsOffset=~0u,physicsMapOffset=~0u;
    uint32_t healthOffset=~0u,deadOffset=~0u;
};
struct EnemyTelemetryCapture {
    std::string state="metadata_pending",detail;
    uint32_t unavailable=0;
    std::vector<EnemyTelemetryRow> enemies;
    std::vector<EnemyTelemetryRow> barEnemies;
};
static const GraphField* enemyTelemetryField(const GraphType& schema,const char* name) {
    for(const auto& field:schema.fields) if(field.name==name) return &field; return nullptr;
}
struct EnemyTelemetryLayout {
    uint32_t think=~0u,name=~0u,stringData=~0u,removed=~0u;
    uint32_t spawnSize=0,actor=~0u,managedSize=0,ptr=~0u,token=~0u;
    uint32_t health=~0u,components=~0u,componentSize=0,componentCount=0,type=~0u,current=~0u,maximum=~0u,dead=~0u;
    uint32_t physics=~0u,bounds=~0u,physicsMap=~0u;
    uint32_t staggerVolatile=~0u,staggerReaction=~0u,staggerFinish=~0u,staggerLength=~0u;
    struct List { uint32_t offset=0,stride=0,actor=0; };
    std::vector<List> lists;
    bool valid=false,pending=false;
};
static EnemyTelemetryLayout resolveEnemyTelemetryLayout(const GraphLookup& lookup) {
    EnemyTelemetryLayout layout;
    auto schema=[&](const char* name)->const GraphType& { const auto& result=lookup(name); if(!result.complete) layout.pending=true; return result; };
    auto field=[&](const GraphType& type,const char* name,const char* expected,uint32_t size)->const GraphField* {
        const auto* value=enemyTelemetryField(type,name);
        return value&&value->type==expected&&value->size==size&&value->ops.empty()?value:nullptr;
    };
    const auto& entity=schema("idEntity"); const auto& bloated=schema("idBloatedEntity"); const auto& string=schema("idStr");
    const auto& manager=schema("idEncounterManager"); const auto& spawn=schema("idEncounterSpawn_t");
    const auto& managed=schema("idManagedClassPtr < idEntity >"); const auto& ai=schema("idAI2");
    const auto& health=schema("idHealthT < aiHealthComponent_t , AI_HEALTH_MAX , AI_HEALTH_HITPOINTS >");
    const auto& component=schema("idHealthT < aiHealthComponent_t , AI_HEALTH_MAX , AI_HEALTH_HITPOINTS >::component_t");
    const auto* think=field(bloated,"thinkFlags","int",4); const auto* name=enemyTelemetryField(entity,"name");
    const auto* data=enemyTelemetryField(string,"data"); const auto* actor=field(spawn,"actor","idManagedClassPtr < idEntity >",managed.size);
    const auto* pointer=enemyTelemetryField(managed,"ptr"); const auto* token=enemyTelemetryField(managed,"spawnId");
    const auto* aiHealth=enemyTelemetryField(ai,"aiHealth"); const auto* components=enemyTelemetryField(health,"components");
    const auto* dead=field(health,"isDead","bool",1); const auto* componentType=field(component,"type","aiHealthComponent_t",4);
    const auto* current=field(component,"cur","float",4); const auto* maximum=field(component,"max","float",4);
    bool healthAncestry=false;
    if(aiHealth&&aiHealth->type=="idAIHealth"&&aiHealth->ops.empty()) {
        std::string parent="idAIHealth"; uint32_t available=aiHealth->size;
        for(unsigned depth=0;depth<24&&!parent.empty();++depth) {
            const auto& ancestor=schema(parent.c_str());
            if(!ancestor.complete||!ancestor.size||ancestor.size>available) break;
            if(parent=="idHealthT < aiHealthComponent_t , AI_HEALTH_MAX , AI_HEALTH_HITPOINTS >") { healthAncestry=true; break; }
            available=ancestor.size; parent=ancestor.parent;
        }
    }
    // idEntity.removed is a reflected property without stored bytes in some
    // builds. The audited map-slot ABI has the removal byte at 0xa4, shared
    // with the production entity/encounter readers.
    layout.removed=0xa4;
    if(think) layout.think=think->offset;
    if(name&&name->type.rfind("idStr",0)==0&&name->ops.empty()&&data&&data->type=="char"&&data->ops=="*"&&data->size==8&&data->offset+8<=name->size) {
        layout.name=name->offset; layout.stringData=data->offset;
    }
    if(actor&&pointer&&pointer->type=="idEntity"&&pointer->ops=="*"&&pointer->size==8&&token&&token->type=="idSpawnId"&&token->size==4) {
        layout.spawnSize=spawn.size; layout.actor=actor->offset; layout.managedSize=managed.size; layout.ptr=pointer->offset; layout.token=token->offset;
        for(const char* listName:{"managedEncounterSpawns","managedSpiritRespawns","managedNewSpiritSpawns"}) {
            const auto* list=enemyTelemetryField(manager,listName);
            if(list&&list->size==24&&list->type=="idList < idEncounterSpawn_t , TAG_IDLIST , false >"&&list->ops.empty())
                layout.lists.push_back({list->offset,spawn.size,actor->offset});
        }
        const auto* adopted=enemyTelemetryField(manager,"adoptedStandAloneAI");
        if(adopted&&adopted->size==24&&adopted->type=="idList < idManagedClassPtr < idEntity > , TAG_IDLIST , false >"&&adopted->ops.empty())
            layout.lists.push_back({adopted->offset,managed.size,0});
    }
    if(healthAncestry&&aiHealth&&aiHealth->size>=health.size&&health.size&&components&&components->type=="idHealthT < aiHealthComponent_t , AI_HEALTH_MAX , AI_HEALTH_HITPOINTS >::component_t"&&
       components->ops=="[2]"&&component.size&&components->size==component.size*2&&dead&&componentType&&current&&maximum) {
        layout.health=aiHealth->offset; layout.components=components->offset; layout.componentSize=component.size; layout.componentCount=2;
        layout.type=componentType->offset; layout.current=current->offset; layout.maximum=maximum->offset; layout.dead=dead->offset;
    }
    const bool requiredPending=layout.pending;
    const auto& physics=lookup("idPhysics"); const auto& havok=lookup("idHavokPhysics_Base");
    const auto* physicsPointer=enemyTelemetryField(entity,"physics"); const auto* bounds=field(havok,"bodyBoundsWorld","idBounds",24); const auto* map=enemyTelemetryField(physics,"mapInstance");
    if(physicsPointer&&physicsPointer->size==8&&physicsPointer->type=="idPhysics"&&physicsPointer->ops=="*"&&bounds&&map&&map->size==8&&map->type=="idMapInstance"&&map->ops=="*") {
        layout.physics=physicsPointer->offset; layout.bounds=bounds->offset; layout.physicsMap=map->offset;
    }
    const auto& pain=lookup("aiPainState_t");
    const auto* aiVolatile=enemyTelemetryField(ai,"aiVolatile"),*reaction=field(pain,"currentPainReaction","painType_t",4);
    const auto* finish=field(pain,"finishStaggerTime","idTypesafeTime < long long , gameTimeUnique_t , 999960 >",8);
    const auto* length=field(pain,"staggerLength","idTypesafeTime < long long , gameTimeUnique_t , 999960 >",8);
    if(pain.complete&&pain.size&&aiVolatile&&aiVolatile->size==8&&aiVolatile->type=="idAIVolatile"&&aiVolatile->ops=="*"&&reaction&&finish&&length) {
        layout.staggerVolatile=aiVolatile->offset; layout.staggerReaction=reaction->offset; layout.staggerFinish=finish->offset; layout.staggerLength=length->offset;
    }
    layout.pending=requiredPending;
    const auto* primary=enemyTelemetryField(manager,"managedEncounterSpawns");
    bool primaryKnown=primary&&primary->size==24&&primary->type=="idList < idEncounterSpawn_t , TAG_IDLIST , false >"&&primary->ops.empty();
    layout.valid=!layout.pending&&layout.think!=~0u&&layout.name!=~0u&&layout.health!=~0u&&primaryKnown&&!layout.lists.empty();
    return layout;
}
static bool enemyTelemetryIdentity(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const EntityReference& ref) {
    struct Entry { uint32_t id,generation; uintptr_t entity,previous,next; } current{};
    uintptr_t system=0,map=0,list=0; uint32_t count=0; unsigned char removed=1;
    return ref&&memory.get(base+profile.gameSystemRva,system)&&memory.get(system+0x50,map)&&map==ref.map&&
        memory.get(map+profile.mapEntitiesOffset,list)&&memory.get(map+profile.mapEntitiesOffset+8,count)&&count<=32768&&ref.slot>=list&&
        (ref.slot-list)%sizeof(Entry)==0&&(ref.slot-list)/sizeof(Entry)<count&&memory.get(ref.slot,current)&&current.id==ref.id&&
        current.generation==ref.generation&&current.entity==ref.address&&memory.get(ref.address+0xa4,removed)&&!removed;
}
static bool refreshEnemyTelemetryTop(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const EnemyTelemetryRow& row,Vec3& freshTop) {
    if(row.physicsOffset==~0u||row.worldBoundsOffset==~0u||row.physicsMapOffset==~0u||!enemyTelemetryIdentity(memory,base,profile,row.entity)) return false;
    unsigned char dead=2;
    if(row.healthOffset==~0u||row.deadOffset==~0u||!memory.get(row.entity.address+row.healthOffset+row.deadOffset,dead)||dead) return false;
    uintptr_t physics=0,map=0,finalPhysics=0; float bounds[6]{};
    if(!memory.get(row.entity.address+row.physicsOffset,physics)||!physics||!memory.get(physics+row.physicsMapOffset,map)||map!=row.entity.map||
       !memory.read(physics+row.worldBoundsOffset,bounds,sizeof(bounds))) return false;
    for(float value:bounds) if(!std::isfinite(value)||std::abs(value)>1e7f) return false;
    for(unsigned axis=0;axis<3;++axis) if(bounds[axis]>bounds[axis+3]||bounds[axis+3]-bounds[axis]>10000) return false;
    if(bounds[5]-bounds[2]<.01f||!memory.get(row.entity.address+row.physicsOffset,finalPhysics)||physics!=finalPhysics||
       !enemyTelemetryIdentity(memory,base,profile,row.entity)) return false;
    freshTop={(bounds[0]+bounds[3])*.5f,(bounds[1]+bounds[4])*.5f,bounds[5]+.15f}; return true;
}
// idAIVolatile has no public field table. Follow only the two audited vanilla
// getters: one returns aiPainState_t, the other its finishStaggerTime. Match
// their pointer displacement to reflection and their deadline displacement to
// the reflected field, so unsupported/changed code yields no countdown.
static bool resolveEnemyStaggerPointer(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                      const EnemyTelemetryLayout& layout,uint32_t& painOffset) {
    if(layout.staggerVolatile==~0u||layout.staggerReaction==~0u||layout.staggerFinish==~0u||layout.staggerLength==~0u) return false;
    uintptr_t painGetter=0,finishGetter=0;
    if(profile.timestamp==0x5e794131&&profile.imageSize==0x1e706000) { painGetter=0xbb6fc0; finishGetter=0xbb7050; }
    if(profile.timestamp==0x5ec7deca&&profile.imageSize==0x1d551000) { painGetter=0xbc3f00; finishGetter=0xbc3fb0; }
    if(profile.timestamp==0x607e69b0&&profile.imageSize==0x1b5dd000) { painGetter=0xc8ceb0; finishGetter=0xc8cf40; }
    if(!painGetter||!finishGetter) return false;
    unsigned char pain[14]{},finish[21]{}; uint32_t painPointer=0,finishPointer=0,deadline=0;
    if(!memory.read(base+painGetter,pain,sizeof(pain))||!memory.read(base+finishGetter,finish,sizeof(finish))) return false;
    const unsigned char load[]{0x48,0x8b,0x81},add[]{0x48,0x05},read[]{0x48,0x8b,0x88},tail[]{0x48,0x8b,0xc2,0x48,0x89,0x0a,0xc3};
    memcpy(&painPointer,pain+3,4); memcpy(&painOffset,pain+9,4); memcpy(&finishPointer,finish+3,4); memcpy(&deadline,finish+10,4);
    return !memcmp(pain,load,3)&&!memcmp(pain+7,add,2)&&pain[13]==0xc3&&painPointer==layout.staggerVolatile&&painOffset<=0x200000&&
        !memcmp(finish,load,3)&&!memcmp(finish+7,read,3)&&!memcmp(finish+14,tail,sizeof(tail))&&finishPointer==painPointer&&
        uint64_t(painOffset)+layout.staggerFinish==deadline;
}
static bool refreshEnemyStagger(const WallMemory& memory,uintptr_t base,const GameProfile& profile,EnemyTelemetryRow& row,int64_t gameNow=-1) {
    row.staggerKnown=false; row.staggerActive=false; row.staggerSeconds=0; row.staggerDeadline=0; row.staggerDuration=0;
    if(row.staggerVolatileOffset==~0u||row.staggerPainOffset==~0u||row.staggerReactionOffset==~0u||row.staggerFinishOffset==~0u||
       row.staggerLengthOffset==~0u||!enemyTelemetryIdentity(memory,base,profile,row.entity)) return false;
    unsigned char dead=2;
    if(row.healthOffset==~0u||row.deadOffset==~0u||!memory.get(row.entity.address+row.healthOffset+row.deadOffset,dead)||dead) return false;
    uintptr_t data=0,finalData=0; int32_t reaction=-1,finalReaction=-1; int64_t deadline=0,duration=0,finalDeadline=0;
    if(!memory.get(row.entity.address+row.staggerVolatileOffset,data)||!data||
       !memory.get(data+row.staggerPainOffset+row.staggerReactionOffset,reaction)||reaction<0||reaction>=22||
       !memory.get(data+row.staggerPainOffset+row.staggerFinishOffset,deadline)||
       !memory.get(data+row.staggerPainOffset+row.staggerLengthOffset,duration)||
       !memory.get(row.entity.address+row.staggerVolatileOffset,finalData)||data!=finalData||
       !memory.get(data+row.staggerPainOffset+row.staggerReactionOffset,finalReaction)||reaction!=finalReaction||
       !memory.get(data+row.staggerPainOffset+row.staggerFinishOffset,finalDeadline)||deadline!=finalDeadline||
       !enemyTelemetryIdentity(memory,base,profile,row.entity)) return false;
    row.staggerDeadline=deadline; row.staggerDuration=duration;
    // The vanilla stagger/recovery path accepts both PAIN_STAGGER (16) and
    // PAIN_STAGGER_VULNERABLE (17). A live 1.0 glory-kill demonstration uses
    // 16 with a five-second engine deadline. Flinch, falter and stun differ.
    if(reaction!=16&&reaction!=17) { row.staggerKnown=true; return true; }
    if(gameNow<0||deadline<0||duration<0) return false;
    double remaining=deadline<=gameNow?0:double(deadline-gameNow)/999960.0;
    if(!std::isfinite(remaining)||remaining>86400) return false;
    row.staggerKnown=true; row.staggerSeconds=remaining; row.staggerActive=deadline>gameNow; return true;
}
enum class EnemyTelemetryRead { Alive,NotAlive,Unavailable };
static EnemyTelemetryRead readEnemyTelemetryActor(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
    const EnemyTelemetryLayout& layout,const EntityReference& ref,EnemyTelemetryRow& row) {
    if(!enemyTelemetryIdentity(memory,base,profile,ref)) return EnemyTelemetryRead::NotAlive;
    unsigned char dead=2; float hp=0,maximum=0; bool matched=false;
    if(!memory.get(ref.address+layout.health+layout.dead,dead)||dead>1) return EnemyTelemetryRead::Unavailable;
    if(dead) return EnemyTelemetryRead::NotAlive;
    for(uint32_t component=0;component<layout.componentCount;++component) {
        uintptr_t values=ref.address+layout.health+layout.components+component*layout.componentSize; int32_t kind=-1;
        if(!memory.get(values+layout.type,kind)) continue;
        // Reflected enum audited in 1.0 and 5.1: HITPOINTS=0,
        // SHIELD=1, MAX=2. Never report shield as body health.
        if(kind!=0) continue;
        matched=memory.get(values+layout.current,hp)&&memory.get(values+layout.maximum,maximum); break;
    }
    if(!matched||!std::isfinite(hp)||hp<0||hp>1e9f) return EnemyTelemetryRead::Unavailable;
    if(hp<=0) return EnemyTelemetryRead::NotAlive;
    row.entity=ref; row.health=hp; row.maximum=maximum;
    row.maximumKnown=std::isfinite(maximum)&&maximum>0&&maximum<=1e9f;
    uintptr_t text=0; if(memory.get(ref.address+layout.name+layout.stringData,text)) row.name=readEntityText(memory,text,256);
    row.physicsOffset=layout.physics; row.worldBoundsOffset=layout.bounds; row.physicsMapOffset=layout.physicsMap;
    row.healthOffset=layout.health; row.deadOffset=layout.dead;
    uint32_t painOffset=~0u;
    if(resolveEnemyStaggerPointer(memory,base,profile,layout,painOffset)) {
        row.staggerVolatileOffset=layout.staggerVolatile; row.staggerPainOffset=painOffset;
        row.staggerReactionOffset=layout.staggerReaction; row.staggerFinishOffset=layout.staggerFinish; row.staggerLengthOffset=layout.staggerLength;
        refreshEnemyStagger(memory,base,profile,row);
    }
    row.positionKnown=refreshEnemyTelemetryTop(memory,base,profile,row,row.top);
    if(!memory.get(ref.address+layout.health+layout.dead,dead)||dead||!enemyTelemetryIdentity(memory,base,profile,ref)) return EnemyTelemetryRead::NotAlive;
    return EnemyTelemetryRead::Alive;
}
static EnemyTelemetryCapture captureEnemyTelemetry(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                                  const LevelIdentity& level,const GraphLookup& lookup,bool collectBars=false) {
    EnemyTelemetryCapture result; auto layout=resolveEnemyTelemetryLayout(lookup);
    if(!layout.valid) { result.state=layout.pending?"metadata_pending":"metadata_unavailable"; result.detail="Encounter membership / AI health metadata unavailable"; return result; }
    struct Entry { uint32_t id,generation; uintptr_t entity,previous,next; };
    uintptr_t system=0,map=0,list=0; uint32_t count=0;
    if(!level||!memory.get(base+profile.gameSystemRva,system)||!memory.get(system+0x50,map)||map!=level.map) { result.state="loading"; return result; }
    if(!memory.get(map+profile.mapEntitiesOffset,list)||!memory.get(map+profile.mapEntitiesOffset+8,count)||count>32768) { result.state="read_error"; return result; }
    std::vector<Entry> entries(count);
    if(count&&!memory.read(list,entries.data(),entries.size()*sizeof(Entry))) { result.state="read_error"; return result; }
    std::unordered_map<uintptr_t,size_t> byAddress; for(size_t i=0;i<entries.size();++i) if(entries[i].entity) byAddress.emplace(entries[i].entity,i);
    std::unordered_map<uintptr_t,std::string> classes;
    auto type=[&](uintptr_t address)->std::string {
        uintptr_t vtable=0; if(!memory.get(address,vtable)) return {};
        auto found=classes.find(vtable); if(found==classes.end()) found=classes.emplace(vtable,readEntityClass(memory,vtable,base,profile)).first;
        return found->second;
    };
    auto reference=[&](size_t index,const std::string& className) {
        const auto& entry=entries[index]; return EntityReference{entry.entity,map,list+index*sizeof(Entry),entry.id,entry.generation,className};
    };
    std::unordered_map<std::string,bool> aiClasses;
    auto aiType=[&](std::string name) {
        auto cached=aiClasses.find(name); if(cached!=aiClasses.end()) return cached->second;
        const auto original=name;
        for(unsigned ancestry=0;ancestry<24&&!name.empty();++ancestry) {
            if(name=="idAI2") { aiClasses[original]=true; return true; }
            const auto& schema=lookup(name); if(!schema.complete) { ++result.unavailable; aiClasses[original]=false; return false; } name=schema.parent;
        } aiClasses[original]=false; return false;
    };
    auto entityName=[&](uintptr_t address) { uintptr_t text=0; return memory.get(address+layout.name+layout.stringData,text)?readEntityText(memory,text,256):std::string{}; };
    result.state="ready"; uint32_t referencesRead=0;
    for(size_t managerIndex=0;managerIndex<entries.size()&&referencesRead<=32768&&result.enemies.size()<2048;++managerIndex) {
        const auto& manager=entries[managerIndex]; if(!manager.entity||type(manager.entity)!="idEncounterManager") continue;
        const auto managerRef=reference(managerIndex,"idEncounterManager"); int32_t think=0;
        if(!enemyTelemetryIdentity(memory,base,profile,managerRef)) continue;
        if(!memory.get(manager.entity+layout.think,think)) { ++result.unavailable; continue; } if(!think) continue;
        std::string name=entityName(manager.entity),label=encounterLabel(level.name,name);
        if(label.rfind("Encounter: ",0)==0) label=label.substr(11); if(label.empty()) label=name;
        std::unordered_set<uintptr_t> seen;
        for(const auto& source:layout.lists) {
            if(referencesRead>32768||result.enemies.size()>=2048) break;
            uintptr_t data=0; int32_t length=0;
            if(!memory.get(manager.entity+source.offset,data)||!memory.get(manager.entity+source.offset+8,length)||length<0||length>4096||(!data&&length)) { ++result.unavailable; continue; }
            size_t start=result.enemies.size();
            for(int32_t i=0;i<length;++i) {
                if(++referencesRead>32768) { ++result.unavailable; break; }
                if(result.enemies.size()>=2048) { ++result.unavailable; break; }
                uintptr_t link=data+size_t(i)*source.stride+source.actor,address=0; uint32_t token=0;
                if(!memory.get(link+layout.ptr,address)||!memory.get(link+layout.token,token)) { ++result.unavailable; continue; } if(!address) continue;
                auto found=byAddress.find(address); if(found==byAddress.end()) { ++result.unavailable; continue; }
                std::string className=type(address); if(!aiType(className)) continue;
                EnemyTelemetryRow row;
                auto read=readEnemyTelemetryActor(memory,base,profile,layout,reference(found->second,className),row);
                if(read!=EnemyTelemetryRead::Alive) { if(read==EnemyTelemetryRead::Unavailable) ++result.unavailable; continue; }
                row.encounterId=manager.id; row.encounterGeneration=manager.generation; row.encounter=label; row.name=entityName(address);
                uintptr_t finalAddress=0; uint32_t finalToken=0; unsigned char finalDead=2;
                if(!memory.get(link+layout.ptr,finalAddress)||finalAddress!=address||!memory.get(link+layout.token,finalToken)||finalToken!=token||
                   !memory.get(address+layout.health+layout.dead,finalDead)||finalDead||!enemyTelemetryIdentity(memory,base,profile,row.entity)) { ++result.unavailable; continue; }
                if(seen.insert(address).second) result.enemies.push_back(std::move(row));
            }
            uintptr_t finalData=0; int32_t finalLength=0;
            if(!memory.get(manager.entity+source.offset,finalData)||finalData!=data||!memory.get(manager.entity+source.offset+8,finalLength)||finalLength!=length) {
                result.enemies.resize(start); seen.clear(); for(const auto& row:result.enemies) if(row.encounterId==manager.id&&row.encounterGeneration==manager.generation) seen.insert(row.entity.address);
                ++result.unavailable;
            }
        }
        int32_t finalThink=0;
        if(!enemyTelemetryIdentity(memory,base,profile,managerRef)||!memory.get(manager.entity+layout.think,finalThink)||finalThink!=think)
            result.enemies.erase(std::remove_if(result.enemies.begin(),result.enemies.end(),[&](const auto& row) { return row.encounterId==manager.id&&row.encounterGeneration==manager.generation; }),result.enemies.end());
    }
    // A visible AI can be spawned by a console/spawner or remain outside a
    // manager's active script. Bars use all living map AI; they do not invent
    // encounter membership for the grouped encounter list above.
    if(collectBars) for(size_t i=0;i<entries.size()&&result.barEnemies.size()<2048;++i) {
        if(!entries[i].entity) continue;
        const auto className=type(entries[i].entity); if(className.empty()||!aiType(className)) continue;
        EnemyTelemetryRow row; auto read=readEnemyTelemetryActor(memory,base,profile,layout,reference(i,className),row);
        if(read==EnemyTelemetryRead::Alive) result.barEnemies.push_back(std::move(row));
        else if(read==EnemyTelemetryRead::Unavailable) ++result.unavailable;
    }
    uintptr_t finalList=0; uint32_t finalCount=0;
    if(readLevelIdentity(memory,base,profile)!=level||!memory.get(map+profile.mapEntitiesOffset,finalList)||finalList!=list||
       !memory.get(map+profile.mapEntitiesOffset+8,finalCount)||finalCount!=count) { result.enemies.clear(); result.barEnemies.clear(); result.state="loading"; return result; }
    if(referencesRead>32768||result.enemies.size()>=2048||result.barEnemies.size()>=2048) { result.state="partial"; result.detail="Enemy observation limit reached"; }
    else if(result.unavailable) result.state="partial";
    return result;
}
