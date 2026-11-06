#pragma once

// Read the target's own assigned declarations, independently of the player's
// temporary candidate pool. Blueprint identity comes from its native def list.
struct SyncAuthoredAnimation {uintptr_t declaration=0;std::string state,subweb;int action=0;SyncAimRequirement aim;SyncRuleAudit audit;};
struct SyncAuthoredSource {
    SyncPreviewList declarations{},definitions{};
    std::vector<uintptr_t> blueprintDefinitions;
    std::vector<SyncAuthoredAnimation> rows;
    bool known=false;
};
static bool readSyncAuthoredGkPriority(const WallMemory& memory,uintptr_t base,uintptr_t decl,float& priority) {
    const unsigned char copy[]{0xf3,0x0f,0x10,0x80,0x48,1,0,0,0xf3,0x0f,0x11,0x41,0x4c},
        equipment[]{0x49,0x8b,0x96,0xb8,2,0,0,0x48,0x3b,0x15,0x9c,0xf7,0xf3,0};unsigned char code[14]{},flags[3]{};
    uintptr_t type=0,name=0,empty=0,required=0,after=0;float last=0;
    if(!memory.get(decl+0x90,type)||type!=base+0x626d410||!memory.get(type,name)||readEntityText(memory,name,96)!="idSyncAttackInteraction_Generic"||
       !memory.read(base+0x18dab4d,code,sizeof(copy))||memcmp(code,copy,sizeof(copy))||
       !memory.read(base+0x16ba68e,code,sizeof(equipment))||memcmp(code,equipment,sizeof(equipment))||
       !memory.get(base+0x25f9e38,empty)||!empty||!memory.get(decl+0x2b8,required)||required!=empty||
       !memory.read(decl+0x314,flags,3)||flags[0]||flags[1]||flags[2]||
       !memory.get(decl+0x148,priority)||!std::isfinite(priority)||priority<0||priority>1024||
       !memory.get(decl+0x148,last)||last!=priority||!memory.get(decl+0x2b8,after)||after!=required)return false;
    return readSyncGkAiPriority(memory,base,decl,priority);
}
static SyncAuthoredSource readSyncAuthoredSource(const WallMemory& memory,const EntityReference& target,const GraphLookup& lookup) {
    SyncAuthoredSource result;
    struct Field {const char* owner;const char* name;uint32_t offset,size;const char* type;};
    for(const auto& e:{Field{"idAI2","aiConstants",0x4bb0,0x858,"idAI2::idAIConstant"},Field{"idAI2::idAIConstant","syncMelee",0x200,0x628,"idSyncAttack"},
        Field{"idSyncAttack","syncInteractionData",0x608,24,"idList < const idDeclSyncInteraction * , TAG_IDLIST , false >"},Field{"idSyncAttack","syncMeleeEntityDefs",0x1b0,24,"idList < const idDeclEntityDef * , TAG_IDLIST , false >"},
        Field{"idDeclSyncInteraction","animations",0x1b8,32,"idDeclSyncInteraction::animationData_t"},Field{"idDeclSyncInteraction::animationData_t","anims",0,24,"idList < syncAnimationInfo_t , TAG_IDLIST , false >"},
        Field{"syncAnimationInfo_t","subwebName",0,8,"idAtomicString"},Field{"syncAnimationInfo_t","stateName",8,8,"idAtomicString"}}) {
        const auto& schema=lookup(e.owner);const auto* field=enemyTelemetryField(schema,e.name);
        if(!schema.complete||!field||field->offset!=e.offset||field->size!=e.size||field->type!=e.type||!field->ops.empty())return result;
    }
    const uintptr_t source=target.address+0x4db0;
    if(!syncPreviewList(memory,source+0x608,512,result.declarations)||!syncPreviewList(memory,source+0x1b0,32,result.definitions))return result;
    std::vector<uintptr_t> decls(result.declarations.count),defs(result.definitions.count),again;
    if((!decls.empty()&&!memory.read(result.declarations.data,decls.data(),decls.size()*8))||
       (!defs.empty()&&!memory.read(result.definitions.data,defs.data(),defs.size()*8)))return result;
    if(std::any_of(defs.begin(),defs.end(),[](uintptr_t p){return !p;}))return {};
    for(auto decl:decls) {
        int inputs[2]{},type=-1;unsigned char flags[3]{};SyncPreviewList animations;
        if(!decl||!memory.read(decl+0x98,inputs,8)||!memory.get(decl+0xa8,type)||!memory.read(decl+0x314,flags,3))return {};
        if(type!=0||flags[0]||flags[1]||flags[2])continue;
        if(!syncPreviewList(memory,decl+0x1b8,32,animations))return {};
        const auto aim=readSyncAimRequirement(memory,decl);
        for(int i=0;i<animations.count;++i) {
            std::string state,subweb;const auto at=animations.data+size_t(i)*48;
            if(!syncPreviewString(memory,at,subweb)||!syncPreviewString(memory,at+8,state))return {};
            if(state.empty())continue;
            for(int action:{0,1})if(syncInputMatches(inputs,action?16:2))
                result.rows.push_back({decl,state,subweb,action,aim,readSyncRuleAudit(memory,decl,state,action,aim)});
        }
        if(result.rows.size()>128)return {};
        SyncPreviewList final;if(!memory.get(decl+0x1b8,final)||memcmp(&animations,&final,24))return {};
    }
    SyncPreviewList finalDecls,finalDefs;
    if(!memory.get(source+0x608,finalDecls)||memcmp(&result.declarations,&finalDecls,24)||
       !memory.get(source+0x1b0,finalDefs)||memcmp(&result.definitions,&finalDefs,24))return {};
    again.resize(decls.size());if(!again.empty()&&(!memory.read(result.declarations.data,again.data(),again.size()*8)||again!=decls))return {};
    again.resize(defs.size());if(!again.empty()&&(!memory.read(result.definitions.data,again.data(),again.size()*8)||again!=defs))return {};
    result.blueprintDefinitions=std::move(defs);result.known=true;return result;
}
static bool syncAuthoredNodeLayouts(const GraphLookup& lookup) {
    struct Field {const char* owner;const char* name;uint32_t offset,size;const char* type;};
    for(const auto& e:{Field{"idDeclAnimWeb","nodes",0x78,24,"idList < idDeclAnimWebNode * , TAG_DECL_ANIMWEB , false >"},Field{"idDeclAnimWeb","states",0xc0,24,"idList < idAtomicString , TAG_DECL_ANIMWEB , false >"},
        Field{"idDeclAnimWeb","subWebs",0x90,24,"idList < idDeclAnimWeb::idSubWebInfo , TAG_DECL_ANIMWEB , false >"},Field{"idDeclAnimWeb::idSubWebInfo","name",24,8,"idAtomicString"},
        Field{"idDeclAnimWebNode","subWebIndex",0x32,2,"idIndex < short , idAnimWeb::invalidSubWebIndex_t , - 1 >"},Field{"idDeclAnimWebNode","stateIndex",0x34,2,"idIndex < short , idAnimWeb::invalidStateIndex_t , - 1 >"}}) {
        const auto& schema=lookup(e.owner);const auto* field=enemyTelemetryField(schema,e.name);
        if(!schema.complete||!field||field->offset!=e.offset||field->size!=e.size||field->type!=e.type||!field->ops.empty())return false;
    }return lookup("idDeclAnimWeb::idSubWebInfo").size==56;
}
struct SyncAuthoredGeometry {EntityReference blueprint;std::vector<SyncFacingRegion> regions;bool known=false,complete=false;};
static SyncAuthoredGeometry readSyncAuthoredGeometry(const WallMemory& memory,uintptr_t base,const SyncAuthoredSource& source,
        const std::vector<EntityReference>& blueprints,float modifier,const GraphLookup& lookup) {
    SyncAuthoredGeometry result;if(!source.known||!std::isfinite(modifier)||!syncAuthoredNodeLayouts(lookup))return result;
    for(const auto& ref:blueprints) {
        uintptr_t definition=0,web=0;unsigned char facing[2]{1,1};SyncPreviewList parts,caches,nodes,states,subwebs;
        if(!memory.get(ref.address+0xa8,definition)||std::find(source.blueprintDefinitions.begin(),source.blueprintDefinitions.end(),definition)==source.blueprintDefinitions.end()||
           !memory.get(ref.address+0x1bf8,web)||!web||!memory.read(ref.address+0x2bd0,facing,2)||facing[0]||facing[1]||
           !syncPreviewList(memory,ref.address+0x1c18,32,parts)||!syncPreviewList(memory,ref.address+0x2c08,128,caches)||
           !syncPreviewList(memory,web+0x78,512,nodes)||!syncPreviewList(memory,web+0xc0,512,states)||!syncPreviewList(memory,web+0x90,128,subwebs))continue;
        short instigator=-1,target=-1;bool valid=true;
        for(int i=0;i<parts.count;++i){int role=-1;short joint=-1;const auto at=parts.data+size_t(i)*184;
            if(!memory.get(at+0xa0,role)||!memory.get(at+0xaa,joint)){valid=false;break;}
            if(role==0){if(instigator!=-1)valid=false;instigator=joint;}if(role==1){if(target!=-1)valid=false;target=joint;}}
        if(!valid||instigator<0||target<0)continue;
        std::vector<SyncFacingRegion> regions;bool complete=true,covered=true;size_t ordinary=0;
        for(const auto& row:source.rows)if(row.action==0) {
            ++ordinary;uintptr_t node=0,joints=0;short nodeIndex=-1;
            for(int i=0;i<nodes.count;++i){uintptr_t p=0;short indices[3]{};std::string state,subweb;
                if(!memory.get(nodes.data+size_t(i)*8,p)||!p||!memory.read(p+0x30,indices,6)||indices[0]<0||indices[1]<0||indices[1]>=subwebs.count||indices[2]<0||indices[2]>=states.count||
                   !syncPreviewString(memory,states.data+size_t(indices[2])*8,state)||!syncPreviewString(memory,subwebs.data+size_t(indices[1])*56+24,subweb)){valid=false;break;}
                if(state==row.state&&subweb==row.subweb){if(node){valid=false;break;}node=p;nodeIndex=indices[0];}}
            if(!valid)break;
            if(!node){covered=false;break;}
            for(int i=0;i<caches.count;++i){short index=-1;const auto at=caches.data+size_t(i)*0x3030;
                if(!memory.get(at,index)){valid=false;break;}if(index==nodeIndex){if(joints){valid=false;break;}joints=at+0x10;}}
            SyncFacingRegion region;
            if(!valid)break;
            if(!row.audit.matched||!joints||!readSyncFacingRegion(memory,row.declaration,node,joints,instigator,target,row.state,region)||
               region.maxXY+modifier<=0||region.maxXY+modifier>4096){complete=false;continue;}
            region.audit=row.audit;region.maxXY+=modifier;
            region.priorityKnown=readSyncAuthoredGkPriority(memory,base,row.declaration,region.priority);
            regions.push_back(std::move(region));
        }
        SyncPreviewList check;uintptr_t finalWeb=0,finalDef=0;
        if(!valid||!covered||!ordinary||!memory.get(ref.address+0xa8,finalDef)||finalDef!=definition||!memory.get(ref.address+0x1bf8,finalWeb)||finalWeb!=web)continue;
        bool coherent=true;for(const auto& pair:{std::make_pair(ref.address+0x1c18,parts),std::make_pair(ref.address+0x2c08,caches),
            std::make_pair(web+0x78,nodes),std::make_pair(web+0xc0,states),std::make_pair(web+0x90,subwebs)})
            coherent&=memory.get(pair.first,check)&&!memcmp(&check,&pair.second,24);
        if(!coherent)continue;
        // Multiple fully matching templates need an independently established
        // native choice. Never select a first/third person template by its name.
        if(result.known){
            if(result.complete!=complete||result.regions.size()!=regions.size())return {};
            for(size_t i=0;i<regions.size();++i)if(result.regions[i].name!=regions[i].name||!syncSameFacingRule(result.regions[i],regions[i]))return {};
            continue;
        }
        result={ref,std::move(regions),true,complete};
    }return result;
}
