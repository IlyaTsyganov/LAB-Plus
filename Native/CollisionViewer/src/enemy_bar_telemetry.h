#pragma once

// Optional health-bar metadata is resolved separately from the encounter HUD.
struct EnemyBarLayout {
    uint32_t definition=~0u,resourceName=~0u,atomicText=~0u,thinkFlags=~0u;
    uint32_t staggerEnabled=~0u,behavior=~0u,damageGraph=~0u,links=~0u;
    uint32_t conditions=~0u,enabled=~0u,conditionType=~0u,conditionObject=~0u,conditionStride=0,typeName=~0u;
    uint32_t entries=~0u,entryStride=0,bucket=~0u,fraction=~0u,damageDecls=~0u,collections=~0u;
    uint32_t aiVolatile=~0u,components=~0u,componentCount=0,owner=~0u,componentDecl=~0u,armorGroups=~0u,groupStride=0,groupName=~0u,groupTag=~0u;
    uint32_t animator=~0u,renderModel=~0u,animDecl=~0u,props=~0u,propStride=0,tags=~0u,tagStride=0,tagName=~0u,tagJoint=~0u,tagTranslation=~0u;
    uint32_t model=~0u,skeleton=~0u,skeletonData=~0u,numJoints=~0u,jointHandles=~0u;
    uint32_t khanStageOneMax=~0u,khanStageTwoMax=~0u;
    EnemyPartPose pose;
};

static EnemyBarLayout resolveEnemyBarLayout(const GraphLookup& lookup) {
    EnemyBarLayout l;
    auto field=[&](const char* owner,const char* name,uint32_t size)->const GraphField* {
        const auto& schema=lookup(owner);
        const auto* f=enemyTelemetryField(schema,name);
        return schema.complete&&f&&f->size==size?f:nullptr;
    };
    auto offset=[&](const char* owner,const char* name,uint32_t size) {
        const auto* f=field(owner,name,size);
        return f?f->offset:~0u;
    };
    auto stride=[&](const char* owner) {
        const auto& schema=lookup(owner);
        return schema.complete?schema.size:0;
    };
    auto chain=[](uint32_t a,uint32_t b) { return a==~0u||b==~0u?~0u:a+b; };

    l.definition=offset("idEntity","entityDef",8);
    l.thinkFlags=offset("idBloatedEntity","thinkFlags",4);
    l.atomicText=offset("idAtomicString","str",8);
    l.resourceName=chain(offset("idResource","name",8),l.atomicText);
    const auto editable=offset("idAI2","aiEditable",stride("idAIEditable"));
    l.staggerEnabled=chain(editable,offset("idAIEditable","staggerEnabled",1));
    l.behavior=chain(editable,chain(offset("idAIEditable","behaviors",stride("idAIBehaviors")),offset("idAIBehaviors","decl",8)));
    l.damageGraph=chain(offset("idDeclAIBehavior","damageBehaviors",stride("idDeclAIBehavior::idDamageBehaviors")),
                        offset("idDeclAIBehavior::idDamageBehaviors","damageStateGraph",8));
    l.links=offset("idDeclTypeInfoGraph","links",24);
    l.conditions=offset("idDeclAIDamageStateLink","conditions",24);
    l.enabled=offset("idDeclAIDamageStateLink","enabled",1);
    constexpr auto condition="idTypeInfoObjectPtr < idAIDamageState_LinkCondition >";
    l.conditionStride=stride(condition);
    l.conditionType=offset(condition,"typeInfo",8);
    l.conditionObject=offset(condition,"object",8);
    l.typeName=offset("idTypeInfo","classname",8);
    l.entries=offset("idAIDamageState_LinkCondition_SDPS","entries",24);
    constexpr auto entry="aiDamageStateSDPSEntry_t";
    l.entryStride=stride(entry);
    l.bucket=offset(entry,"bucketType",4);
    l.fraction=offset(entry,"prerequisiteHealthFraction",4);
    l.damageDecls=offset(entry,"damageDecls",24);
    l.collections=offset(entry,"damageCollections",24);

    l.aiVolatile=offset("idAI2","aiVolatile",8);
    const auto& ai=lookup("idAI2");
    const auto* components=enemyTelemetryField(ai,"aiComponents");
    if(ai.complete&&components&&components->type.rfind("idArray < idAI2Component * , ",0)==0&&
       components->size%8==0&&components->size<=64*8) {
        l.components=components->offset;
        l.componentCount=components->size/8;
    }
    l.khanStageOneMax=offset("idDeclAIComponent_KhanMaykr","stage1ShieldHealth",4);
    l.khanStageTwoMax=offset("idDeclAIComponent_KhanMaykr","stage2ShieldHealth",4);
    l.owner=offset("idAI2Component","owner",8);
    l.componentDecl=offset("idAI2Component","componentDecl",8);
    l.armorGroups=offset("idDeclAIComponent_IconOfSin","armorGroups",24);
    constexpr auto group="idDeclAIComponent_IconOfSin::armorGroup_t";
    l.groupStride=stride(group);
    l.groupName=chain(offset(group,"armorGroupName",8),l.atomicText);
    l.groupTag=chain(offset(group,"targetingTagName",8),l.atomicText);

    const auto stack=offset("idAnimatedEntity","animStack",stride("idAnimStack"));
    l.animator=chain(stack,offset("idAnimStack","jointAnimator",8));
    l.renderModel=chain(stack,offset("idAnimStack","renderModel",8));
    l.animDecl=offset("idJointAnimator","decl",8);
    l.props=chain(offset("idDeclMD6","props",stride("idPropsCollection")),offset("idPropsCollection","props",24));
    l.propStride=stride("idPropInfo");
    l.tags=offset("idPropInfo","tags",24);
    l.tagStride=stride("idTagInfo");
    l.tagName=chain(offset("idTagInfo","tagName",8),l.atomicText);
    const auto tagData=offset("idTagInfo","tagData",stride("tagData_t"));
    l.tagJoint=chain(tagData,offset("tagData_t","parentJoint",2));
    l.tagTranslation=chain(tagData,offset("tagData_t","trans",12));
    l.model=offset("idDeclMD6","model",8);
    l.skeleton=offset("idMD6Model","skeleton",8);
    l.skeletonData=offset("idMD6Skel","data",8);
    l.numJoints=offset("idMD6SkelData","numJoints",2);
    l.jointHandles=offset("idMD6SkelData","jointHandleTblOffset",2);
    l.pose.animator=l.animator;
    l.pose.renderModel=l.renderModel;
    l.pose.animDecl=l.animDecl;
    l.pose.joints=offset("idJointAnimator","joints",32);
    l.pose.deferred=offset("idJointAnimator","currentDeferred",4);
    const auto parms=offset("idRenderModel","g",stride("idRenderModelParms"));
    l.pose.origin=chain(parms,offset("idRenderModelParms","origin",12));
    l.pose.axis=chain(parms,offset("idRenderModelParms","axis",36));
    l.pose.scale=chain(parms,offset("idRenderModelParms","scale",12));
    return l;
}

template<class T> static bool enemyBarRead(const WallMemory& memory,uintptr_t address,uint32_t offset,T& value) {
    return address&&offset!=~0u&&memory.get(address+offset,value);
}
static uintptr_t enemyBarPointer(const WallMemory& memory,uintptr_t address,uint32_t offset) {
    uintptr_t value=0;
    return enemyBarRead(memory,address,offset,value)?value:0;
}
struct EnemyBarList { uintptr_t data=0; int32_t count=0; };
static bool enemyBarList(const WallMemory& memory,uintptr_t address,uint32_t offset,EnemyBarList& list,int32_t limit) {
    return address&&offset!=~0u&&memory.get(address+offset,list.data)&&memory.get(address+offset+8,list.count)&&
        list.count>=0&&list.count<=limit&&(list.data||!list.count);
}

static double readEnemyStaggerFraction(const WallMemory& memory,const EnemyBarLayout& l,uintptr_t behavior) {
    EnemyBarList links;
    if(!l.conditionStride||!l.entryStride||
       !enemyBarList(memory,enemyBarPointer(memory,behavior,l.damageGraph),l.links,links,512)) return -1;
    double fraction=-1;
    for(int32_t i=0;i<links.count;++i) {
        const auto link=enemyBarPointer(memory,links.data,uint32_t(i)*8);
        uint8_t enabled=0;
        EnemyBarList conditions;
        if(!enemyBarRead(memory,link,l.enabled,enabled)||enabled!=1||!enemyBarList(memory,link,l.conditions,conditions,64)) continue;
        for(int32_t j=0;j<conditions.count;++j) {
            const auto condition=conditions.data+size_t(j)*l.conditionStride;
            const auto info=enemyBarPointer(memory,condition,l.conditionType);
            const auto name=readEntityText(memory,enemyBarPointer(memory,info,l.typeName),64);
            if(name!="idAIDamageState_LinkCondition_SDPS") continue;
            EnemyBarList entries;
            if(!enemyBarList(memory,enemyBarPointer(memory,condition,l.conditionObject),l.entries,entries,64)) continue;
            for(int32_t k=0;k<entries.count;++k) {
                const auto entry=entries.data+size_t(k)*l.entryStride;
                int32_t bucket=0,decls=-1,collections=-1;
                float hp=0;
                // Weapon-specific/cheat triggers can cause stagger at unrelated
                // health values. Use the general health gate for the bucket.
                if(!enemyBarRead(memory,entry,l.bucket,bucket)||(bucket!=16&&bucket!=17)||
                   l.damageDecls==~0u||l.collections==~0u||
                   !enemyBarRead(memory,entry,l.damageDecls+8,decls)||decls||
                   !enemyBarRead(memory,entry,l.collections+8,collections)||collections||
                   !enemyBarRead(memory,entry,l.fraction,hp)||!std::isfinite(hp)||hp<=0||hp>1) continue;
                fraction=std::max(fraction,double(hp));
            }
        }
    }
    return fraction;
}

static float enemyBarExtraHeight(std::string_view family) {
    if(family=="tyrant"||family=="tyrant_anim") return 1.7f;
    if(family=="archvile") return 1.5f;
    if(family=="revenant") return .7f;
    if(family=="khan_maykr") return .75f;
    if(family=="whiplash"||family=="whiplash_base"||family=="whiplash_spectre") return .75f;
    if(family=="mancubus_fire"||family=="mancubus_goo"||family=="mancubus_base") return .8f;
    if(family=="baron"||family.compare(0,6,"baron_")==0) return 1.05f;
    return 0;
}

struct EnemyBarComponent { uintptr_t address=0,decl=0; };
static EnemyBarComponent findEnemyBarComponent(const WallMemory& memory,const EnemyBarLayout& l,uintptr_t actor,const char* wanted) {
    for(uint32_t i=0;i<l.componentCount;++i) {
        const auto component=enemyBarPointer(memory,actor,l.components+i*8);
        if(!component||enemyBarPointer(memory,component,l.owner)!=actor) continue;
        const auto decl=enemyBarPointer(memory,component,l.componentDecl);
        if(readEntityText(memory,enemyBarPointer(memory,decl,l.resourceName),128)==wanted)
            return {component,decl};
    }
    return {};
}

template<size_t Size> static bool readEnemyBarDamageCode(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                                        uintptr_t component,uint8_t (&code)[Size],uintptr_t* entry=nullptr) {
    const auto table=enemyBarPointer(memory,component,0);
    if(table<base||table>=base+profile.imageSize-0xd8) return false;
    const auto damage=enemyBarPointer(memory,table,0xd0);
    if(damage<base||damage>=base+profile.typeGetterLimitRva||!memory.read(damage,code,Size)) return false;
    if(entry) *entry=damage;
    return true;
}

static bool readEnemyKhanShield(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const EnemyBarLayout& l,
                                EnemyTelemetryRow& row) {
    const auto component=findEnemyBarComponent(memory,l,row.entity.address,"aicomponent/khan_maykr/khan_maykr");
    uint8_t code[0x200]{};
    if(!readEnemyBarDamageCode(memory,base,profile,component.address,code)) return false;
    // Khan keeps the shield on the boss component, separate from idAIHealth.
    // Verify the damage path's read/modify/write and its stage flag before using either.
    constexpr uint8_t update[]{0,0x0f,0x57,0xc0,0xf3,0x0f,0x5c,0xce,0x0f,0x2f,0xc1,0xf3,0x0f,0x11};
    bool healthRead=false,stageRead=false;
    for(size_t i=0;i+28<=sizeof(code);++i) {
        if(code[i]==0xf3&&code[i+1]==0x0f&&code[i+2]==0x10&&(code[i+3]==0x8e||code[i+3]==0x8f)&&
           code[i+4]==0xc4&&!code[i+5]&&!code[i+6]&&!code[i+7]&&
           !memcmp(code+i+7,update,sizeof(update))&&code[i+21]==code[i+3]&&
           code[i+22]==0xc4&&!code[i+23]&&!code[i+24]&&!code[i+25]) healthRead=true;
        if((code[i]==0x40||code[i]==0x44)&&code[i+1]==0x38&&(code[i+2]==0xb6||code[i+2]==0xb7)&&
           code[i+3]==0x50&&code[i+4]==2&&!code[i+5]&&!code[i+6]) stageRead=true;
    }
    unsigned char stageOne=2,checked=2;
    int32_t maximum=0;
    float current=0;
    if(!healthRead||!stageRead||!memory.get(component.address+0x250,stageOne)||stageOne>1||
       !enemyBarRead(memory,component.decl,stageOne?l.khanStageOneMax:l.khanStageTwoMax,maximum)||maximum<=0||
       !memory.get(component.address+0xc4,current)||!std::isfinite(current)||
       !memory.get(component.address+0x250,checked)||checked!=stageOne) return false;
    row.health=std::clamp(double(current),0.0,double(maximum));
    row.maximum=maximum;
    row.maximumKnown=row.showEmpty=true;
    row.staggerVolatileOffset=~0u; // Blood Punch shield, no Glory Kill countdown.
    return true;
}

static bool applyEnemyDarkLordPhaseHealth(EnemyTelemetryRow& row) {
    if(!row.maximumKnown||!std::isfinite(row.maximum)||row.maximum<=0||
       !std::isfinite(row.health)||row.health<0) return false;
    // The Dark Lord's HUD shows a full phase when the meter is reset.
    // During the phase the actor's health field is used as an inverted meter.
    // PAIN_STAGGER forces the displayed HP to zero before the phase finisher.
    if(row.health!=row.maximum) {
        if(row.painReaction<0) return false;
        row.health=row.painReaction==16?0:std::clamp(row.maximum-row.health,0.0,row.maximum);
    }
    row.showEmpty=true;
    return true;
}

static bool readEnemySamurPhaseHealth(const WallMemory& memory,const EnemyTelemetryLayout& layout,EnemyTelemetryRow& row) {
    float body=0,bodyMax=0,shield=0,shieldMax=0;
    if(!readEnemyTelemetryHealthComponent(memory,row.entity.address,layout,0,body,bodyMax)||
       !readEnemyTelemetryHealthComponent(memory,row.entity.address,layout,1,shield,shieldMax)||
       !std::isfinite(body)||!std::isfinite(bodyMax)||!std::isfinite(shield)||!std::isfinite(shieldMax)||
       bodyMax<0||shieldMax<0) return false;
    // Samur sets the actor's pool maximum when entering each damage phase.
    // The native Samur HUD adds body and shield when showing health.
    const float maximum=bodyMax+shieldMax;
    if(maximum<=0||maximum>1e9f) return false;
    row.health=std::clamp(body+shield,0.f,maximum);
    row.maximum=maximum;
    row.maximumKnown=row.showEmpty=true;
    return true;
}

struct EnemyIconLayout {
    uint32_t list=0,stride=0,maximum=0,current=0;
    uint32_t phaseMemory=~0u;
    bool bodyHealth=false;
};
static constexpr char kIconPhaseTwoKey[]="stage2_active";

static uint32_t resolveEnemyIconPhaseMemory(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                           uintptr_t entry,const uint8_t* code,uint32_t aiVolatile) {
    // Follow the damage handler's read-only FSM memory getter. The phase
    // branch looks for an atomic key in the 256-slot key array at +0x400.
    constexpr uint8_t keyRef[]{0x48,0x8d,0x15};
    constexpr uint8_t ownerCall[]{0x48,0x8b,0x4f,0x08,0xe8};
    constexpr uint8_t keyArray[]{0x48,0x05,0,4,0,0};
    constexpr uint8_t slotCount[]{0x48,0x81,0xfa,0,1,0,0};
    bool keyFound=false;
    for(size_t i=0;i<0x80;++i) {
        int32_t delta=0;
        if(!memcmp(code+i,keyRef,sizeof(keyRef))) {
            memcpy(&delta,code+i+3,4);
            const uintptr_t address=entry+i+7+delta;
            char text[sizeof(kIconPhaseTwoKey)]{};
            if(address>=base&&address<base+profile.imageSize&&memory.read(address,text,sizeof(text))&&
               !memcmp(text,kIconPhaseTwoKey,sizeof(text))) keyFound=true;
        }
        if(!keyFound||memcmp(code+i,ownerCall,sizeof(ownerCall))) continue;
        memcpy(&delta,code+i+5,4);
        const uintptr_t getter=entry+i+9+delta;
        uint8_t bytes[15]{};
        if(getter<base||getter>=base+profile.typeGetterLimitRva||!memory.read(getter,bytes,sizeof(bytes))||
           memcmp(bytes,"\x48\x8b\x81",3)||memcmp(bytes+7,"\x48\x8b\x80",3)||bytes[14]!=0xc3) continue;
        uint32_t data=0,phaseMemory=0;
        memcpy(&data,bytes+3,4);
        memcpy(&phaseMemory,bytes+10,4);
        const auto* tail=code+i+9;
        const auto* end=tail+0x80;
        if(data==aiVolatile&&phaseMemory<0x200000&&
           std::search(tail,end,std::begin(keyArray),std::end(keyArray))!=end&&
           std::search(tail,end,std::begin(slotCount),std::end(slotCount))!=end) return phaseMemory;
    }
    return ~0u;
}

static EnemyIconLayout resolveEnemyIconLayout(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
                                             uintptr_t component,uint32_t aiVolatile) {
    // The private array is absent from reflection. On the verified Icon
    // component, check the damage handler for the list, stride and HP reads.
    uint8_t code[0xe00]{};
    uintptr_t entry=0;
    if(!readEnemyBarDamageCode(memory,base,profile,component,code,&entry)) return {};
    constexpr uint8_t early[]{0x4c,0x8b,0x8f,0xc8,1,0,0,0x49,0x83,0xca,0xff,0x49,0x69,0xf5,0x60,1,
        0,0,0x41,0x0f,0x28,0xe2,0x41,0x0f,0x28,0xdd,0x33,0xd2,0xf3,0x41,0x0f,0x10,0x84,0x31,0x80,
        0,0,0,0x45,0x8b,0x44,0x31,0x70,0x0f,0x28,0xc8,0xf3,0x41,0x0f,0x5c,0x8c,0x31,0x84,0,0,0};
    constexpr uint8_t lateIndex[]{0x48,0x8b,0x8f,0xc8,1,0,0,0x49,0x69,0xf5,0x60,1,0,0,0x45,0x84,0xe4,0x0f,0x84};
    constexpr uint8_t lateHealth[]{0xf3,0x0f,0x10,0x8c,0x0e,0x80,0,0,0,0x45,0x33,0xc0,0xf3,0x0f,0x10,0xac,0x0e,0x84,0,0,0};
    auto* end=std::begin(code)+0x480;
    bool layoutKnown=std::search(std::begin(code),end,std::begin(early),std::end(early))!=end;
    for(size_t i=0;!layoutKnown&&i+sizeof(lateIndex)+4+sizeof(lateHealth)<=0x480;++i)
        layoutKnown=!memcmp(code+i,lateIndex,sizeof(lateIndex))&&!memcmp(code+i+sizeof(lateIndex)+4,lateHealth,sizeof(lateHealth));
    if(!layoutKnown) return {};
    // Phase two writes into three successive damage pools on each part.
    // Confirm the indexed stores before reading the private record layout.
    bool bodyHealth=true;
    for(uint8_t offset:{uint8_t(0xb8),uint8_t(0xd8),uint8_t(0xdc)}) {
        const uint8_t oldStore[]{0xf3,0x42,0x0f,0x11,0x84,0x30,offset,0,0,0};
        const uint8_t newStore[]{0xf3,0x0f,0x11,0x9c,uint8_t(offset==0xdc?0x06:0x0e),offset,0,0,0};
        bodyHealth=bodyHealth&&(std::search(end,std::end(code),std::begin(oldStore),std::end(oldStore))!=std::end(code)||
            std::search(end,std::end(code),std::begin(newStore),std::end(newStore))!=std::end(code));
    }
    return {0x1c8,0x160,0x80,0x84,resolveEnemyIconPhaseMemory(memory,base,profile,entry,code,aiVolatile),bodyHealth};
}

static bool enemyIconPhaseTwo(const WallMemory& memory,uintptr_t actor,const EnemyBarLayout& layout,
                             const EnemyIconLayout& native,bool declaredPhaseTwo) {
    const auto data=enemyBarPointer(memory,actor,layout.aiVolatile);
    const auto fsm=enemyBarPointer(memory,data,native.phaseMemory);
    uintptr_t keys[256]{};
    if(!fsm||!memory.get(fsm+0x400,keys)) return declaredPhaseTwo;
    for(const auto key:keys) {
        char text[sizeof(kIconPhaseTwoKey)]{};
        if(key&&memory.read(key,text,sizeof(text))&&!memcmp(text,kIconPhaseTwoKey,sizeof(text))) return true;
    }
    return false;
}

static bool readEnemyIconPartHealth(const WallMemory& memory,uintptr_t part,const EnemyIconLayout& native,
                                   bool phaseTwo,float& hp,float& maximum) {
    if(!memory.get(part+native.maximum,maximum)||!std::isfinite(maximum)||maximum<=0||maximum>1e9f) return false;
    if(!phaseTwo) return memory.get(part+native.current,hp)&&std::isfinite(hp);
    if(!native.bodyHealth) return false;
    // A depleted pool may be negative after a hit. Later pools still count
    // toward the part's total until its damage state advances past them.
    uint32_t state=0,checked=0;
    if(!memory.get(part+0xb4,state)||state>3) return false;
    constexpr uint32_t pools[]{0xb8,0xd8,0xdc};
    hp=0;
    for(uint32_t i=state;i<std::size(pools);++i) {
        float remaining=0;
        if(!memory.get(part+pools[i],remaining)||!std::isfinite(remaining)) return false;
        hp+=std::max(0.f,remaining);
    }
    return std::isfinite(hp)&&memory.get(part+0xb4,checked)&&checked==state;
}

static bool refreshEnemyPartTop(const WallMemory& memory,const EnemyTelemetryRow& row,Vec3& top) {
    const auto& pose=row.partPose;
    const auto actor=row.entity.address;
    const auto animator=enemyBarPointer(memory,actor,pose.animator);
    const auto render=enemyBarPointer(memory,actor,pose.renderModel);
    if(!animator||!render||enemyBarPointer(memory,animator,pose.animDecl)!=pose.decl||pose.joints==~0u) return false;
    int32_t deferred=-1;
    if(!enemyBarRead(memory,animator,pose.deferred,deferred)||deferred<0||deferred>1) return false;
    const auto matrices=enemyBarPointer(memory,animator,pose.joints+deferred*8);
    Vec3 origin{},scale{};
    float axis[3][3]{},joint[3][4]{};
    if(!matrices||!memory.read(matrices+size_t(pose.joint)*sizeof(joint),joint,sizeof(joint))||
       !enemyBarRead(memory,render,pose.origin,origin)||!enemyBarRead(memory,render,pose.axis,axis)||
       !enemyBarRead(memory,render,pose.scale,scale)||!motionFinite(origin)||!motionFinite(scale)||
       scale.x<=0||scale.y<=0||scale.z<=0||scale.x>100||scale.y>100||scale.z>100) return false;
    for(const auto& basis:axis) {
        const float length=motionLength({basis[0],basis[1],basis[2]});
        if(!std::isfinite(length)||length<.9f||length>1.1f) return false;
    }
    auto transform=[&](unsigned i) {
        return joint[i][0]*pose.translation.x+joint[i][1]*pose.translation.y+joint[i][2]*pose.translation.z+joint[i][3];
    };
    const Vec3 local{transform(0)*scale.x,transform(1)*scale.y,transform(2)*scale.z};
    if(!motionFinite(local)) return false;
    top=motionAdd(origin,motionAdd(motionScale({axis[0][0],axis[0][1],axis[0][2]},local.x),
        motionAdd(motionScale({axis[1][0],axis[1][1],axis[1][2]},local.y),
                  motionScale({axis[2][0],axis[2][1],axis[2][2]},local.z))));
    top=motionAdd(top,motionScale({axis[0][0],axis[0][1],axis[0][2]},2.25f));
    return motionFinite(top);
}

static bool refreshEnemyBarTop(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const EnemyTelemetryRow& row,Vec3& top) {
    if(!row.part) {
        if(!refreshEnemyTelemetryTop(memory,base,profile,row,top)) return false;
        top.z+=row.extraHeight;
        return true;
    }
    unsigned char dead=2;
    return enemyTelemetryIdentity(memory,base,profile,row.entity)&&
        memory.get(row.entity.address+row.healthOffset+row.deadOffset,dead)&&!dead&&refreshEnemyPartTop(memory,row,top);
}

static void collectEnemyIconBars(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const EnemyBarLayout& l,
                                 const EnemyTelemetryRow& actor,bool declaredPhaseTwo,std::vector<EnemyTelemetryRow>& rows) {
    if(!l.groupStride||!l.propStride||!l.tagStride) return;
    const auto component=findEnemyBarComponent(memory,l,actor.entity.address,"aicomponent/iconofsin/icon_of_sin");
    const auto native=resolveEnemyIconLayout(memory,base,profile,component.address,l.aiVolatile);
    if(!native.stride) return;
    const bool phaseTwo=enemyIconPhaseTwo(memory,actor.entity.address,l,native,declaredPhaseTwo);
    EnemyBarList groups,runtime,props;
    const auto animator=enemyBarPointer(memory,actor.entity.address,l.animator);
    const auto decl=enemyBarPointer(memory,animator,l.animDecl);
    const auto model=enemyBarPointer(memory,decl,l.model);
    const auto skeleton=enemyBarPointer(memory,model,l.skeleton);
    const auto skeletonData=enemyBarPointer(memory,skeleton,l.skeletonData);
    uint16_t jointCount=0,handleOffset=0;
    if(!enemyBarList(memory,component.decl,l.armorGroups,groups,8)||!groups.count||
       !enemyBarList(memory,component.address,native.list,runtime,8)||runtime.count!=groups.count||
       !enemyBarList(memory,decl,l.props,props,64)||
       !enemyBarRead(memory,skeletonData,l.numJoints,jointCount)||!jointCount||jointCount>2048||
       !enemyBarRead(memory,skeletonData,l.jointHandles,handleOffset)||!handleOffset) return;
    std::vector<uint16_t> handles(jointCount);
    if(!memory.read(skeletonData+handleOffset,handles.data(),handles.size()*sizeof(uint16_t))) return;

    // Resolve the model tags in a single pass. Their parent joints drive the
    // anchors at render time, including when the boss swings an arm.
    std::unordered_map<uintptr_t,EnemyPartPose> targets;
    for(int32_t i=0;i<props.count;++i) {
        EnemyBarList tags;
        if(!enemyBarList(memory,props.data+size_t(i)*l.propStride,l.tags,tags,1024)) continue;
        for(int32_t j=0;j<tags.count;++j) {
            const auto tag=tags.data+size_t(j)*l.tagStride;
            const auto name=enemyBarPointer(memory,tag,l.tagName);
            auto pose=l.pose;
            uint16_t handle=0xffff;
            if(!name||!enemyBarRead(memory,tag,l.tagJoint,handle)||handle==0xffff||
               !enemyBarRead(memory,tag,l.tagTranslation,pose.translation)||!motionFinite(pose.translation)) continue;
            const auto joint=std::find(handles.begin(),handles.end(),handle);
            if(joint==handles.end()) continue;
            pose.joint=static_cast<uint16_t>(joint-handles.begin());
            pose.decl=decl;
            targets.try_emplace(name,pose);
        }
    }
    for(int32_t i=0;i<groups.count&&rows.size()<2048;++i) {
        const auto group=groups.data+size_t(i)*l.groupStride;
        const auto values=runtime.data+size_t(i)*native.stride;
        const auto name=enemyBarPointer(memory,group,l.groupName);
        const auto pose=targets.find(enemyBarPointer(memory,group,l.groupTag));
        float hp=0,maximum=0;
        if(!name||enemyBarPointer(memory,values,l.atomicText)!=name||pose==targets.end()||
           !readEnemyIconPartHealth(memory,values,native,phaseTwo,hp,maximum)) continue;
        auto row=actor;
        row.part=static_cast<uint8_t>(i+1);
        row.partPose=pose->second;
        row.health=std::clamp(hp,0.f,maximum);
        row.maximum=maximum;
        row.maximumKnown=row.positionKnown=row.showEmpty=true;
        rows.push_back(std::move(row));
    }
}

static void collectEnemyHealthBar(const WallMemory& memory,const EnemyTelemetryLayout& health,const EnemyBarLayout& l,
    uintptr_t base,const GameProfile& profile,
    EnemyTelemetryRow&& row,std::unordered_map<uintptr_t,double>& thresholds,std::vector<EnemyTelemetryRow>& bars) {
    const auto actor=row.entity.address;
    const auto behavior=enemyBarPointer(memory,actor,l.behavior);
    const auto behaviorName=readEntityText(memory,enemyBarPointer(memory,behavior,l.resourceName),128);
    std::string_view family;
    constexpr std::string_view prefix="behaviors/";
    if(behaviorName.compare(0,prefix.size(),prefix)==0)
        family=std::string_view(behaviorName).substr(prefix.size(),behaviorName.find('/',prefix.size())-prefix.size());
    const auto definition=enemyBarPointer(memory,actor,l.definition);
    const auto name=readEntityText(memory,enemyBarPointer(memory,definition,l.resourceName),160);
    const bool darkLord=behaviorName=="behaviors/darklord/default"||name=="ai/boss/darklord";
    const bool samur=behaviorName=="behaviors/samuel/default"||name=="ai/boss/samuel";
    if(!darkLord&&!samur&&row.health<=0) return;
    if(family=="icon_of_sin"||name=="ai/boss/icon_of_sin_base"||name=="ai/boss/icon_of_sin_stage2") {
        const bool declaredPhaseTwo=behaviorName=="behaviors/icon_of_sin/icon_of_sin_stage2"||name=="ai/boss/icon_of_sin_stage2";
        collectEnemyIconBars(memory,base,profile,l,row,declaredPhaseTwo,bars);
        return;
    }
    if(family=="khan_maykr"||name=="ai/boss/khan_maykr") {
        if(!readEnemyKhanShield(memory,base,profile,l,row)) return;
    } else if(samur) {
        int32_t think=0;
        if(!enemyBarRead(memory,actor,l.thinkFlags,think)||!think||
           !findEnemyBarComponent(memory,l,actor,"aicomponent/samuel/samuel").address||
           !readEnemySamurPhaseHealth(memory,health,row)) return;
    } else if(darkLord) {
        int32_t think=0;
        // The encounter also owns a dormant Dark Lord for the reveal scene.
        if(!enemyBarRead(memory,actor,l.thinkFlags,think)||!think||
           !findEnemyBarComponent(memory,l,actor,"aicomponent/dark_lord_health").address||
           !applyEnemyDarkLordPhaseHealth(row)) return;
    } else {
        unsigned char enabled=0;
        if(behavior&&row.maximumKnown&&enemyBarRead(memory,actor,l.staggerEnabled,enabled)&&enabled==1) {
            const auto [threshold,inserted]=thresholds.try_emplace(behavior,-1);
            if(inserted) threshold->second=readEnemyStaggerFraction(memory,l,behavior);
            if(threshold->second>0) row.staggerThreshold=row.maximum*threshold->second;
        }
    }
    if(behaviorName=="behaviors/darklord/summoned/baron") family="baron";
    row.extraHeight=enemyBarExtraHeight(family);
    if(!row.extraHeight) {
        const auto slash=name.find_last_of('/');
        row.extraHeight=enemyBarExtraHeight(std::string_view(name).substr(slash==std::string::npos?0:slash+1));
    }
    bars.push_back(std::move(row));
}
