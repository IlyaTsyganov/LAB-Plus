#pragma once

struct AuthoredPreviewFixture {
    std::vector<std::unique_ptr<std::vector<unsigned char>>> allocations;
    std::unordered_map<std::string,GraphType> schemas;
    EntityReference target,blueprint;
    uintptr_t source=0,declList=0,def=0,web=0,caches=0;
    size_t gkCount=0,total=0;std::vector<uintptr_t> declarations;
    uintptr_t block(size_t n){allocations.push_back(std::make_unique<std::vector<unsigned char>>(n));return reinterpret_cast<uintptr_t>(allocations.back()->data());}
    template<class T> void put(uintptr_t p,const T& value){memcpy(reinterpret_cast<void*>(p),&value,sizeof(value));}
    uintptr_t text(const std::string& value){const auto p=block(160);memcpy(reinterpret_cast<void*>(p),value.data(),value.size());return p;}
    void list(uintptr_t at,uintptr_t data,int count){put(at,SyncPreviewList{data,count,count,0});}
    explicit AuthoredPreviewFixture(const std::string& family) {
        std::ifstream layouts("Native/CollisionViewer/tests/fixtures/sync-authored-layouts-11.tsv");std::string line;
        requireTarget(bool(layouts),"saved native authoring layouts exist");
        while(std::getline(layouts,line)){if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);char kind;std::string owner;fields>>kind>>owner;
            auto& schema=schemas[owner];schema.complete=true;if(kind=='T')fields>>schema.size;else{GraphField f;fields>>f.name>>f.offset>>f.size;std::getline(fields,f.type);f.type=graphTrim(f.type);schema.fields.push_back(f);}}
        target={block(0x6500),0x10000,0x20000,1,1,"idAI2"};source=target.address+0x4db0;
        def=block(0x100);const auto defs=block(8);put(defs,def);list(source+0x1b0,defs,1);
        std::vector<const SyncAuditedEnemyRule*> rules;for(const auto& r:syncAuditedEnemyRules)if(family==r.family){rules.push_back(&r);gkCount+=r.action==0;}
        total=rules.size();declList=block(total*8);list(source+0x608,declList,int(total));
        std::vector<std::string> states;for(const auto* r:rules)if(r->action==0&&std::find(states.begin(),states.end(),r->animation)==states.end())states.push_back(r->animation);
        for(size_t i=0;i<rules.size();++i){const auto& r=*rules[i];const auto decl=block(0x398),animation=block(48);declarations.push_back(decl);put(declList+i*8,decl);
            put(decl+8,text(r.declaration));put(decl+0x98,r.action?16:2);put(decl+0x1d8,static_cast<unsigned char>(1));
            put(decl+0x258,10.f);put(decl+0x260,3.f);put(decl+0x264,90.f);put(decl+0x268,60.f);put(decl+0x310,r.quadrants);
            put(animation,text("killedByPlayer"));put(animation+8,text(r.animation));list(decl+0x1b8,animation,1);
            std::istringstream groups(r.groups);std::string group;std::vector<std::string> names;while(std::getline(groups,group,'|'))if(!group.empty())names.push_back(group);
            const auto data=block(std::max<size_t>(8,names.size()*8));for(size_t j=0;j<names.size();++j)put(data+j*8,text(names[j]));list(decl+0x2a0,data,int(names.size()));}
        blueprint={block(0x2ce0),0x10000,0x30000,2,1,"idSyncEntity"};put(blueprint.address+0xa8,def);
        web=block(0x1d8);put(blueprint.address+0x1bf8,web);const auto parts=block(368);list(blueprint.address+0x1c18,parts,2);
        put(parts+0xa0,0);put(parts+0xaa,short(0));put(parts+184+0xa0,1);put(parts+184+0xaa,short(1));
        const auto nodes=block(states.size()*8),stateNames=block(states.size()*8),subwebs=block(56);put(subwebs+24,text("killedByPlayer"));
        list(web+0x78,nodes,int(states.size()));list(web+0xc0,stateNames,int(states.size()));list(web+0x90,subwebs,1);
        caches=block(states.size()*0x3030);list(blueprint.address+0x2c08,caches,int(states.size()));
        const float instigator[12]{1,0,0,-2,0,1,0,0,0,0,1,0},victim[12]{1,0,0,0,0,1,0,0,0,0,1,0};
        for(size_t i=0;i<states.size();++i){const auto node=block(0xb0);put(nodes+i*8,node);put(node+0x30,short(i));put(node+0x34,short(i));put(stateNames+i*8,text(states[i]));
            const auto cache=caches+i*0x3030;put(cache,short(i));memcpy(reinterpret_cast<void*>(cache+0x10),instigator,48);memcpy(reinterpret_cast<void*>(cache+0x40),victim,48);put(cache+0x10+0x301c,static_cast<unsigned char>(1));}
    }
    GraphLookup lookup(){return [&](const std::string& n)->const GraphType&{return schemas[n];};}
};
static void checkAuthoredBeforeGkState() {
    std::unordered_set<std::string> families;for(const auto& rule:syncAuditedEnemyRules)families.insert(rule.family);
    for(const auto& family:families){AuthoredPreviewFixture fixture(family);auto lookup=fixture.lookup();
        const auto source=readSyncAuthoredSource(WallMemory{},fixture.target,lookup);
        requireTarget(source.known&&source.rows.size()==fixture.total,"every family's assigned authored rules are readable without any player candidates or GK state");
        for(const auto& row:source.rows)requireTarget(row.audit.matched,"source rules retain exact native declaration/state/aim audits before GK state");
        const auto geometry=readSyncAuthoredGeometry(WallMemory{},0,source,{fixture.blueprint},.5f,lookup);
        if(fixture.gkCount)requireTarget(geometry.known&&geometry.complete&&geometry.regions.size()==fixture.gkCount,"all ordinary assigned GK transforms can be read from an owned template before candidate preparation");
        for(const auto& region:geometry.regions)requireTarget(region.maxXY==10.5f&&region.start.x==-2.f,"reference uses native cached start translation and the current range modifier");
        auto foreign=source;foreign.blueprintDefinitions={0xdead};requireTarget(!readSyncAuthoredGeometry(WallMemory{},0,foreign,{fixture.blueprint},0,lookup).known,"a template assigned to a different native definition cannot supply enemy directions");
        fixture.put(fixture.source+0x608,SyncPreviewList{});const auto unassigned=readSyncAuthoredSource(WallMemory{},fixture.target,lookup);
        requireTarget(unassigned.known&&unassigned.rows.empty(),"catalog family names alone cannot add rules absent from the actual enemy's assigned list");
    }
    AuthoredPreviewFixture tyrant("tyrant");auto lookup=tyrant.lookup();auto source=readSyncAuthoredSource(WallMemory{},tyrant.target,lookup);
    auto copied=tyrant.blueprint;copied.address=tyrant.block(0x2ce0);memcpy(reinterpret_cast<void*>(copied.address),reinterpret_cast<void*>(tyrant.blueprint.address),0x2ce0);
    requireTarget(readSyncAuthoredGeometry(WallMemory{},0,source,{tyrant.blueprint,copied},0,lookup).known,"two templates with identical verified geometry need no arbitrary first/third person selection");
    const auto otherCache=tyrant.block(source.rows.size()*0x3030);memcpy(reinterpret_cast<void*>(otherCache),reinterpret_cast<void*>(tyrant.caches),source.rows.size()*0x3030);
    tyrant.put(copied.address+0x2c08+0,otherCache);tyrant.put(otherCache+0x10+3*4,-4.f);
    requireTarget(!readSyncAuthoredGeometry(WallMemory{},0,source,{tyrant.blueprint,copied},0,lookup).known,"conflicting first/third person starts remain unknown instead of choosing a template by name");
    tyrant.put(tyrant.caches+0x10+0x301c,static_cast<unsigned char>(0));const auto invalid=readSyncAuthoredGeometry(WallMemory{},0,source,{tyrant.blueprint},0,lookup);
    requireTarget(invalid.known&&!invalid.complete&&invalid.regions.size()<tyrant.gkCount,"invalid frame-zero data preserves the list while preventing a complete standing-area claim");
    printf("PASS: pre-GK authoring reader for all 25 families with synthetic cached poses; ownership and ambiguous-template guards.\n");
}
