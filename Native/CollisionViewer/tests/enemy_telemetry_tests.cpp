#include "../src/vulkan_layer.cpp"
#include "../src/enemy_telemetry.h"
static void expect(bool value,const char* message) { if(!value) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
template<class T> static void put(void* data,size_t offset,T value) { memcpy(static_cast<unsigned char*>(data)+offset,&value,sizeof(value)); }
static std::unordered_map<std::string,GraphType> makeSchemas(uint32_t stride) {
    const std::string health="idHealthT < aiHealthComponent_t , AI_HEALTH_MAX , AI_HEALTH_HITPOINTS >",component=health+"::component_t";
    std::unordered_map<std::string,GraphType> result;
    result["idEntity"]={576,"",{{"name","idStr","",64,48},{"physics","idPhysics","*",464,8}},true};
    result["idBloatedEntity"]={2928,"idEntity",{{"thinkFlags","int","",1576,4}},true};
    result["idStr"]={48,"",{{"data","char","*",8,8}},true};
    result["idEncounterManager"]={4264,"idBloatedEntity",{{"managedEncounterSpawns","idList < idEncounterSpawn_t , TAG_IDLIST , false >","",3176,24},{"adoptedStandAloneAI","idList < idManagedClassPtr < idEntity > , TAG_IDLIST , false >","",3952,24}},true};
    result["idEncounterSpawn_t"]={104,"",{{"actor","idManagedClassPtr < idEntity >","",8,32}},true};
    result["idManagedClassPtr < idEntity >"]={32,"",{{"ptr","idEntity","*",8,8},{"spawnId","idSpawnId","",0,4}},true};
    result["idAI2"]={65536,"idActor",{{"aiHealth","idAIHealth","",25840,16+stride*2+128},{"aiVolatile","idAIVolatile","*",0x400,8}},true};
    result["aiPainState_t"]={3544,"",{{"currentPainReaction","painType_t","",0,4},{"finishStaggerTime","idTypesafeTime < long long , gameTimeUnique_t , 999960 >","",0xc0,8},{"staggerLength","idTypesafeTime < long long , gameTimeUnique_t , 999960 >","",0xc8,8}},true};
    result[health]={16+stride*2+128,"idBaseHealth",{{"components",component,"[2]",16,stride*2},{"isDead","bool","",16+stride*2+64,1}},true};
    result["idAIHealth"]={16+stride*2+128,"idShieldHealthT",{},true};
    result["idShieldHealthT"]={16+stride*2+128,health,{},true};
    result[component]={stride,"",{{"type","aiHealthComponent_t","",0,4},{"max","float","",4,4},{"cur","float","",12,4}},true};
    result["idPhysics"]={128,"",{{"mapInstance","idMapInstance","*",112,8}},true};
    result["idHavokPhysics_Base"]={520,"idPhysics",{{"bodyBoundsWorld","idBounds","",224,24}},true};
    return result;
}
int main() {
    for(const auto* selected:{&kProfile10,&kProfile11,&kProfile51}) {
        uint32_t stride=selected==&kProfile51?96u:88u;
        auto schemas=makeSchemas(stride); auto lookup=[&](const std::string& name)->const GraphType& { auto found=schemas.find(name); static const GraphType empty; return found==schemas.end()?empty:found->second; };
        auto layout=resolveEnemyTelemetryLayout(lookup); expect(layout.valid&&layout.componentSize==stride,"health component stride comes from reflection");
        const auto& profile=*selected; auto* base=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x7000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)); expect(base!=nullptr,"base allocation"); uintptr_t b=reinterpret_cast<uintptr_t>(base);
        std::vector<unsigned char> system(96),map(std::max(profile.mapEntitiesOffset+16,profile.mapPlayerOffset+8)),manager(4264),actor(65536),physics(520),havok(0x210b8),aiVolatile(0x2000);
        uintptr_t world=1;
        struct Entry { uint32_t id,generation; uintptr_t entity,previous,next; }; std::array<Entry,2> entries{{{12,3,reinterpret_cast<uintptr_t>(manager.data()),0,0},{33,4,reinterpret_cast<uintptr_t>(actor.data()),0,0}}};
        std::array<unsigned char,208> spawns{}; std::array<unsigned char,32> adopted{};
        put(base,profile.gameSystemRva,reinterpret_cast<uintptr_t>(system.data())); put(system.data(),0x44,uint32_t(2)); put(system.data(),0x50,reinterpret_cast<uintptr_t>(map.data())); put(map.data(),profile.mapEntitiesOffset,reinterpret_cast<uintptr_t>(entries.data())); put(map.data(),profile.mapEntitiesOffset+8,uint32_t(2));
        const char* mapName="test_map"; put(map.data(),0x20,reinterpret_cast<uintptr_t>(mapName)); put(map.data(),0x28,uint32_t(strlen(mapName)));
        put(map.data(),profile.mapPlayerOffset,reinterpret_cast<uintptr_t>(actor.data())); put(physics.data(),0x80,reinterpret_cast<uintptr_t>(havok.data())); put(havok.data(),0x210b0,reinterpret_cast<uintptr_t>(&world));
        const unsigned char getter[]{0x48,0x8d,0x05,0,0,0,0,0xc3};
        const char* typeNames[]{"idEncounterManager","idAI2"};
        for(unsigned i=0;i<2;++i) { uint32_t vt=0x200000+i*0x100,g=0x210000+i*0x100,info=0x220000+i*0x100; memcpy(base+g,getter,8); put(base,g+3,int32_t(info-g-7)); put(base,vt+16,b+g); put(base,info,reinterpret_cast<uintptr_t>(typeNames[i])); put(i?actor.data():manager.data(),0,b+vt); }
        const char* managerName="my_encounter",*actorName="imp_42"; put(manager.data(),72,reinterpret_cast<uintptr_t>(managerName)); put(actor.data(),72,reinterpret_cast<uintptr_t>(actorName)); put(manager.data(),layout.think,int32_t(1));
        put(manager.data(),3176,reinterpret_cast<uintptr_t>(spawns.data())); put(manager.data(),3184,int32_t(2)); put(manager.data(),3952,reinterpret_cast<uintptr_t>(adopted.data())); put(manager.data(),3960,int32_t(1));
        for(unsigned i=0;i<2;++i) { put(spawns.data(),i*104+8,uint32_t(100)); put(spawns.data(),i*104+16,reinterpret_cast<uintptr_t>(actor.data())); } put(adopted.data(),0,uint32_t(100)); put(adopted.data(),8,reinterpret_cast<uintptr_t>(actor.data()));
        put(actor.data(),layout.health+layout.components,int32_t(0)); put(actor.data(),layout.health+layout.components+4,125.0f); put(actor.data(),layout.health+layout.components+12,75.0f);
        put(actor.data(),layout.health+layout.components+stride,int32_t(1)); put(actor.data(),layout.health+layout.components+stride+4,999.0f); put(actor.data(),layout.health+layout.components+stride+12,900.0f);
        put(actor.data(),464,reinterpret_cast<uintptr_t>(physics.data())); put(physics.data(),112,reinterpret_cast<uintptr_t>(map.data())); float bounds[]{1,2,3,5,6,8}; memcpy(physics.data()+224,bounds,24);
        const uint32_t painOffset=0x100; put(actor.data(),layout.staggerVolatile,reinterpret_cast<uintptr_t>(aiVolatile.data()));
        unsigned char painGetter[]{0x48,0x8b,0x81,0,0,0,0,0x48,0x05,0,0,0,0,0xc3};
        unsigned char finishGetter[]{0x48,0x8b,0x81,0,0,0,0,0x48,0x8b,0x88,0,0,0,0,0x48,0x8b,0xc2,0x48,0x89,0x0a,0xc3};
        memcpy(painGetter+3,&layout.staggerVolatile,4); memcpy(painGetter+9,&painOffset,4); memcpy(finishGetter+3,&layout.staggerVolatile,4);
        uint32_t deadlineOffset=painOffset+layout.staggerFinish; memcpy(finishGetter+10,&deadlineOffset,4);
        uint32_t painGetterRva=selected==&kProfile10?0xbb6fc0:selected==&kProfile11?0xbc3f00:0xc8ceb0;
        uint32_t finishGetterRva=selected==&kProfile10?0xbb7050:selected==&kProfile11?0xbc3fb0:0xc8cf40;
        memcpy(base+painGetterRva,painGetter,sizeof(painGetter)); memcpy(base+finishGetterRva,finishGetter,sizeof(finishGetter));
        uint32_t resolvedPain=~0u; expect(resolveEnemyStaggerPointer(WallMemory{},b,profile,layout,resolvedPain)&&resolvedPain==painOffset,"each audited 1.0,1.1,5.1 getter pair resolves its reflected pain state");
        auto wrongProfile=profile; wrongProfile.imageSize+=0x1000; expect(!resolveEnemyStaggerPointer(WallMemory{},b,wrongProfile,layout,resolvedPain),"a version label or timestamp alone cannot authorize another executable's getters");
        LevelIdentity level{reinterpret_cast<uintptr_t>(map.data()),1,"test_map"};
        auto capture=captureEnemyTelemetry(WallMemory{},b,profile,level,lookup); expect(capture.state=="ready"&&capture.enemies.size()==1,"exact spawn and adopted memberships deduplicate per manager");
        auto row=capture.enemies.front(); expect(row.health==75&&row.maximumKnown&&row.maximum==125&&row.entity.id==33&&row.encounterId==12,"body health and both entity generations retained"); expect(row.positionKnown&&row.top.z>8&&row.top.x==3,"bar top derives from current world bounds");
        expect(row.staggerKnown&&!row.staggerActive,"normal pain reaction has no glory-kill window");
        // Actual 1.0 inputs from the user's confirmed blinking/glory-kill
        // demonstrations in stagger-positive-watch-retry.tsv. The old watch
        // classified these as inactive because it incorrectly required 17.
        struct RecordedWindow { int64_t clock,deadline; double remaining; };
        for(const auto& observed:std::array<RecordedWindow,5>{{
            {1160981410,1163496338,2.5150286011440457},
            {1161973904,1163496338,1.5224948997959917},
            {1165069948,1170013637,4.943886755470219},
            {1166177609,1170013637,3.83618144725789},
            {1166225635,1170013637,3.7881535261410457}
        }}) {
            put(aiVolatile.data(),painOffset+layout.staggerReaction,int32_t(16));
            put(aiVolatile.data(),painOffset+layout.staggerFinish,observed.deadline);
            put(aiVolatile.data(),painOffset+layout.staggerLength,int64_t(4999800));
            expect(refreshEnemyStagger(WallMemory{},b,profile,row,observed.clock)&&row.staggerKnown&&row.staggerActive&&
                std::abs(row.staggerSeconds-observed.remaining)<1e-9,"real recorded reaction16 glory-kill inputs produce the measured deadline-minus-clock countdown");
        }
        put(aiVolatile.data(),painOffset+layout.staggerReaction,int32_t(16)); put(aiVolatile.data(),painOffset+layout.staggerFinish,int64_t(9999600)); put(aiVolatile.data(),painOffset+layout.staggerLength,int64_t(3999840));
        expect(refreshEnemyStagger(WallMemory{},b,profile,row,7999680)&&row.staggerKnown&&row.staggerActive&&row.staggerSeconds==2,"observed glory-kill stagger16 uses the engine deadline and scaled game clock");
        put(aiVolatile.data(),painOffset+layout.staggerReaction,int32_t(17));
        expect(refreshEnemyStagger(WallMemory{},b,profile,row,7999680)&&row.staggerKnown&&row.staggerActive&&row.staggerSeconds==2,"vulnerable stagger17 uses the same audited deadline");
        expect(refreshEnemyStagger(WallMemory{},b,profile,row,7999680)&&row.staggerSeconds==2,"paused game clock preserves the remaining window");
        expect(refreshEnemyStagger(WallMemory{},b,profile,row,9999600)&&!row.staggerActive&&row.staggerSeconds==0,"expired deadline removes the execution countdown");
        expect(!refreshEnemyStagger(WallMemory{},b,profile,row)&&!row.staggerKnown,"unknown game clock cannot become a guessed countdown");
        for(int32_t reaction:{1,3,7,13}) { put(aiVolatile.data(),painOffset+layout.staggerReaction,reaction); expect(refreshEnemyStagger(WallMemory{},b,profile,row,7999680)&&row.staggerKnown&&!row.staggerActive,"flinch, falter and stun never become glory-kill windows"); }
        put(aiVolatile.data(),painOffset+layout.staggerReaction,int32_t(0));
        base[finishGetterRva+10]^=4; auto changed=captureEnemyTelemetry(WallMemory{},b,profile,level,lookup); expect(changed.enemies.size()==1&&!changed.enemies.front().staggerKnown,"changed engine getter disables only the unsupported countdown"); base[finishGetterRva+10]^=4;
        put(physics.data(),224+20,11.0f); Vec3 fresh{}; expect(refreshEnemyTelemetryTop(WallMemory{},b,profile,row,fresh)&&fresh.z>11,"render pose refresh follows bounds without a new snapshot");
        ++entries[1].generation; expect(!refreshEnemyTelemetryTop(WallMemory{},b,profile,row,fresh),"reused map slots cannot retain an old HP bar"); --entries[1].generation;
        put(actor.data(),layout.health+layout.dead,uint8_t(1)); expect(captureEnemyTelemetry(WallMemory{},b,profile,level,lookup).enemies.empty(),"dead AI excluded despite positive health"); put(actor.data(),layout.health+layout.dead,uint8_t(0));
        put(actor.data(),layout.health+layout.components+12,std::numeric_limits<float>::quiet_NaN()); capture=captureEnemyTelemetry(WallMemory{},b,profile,level,lookup); expect(capture.enemies.empty()&&capture.state=="partial","unreadable/nonfinite health never becomes a zero-health enemy"); put(actor.data(),layout.health+layout.components+12,75.0f);
        put(manager.data(),layout.think,int32_t(0)); expect(captureEnemyTelemetry(WallMemory{},b,profile,level,lookup).enemies.empty(),"inactive managers do not contribute enemies"); put(manager.data(),layout.think,int32_t(1));
        put(manager.data(),layout.think,int32_t(0)); auto standalone=captureEnemyTelemetry(WallMemory{},b,profile,level,lookup,true);
        expect(standalone.enemies.empty()&&standalone.barEnemies.size()==1&&standalone.barEnemies.front().encounter.empty(),"standalone live AI receives a bar without inventing encounter membership");
        put(manager.data(),layout.think,int32_t(1));
        actor[0xa4]=1; expect(captureEnemyTelemetry(WallMemory{},b,profile,level,lookup).enemies.empty(),"removed actors excluded"); actor[0xa4]=0;
        const auto originalHealth=schemas["idAIHealth"];
        schemas["idAIHealth"].size+=16; schemas["idAI2"].fields.front().size+=16;
        expect(resolveEnemyTelemetryLayout(lookup).valid,"larger derived health retains the bounded base layout");
        schemas["idAIHealth"].parent="unrelatedHealth"; schemas["unrelatedHealth"]={16+stride*2+128,"",{},true};
        expect(!resolveEnemyTelemetryLayout(lookup).valid&&!resolveEnemyTelemetryLayout(lookup).pending,"unrelated health type cannot borrow another template offsets");
        schemas["idAIHealth"]=originalHealth; schemas["idAI2"].fields.front().size-=16;
        schemas.erase("idAI2"); expect(captureEnemyTelemetry(WallMemory{},b,profile,level,lookup).state=="metadata_pending","missing metadata is explicit"); VirtualFree(base,0,MEM_RELEASE);
    }
    puts("PASS: exact encounter memberships, 1.0/1.1/5.1 audited getters and health strides, standalone bars, stale identities, recorded glory-kill reaction16, stagger17, paused clocks, flinch rejection and guarded deadlines.");
}
