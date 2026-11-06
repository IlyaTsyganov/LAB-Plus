#pragma once

#include "sync_facing_regions.h"
#include "sync_range_geometry.h"
#include "sync_aim_geometry.h"
#include "sync_enemy_display.h"
#include "sync_aim_parts.h"
#include "sync_position_guide.h"
#include "sync_exclusive_geometry.h"
#include "sync_sector_geometry.h"
#include "sync_clearance_band.h"

struct SyncTargetFrame {
    LevelIdentity level; EntityReference entity,blueprint;
    ULONGLONG captured=0;
    std::vector<SyncHitSphere> spheres;
    std::vector<std::string> gloryKills,chainsaw,chainsawAnimations;
    std::string selectedAnimation;
    std::string automaticAnimation,automaticDetail;bool automaticResolved=false;
    std::vector<SyncFacingRegion> regions;
    std::vector<SyncChainsawRule> chainsawRules;
    std::vector<SyncNativeChoice> nativeChoices;
    std::vector<SyncAnimationRule> animationRules;
    std::string enemyDefinition;
    SyncAimFrame aim;
    SyncJointFocus jointFocus;
    bool retainedTarget=false;
    SyncAimFrame retainedAim;SyncJointFocus retainedFocus;
    bool retainedSelectionKnown=false;float retainedSelectionTolerance=0;
    SyncFacingPose retainedSelectionPose;std::vector<SyncChainsawRule> retainedChainsawRules;
    SyncPlayerFootprint playerFootprint;
    float shotBounds[6]{};
    SyncFacingPose facing{};
    SyncRangeLimits ranges{};
    Vec3 viewPosition{},viewTarget{};
    int action=0;
    float incomingYaw=0;
    std::string regionState="waiting";
    std::string phase="waiting",group;
    float distance=0;
    bool candidatesReady=false,hasChainsaw=false;
    bool rulesCoherent=true;
    bool pendingGloryKill=false,pendingChainsaw=false;
    bool selectionKnown=false;
    float selectionTolerance=0;
    std::shared_ptr<const SyncExclusiveGuide> exclusiveGuide;
    std::shared_ptr<const SyncExclusiveGuide> referenceGuide;
    std::shared_ptr<const SyncSectorMesh> sectorMesh;
    std::shared_ptr<const SyncSectorMesh> originSectorMesh,clearanceBandMesh;
    std::shared_ptr<const SyncAimPartMesh> aimPartMesh;
    std::string aimPartAnimation;int aimPartAction=-1;EntityReference aimPartEntity;
    uint64_t aimPartSignature=0;
    SyncSectorTone sectorTone=SyncSectorTone::Reference;bool sectorToneKnown=false;
};
static bool syncPreviewProfile(const GameProfile& p) { return p.timestamp==0x5ec7deca&&p.imageSize==0x1d551000; }
static bool syncPreviewCode(const WallMemory& memory,uintptr_t base) {
    const unsigned char manager[]{0x48,0x8b,0x0d,0x10,0x0f,0x80,0x04},los[]{0x0f,0xb6,0x81,0xf8,0,0,0,0x83,0xe0,1,0xc3},
        chainsawBypass[]{0x74,0x52,0x83,0xb8,0x9c,0,0,0,0x10,0x74,0x49};
    unsigned char actual[sizeof(los)]{};
    return memory.read(base+0x1927801,actual,sizeof(manager))&&!memcmp(actual,manager,sizeof(manager))&&
        memory.read(base+0x1db9d60,actual,sizeof(los))&&!memcmp(actual,los,sizeof(los))&&
        memory.read(base+0x18db396,actual,sizeof(chainsawBypass))&&!memcmp(actual,chainsawBypass,sizeof(chainsawBypass));
}
static bool syncPreviewFinite(Vec3 p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&fabsf(p.x)<1e6f&&fabsf(p.y)<1e6f&&fabsf(p.z)<1e6f; }
static bool syncSphereHit(Vec3 eye,Vec3 direction,const SyncHitSphere& sphere,float& distance) {
    const auto delta=subtract(eye,sphere.center);
    const double a=dot(direction,direction),b=dot(delta,direction),c=dot(delta,delta)-double(sphere.radius)*sphere.radius;
    const double discriminant=b*b-a*c;
    if(!syncPreviewFinite(eye)||!syncPreviewFinite(direction)||!syncPreviewFinite(sphere.center)||
       !std::isfinite(sphere.radius)||sphere.radius<=0||a<.999||a>1.001||discriminant<0) return false;
    const double root=std::sqrt(discriminant),entryDistance=(-b-root)/a,exitDistance=(-b+root)/a;
    const double hit=entryDistance>=0?entryDistance:exitDistance;
    if(!std::isfinite(hit)||hit<0||hit>=distance) return false;
    distance=float(hit); return true;
}
static bool syncInputMatches(const int (&inputs)[2],int requested) {
    // Audited 1.1 idDeclSyncInteraction input filter (18d0720).
    for(int input:inputs) {
        const bool matches=input&&(input==18||(input==17&&requested)||input==requested||requested==18);
        if(input==16&&requested!=16) return false;
        if(matches) return true;
    }
    return false;
}
static bool syncPreviewLayouts(const GraphLookup& lookup) {
    struct Expected { const char* owner; const char* name; const char* type; uint32_t offset,size; const char* ops=""; };
    const Expected fields[]{
        {"idPlayer","syncMelee","idSyncAttack",0x70e8,0x628},
        {"idGameViewInfo","views","idList < gameView_t , TAG_IDLIST , false >",0x40,24},
        {"idGameViewInfo","resultsIndex","int",0x58,4},
        {"gameView_t","owner","idManagedClassPtr < idEntity >",0xc0,32},
        {"gameView_t","processedEntities","idArray < idList < gameViewEntInfo_t > , 2 >",0x60,48},
        {"gameView_t","resolvedProcessedEntities","idArray < idList < idEntity * > , 2 >",0x140,48},
        {"gameViewEntInfo_t","owner","idSpawnId",0xd0,4},
        {"idAI2","aiHealth","idAIHealth",0x63a0,280},
        {"idHealthT < aiHealthComponent_t , AI_HEALTH_MAX , AI_HEALTH_HITPOINTS >","isDead","bool",0xe8,1},
        {"idAnimatedEntity","animatedEntityVolatile","idAnimatedEntity::idAnimateEntityVolatile_t",0x1a10,112},
        {"idAnimatedEntity","targetingDecl","idDeclTargeting",0xbd0,8,"*"},
        {"idEntity","entityDef","idDeclEntityDef",0xa8,8,"*"},
        {"idResource","name","idAtomicString",8,8},
        {"idDeclTargeting","targetType","aim_targetType_t",0xc4,4},
        {"idAnimatedEntity::idAnimateEntityVolatile_t","sphereModelShape","idHavokShapePointer",0x18,8},
        {"idAnimatedEntity::idAnimateEntityVolatile_t","spherePhysicsObj","idHavokPhysics_SphereTree",0x28,8,"*"},
        {"idPhysics","mapInstance","idMapInstance",0x70,8,"*"},
        {"idHavokPhysics_Base","bodyAxis","idMat3",0x8c,36},
        {"idHavokPhysics_Base","bodyOrigin","idVec3",0xb0,12},
        {"idHavokPhysics_Base","bodyBoundsWorld","idBounds",0xe0,24},
        {"idHavokPhysics_Base","bodyBoundsLocal","idBounds",0xc8,24},
        {"idHavokPhysics_Player","currentShapeIndex","int",0x3cc0,4},
        {"idPlayer","jointFocusHistory","idList < int , TAG_IDLIST , false >",0x7768,24},
        {"idAnimatedEntity::idAnimateEntityVolatile_t","jointAnimator","idJointAnimator",0x10,8,"*"},
        {"idJointAnimator","decl","idDeclMD6",0,8,"*"},
        {"idDeclMD6","jointGroupCollection","idJointGroupCollection",0xe0,408},
        {"idJointGroup","joints","idList < idIndex < short , invalidJointIndex_t , - 1 > , TAG_ANIMATION , false >",0x10,24},
        {"idHavokPhysics_SphereTree","sphereShape","idHavokShapePointer",0x238,8},
        {"idHavokPhysics_SphereTree","lastUpdateTime","idTypesafeTime < long long , gameTimeUnique_t , 999960 >",0x240,8},
        {"idHavokShape","shape","hknpShape",0,8,"*"},
        {"idHavokShape","shapeData","idList < havokShapeData_t , TAG_HAVOK , false >",8,24},
        {"havokShapeData_t","contentFlags","int",8,4},
        {"havokShapeData_t","jointIndex","idIndex < short , invalidJointIndex_t , - 1 >",0x3a,2},
        {"havokShapeData_t","jointGroup","idJointGroup",0x50,8,"*"},
        {"idJointGroup","groupName","idAtomicString",8,8},
        {"idSyncAttack","possibleSyncInteractions","idList < idSyncAttackInteraction * , TAG_IDLIST , false >",0x178,24},
        {"idSyncAttackInteraction","syncInteractionDecl","idDeclSyncInteraction",0x40,8,"*"},
        {"idSyncAttackInteraction","syncValid","bool",0x48,1},
        {"idSyncAttackInteraction","syncPriority","float",0x4c,4},
        {"idSyncAttackInteraction","adjustedParticipantPosition","idVec3",0x84,12},
        {"idSyncAttackInteraction","waitingOnSubmittedTrace","bool",0x110,1},
        {"idSyncAttackInteraction","animationInfo","syncAnimationInfo_t",0xe0,48},
        {"idSyncAttackInteraction","targetEntity","idManagedClassPtr < idAnimatedEntity >",0x138,32},
        {"idManagedClassPtr < idAnimatedEntity >","ptr","idAnimatedEntity",8,8,"*"},
        {"idManagedClassPtr < idEntity >","ptr","idEntity",8,8,"*"},
        {"syncAnimationInfo_t","stateName","idAtomicString",8,8},
        {"idDeclSyncInteraction","syncInputs","idArray < playerInput_t , 2 >",0x98,8},
        {"idDeclSyncInteraction","syncType","syncType_t",0xa8,4},
        {"idDeclSyncInteraction","playerRequirements","idDeclSyncInteraction::playerRequirements_t",0x2a0,120},
        {"idDeclSyncInteraction::playerRequirements_t","requiredBerserk","bool",0x74,1},
        {"idDeclSyncInteraction","defaultSyncPriority","float",0x148,4},
        {"idDeclSyncInteraction::playerRequirements_t","requiredEquipt","idDeclInventoryRef",0x18,8},
        {"idDeclSyncInteraction::playerRequirements_t","requiredCrucible","bool",0x75,1},
        {"idDeclSyncInteraction::playerRequirements_t","requiredMomentum","bool",0x76,1},
        {"idDeclSyncInteraction::playerRequirements_t","focalScreenQuadrants","syncQuadrant_t",0x70,4},
        {"idDeclSyncInteraction::playerRequirements_t","damageGroupNames","idList < idAtomicString , TAG_IDLIST , false >",0,24},
        {"idRenderView","viewProjectionMatrix","idRenderMatrix",0x213f0,64},
        {"idSyncAttack","lastSyncEntity","idManagedClassPtr < const idSyncEntity >",0x48,32},
        {"idSyncEntity","animWeb","idDeclAnimWeb",0x1bf8,8,"*"},
        {"idSyncEntity","adjustToInterpolationEndPosition","bool",0x1caa,1},
        {"idSyncEntity","participants","idList < idSyncParticipant , TAG_GAME , false >",0x1c18,24},
        {"idSyncEntity","cachedAnimJoints","idList < idSyncEntity::joinCacheData_t , TAG_GAME , false >",0x2c08,24},
        {"idSyncEntity","facingRequirements","idSyncEntity::facingRequirement_t",0x2bc8,12},
        {"idSyncEntity::facingRequirement_t","requireSpecificTargetYaw","bool",8,1},
        {"idSyncEntity::facingRequirement_t","alignToInstigatorIgnoringTargetFacing","bool",9,1},
        {"idSyncParticipant","type","idSyncEntity::participantType_t",0xa0,4},
        {"idSyncParticipant","attachJoint","idIndex < short , invalidJointIndex_t , - 1 >",0xaa,2},
        {"idSyncEntity::joinCacheData_t","jointCache","idMD6Util::idJointCache",0x10,0x3020},
        {"idSyncEntity::joinCacheData_t","nodeIndex","idIndex < short , idAnimWeb::invalidNodeIndex_t , - 1 >",0,2},
        {"idMD6Util::idJointCache","jointMatrices","idJointMat",0,0x3000,"[256]"},
        {"idMD6Util::idJointCache","frameNum","int",0x3018,4},
        {"idMD6Util::idJointCache","validJointCache","bool",0x301c,1},
        {"idDeclAnimWebNode","nodeIndex","idIndex < short , idAnimWeb::invalidNodeIndex_t , - 1 >",0x30,2},
        {"syncAnimationInfo_t","animWeb","idDeclAnimWeb",0x20,8,"*"},
        {"syncAnimationInfo_t","animNode","idDeclAnimWebNode",0x28,8,"*"},
        {"idDeclSyncInteraction","validation","idDeclSyncInteraction::validationRules_t",0x1d8,0xb8},
        {"idDeclSyncInteraction","instigatorCentric","bool",0x108,1},
        {"idDeclSyncInteraction","instigatorCentricSwappedPos","bool",0x109,1},
        {"idDeclSyncInteraction","instigatorCentricStairHack","bool",0x10a,1},
        {"idDeclSyncInteraction","instigatorCentricHackNoTest","bool",0x10b,1},
        {"idDeclSyncInteraction","recalculateSyncEntityPos","bool",0x10c,1},
        {"idDeclSyncInteraction","clockwiseDegreeRotation","float",0x144,4},
        {"idDeclSyncInteraction::validationRules_t","performPositionValidation","bool",0,1},
        {"idDeclSyncInteraction::validationRules_t","minRadiusXY","float",0x7c,4},
        {"idDeclSyncInteraction::validationRules_t","maxRadiusXY","float",0x80,4},
        {"idDeclSyncInteraction::validationRules_t","minRadiusZ","float",0x84,4},
        {"idDeclSyncInteraction::validationRules_t","maxRadiusZ","float",0x88,4},
        {"idDeclSyncInteraction::validationRules_t","yawDeltaMax","float",0x90,4},
        {"idDeclSyncInteraction::validationRules_t","pitchDeltaMax","float",0x8c,4},
        {"idPlayer","modelAxis","idMat3",0x377a4,36},
        {"idPlayer","hands","idHands",0xc5a0,0x9160},
        {"idHands","rightItem","idHandsItem",0x2988,0xd78},
        {"idHandsItem","item","idManagedClassPtr < idInventoryItem >",0x30,32},
        {"idManagedClassPtr < idInventoryItem >","ptr","idInventoryItem",8,8,"*"},
        {"idInventoryItem","decl","idDeclInventory",0x38,8,"*"},
        {"idWeapon","fireMode","weaponFireMode_t",0x19d0,4},
        {"idDeclWeapon","declAimAssist","idDeclAimAssist",0x1f40,8,"*"},
        {"idDeclAimAssist","baseData","aimAssistData_t",0x90,0x188},
        {"aimAssistData_t","targetMelee","aimAssist_Melee_t",0xe0,0x90},
        {"aimAssist_Melee_t","monsterMeleeData","idList < aimAssist_Melee_t::monsterMeleeData_t , TAG_IDLIST , false >",0x38,24},
        {"aimAssist_Melee_t","monsterChainsawSyncData","idList < aimAssist_Melee_t::monsterMeleeData_t , TAG_IDLIST , false >",0x50,24},
        {"aimAssist_Melee_t::monsterMeleeData_t","monsterTypes","aiMonsterType_t",0,8},
        {"aimAssist_Melee_t::monsterMeleeData_t","lungeMaxDist","float",12,4},
        {"idDeclAIBehavior","monsterType","aiMonsterType_t",0x1f0,8},
        {"idPlayer","maxDistanceModifierGK","float",0x3b000,4},
        {"idPlayer","chargeAttack","idChargeAttack",0x3b078,8,"*"},
        {"idSyncAttack","aimAssistDef","idDeclAimAssist",0x210,8,"*"},
        {"gameViewEntInfo_t","origin","idVec3",0x30,12},
        {"gameViewEntInfo_t","viewToDistance","float",0x9c,4}
    };
    bool valid=true;
    for(const auto& expected:fields) {
        const auto& schema=lookup(expected.owner);const auto* field=enemyTelemetryField(schema,expected.name);
        if(!schema.complete||!field||field->type!=expected.type||field->ops!=expected.ops||field->size!=expected.size||field->offset!=expected.offset) valid=false;
    }
    return valid&&lookup("gameView_t").size==0x178&&lookup("gameViewEntInfo_t").size==0x100&&
        lookup("havokShapeData_t").size==0x58&&lookup("idSyncAttackInteraction").size==0x160&&
        lookup("idSyncParticipant").size==0xb8&&lookup("idSyncEntity::joinCacheData_t").size==0x3030;
}
struct SyncPreviewList { uintptr_t data=0;int32_t count=0,capacity=0;uint64_t flags=0; };
static bool syncPreviewList(const WallMemory& memory,uintptr_t at,int limit,SyncPreviewList& out) {
    return memory.get(at,out)&&out.count>=0&&out.count<=limit&&out.capacity>=out.count&&(!out.count||out.data);
}
static bool syncPreviewString(const WallMemory& memory,uintptr_t atomic,std::string& text) {
    uintptr_t data=0;char raw[160]{};
    if(!memory.get(atomic,data)||!data||!memory.read(data,raw,sizeof(raw))) return false;
    auto* end=static_cast<char*>(memchr(raw,0,sizeof(raw)));if(!end) return false;
    text.assign(raw,end);
    for(char& c:text) if(static_cast<unsigned char>(c)<32||c=='%'||c=='^') c='?';
    return true;
}
static SyncAimRequirement readSyncAimRequirement(const WallMemory& memory,uintptr_t decl) {
    SyncAimRequirement result;result.known=false;SyncPreviewList groups,after;uint32_t finalMask=0;
    if(!memory.get(decl+0x310,result.quadrants)||!syncAimMaskKnown(result.quadrants)||!syncPreviewList(memory,decl+0x2a0,32,groups))return result;
    for(int i=0;i<groups.count;++i){std::string name;if(!syncPreviewString(memory,groups.data+size_t(i)*8,name)||name.empty())return result;result.damageGroups.push_back(std::move(name));}
    if(!memory.get(decl+0x310,finalMask)||finalMask!=result.quadrants||!memory.get(decl+0x2a0,after)||memcmp(&groups,&after,sizeof(groups)))return result;
    result.known=true;return result;
}
static SyncRuleAudit readSyncRuleAudit(const WallMemory& memory,uintptr_t decl,const std::string& state,int action,const SyncAimRequirement& aim) {
    std::string name,after;SyncRuleAudit result;result.checked=true;
    if(!syncPreviewString(memory,decl+8,name)||!syncPreviewString(memory,decl+8,after)||name!=after)return result;
    return syncAuditEnemyRule(name,state,action,aim);
}
// Nonempty AI requirements can add .25 after successful validation. Several
// native early-success paths bypass that addition, so authoring alone must not
// claim it. Accept a bonus only from the coherent, eligible runtime candidate.
static bool readSyncGkAiPriority(const WallMemory& memory,uintptr_t base,uintptr_t decl,float& priority,uintptr_t interaction=0,float runtime=0) {
    SyncPreviewList requirements,after;
    if(!syncPreviewList(memory,decl+0x318,128,requirements))return false;
    if(!requirements.count)return !interaction||runtime==priority;
    if(!interaction)return false;
    const unsigned char countCode[]{0x41,0x8b,0x85,0x20,3,0,0,0x85,0xc0,0x0f,0x8e,0xb0,1,0,0},
        bonusCode[]{0x48,0x8b,0x44,0x24,0x38,0xf3,0x0f,0x10,0,0xf3,0x0f,0x58,5,0xf1,0x7e,0xa2,1,0xf3,0x0f,0x11,0};
    unsigned char code[21]{},valid=0;uintptr_t target=0,vt=0,validator=0;float bonus=0;
    if(!memory.get(interaction+0x140,target)||!target||!memory.get(target,vt)||!memory.get(vt+0x13a0,validator)||validator!=base+0xbbf0b0||
       !memory.read(base+0xbbf4a8,code,sizeof(countCode))||memcmp(code,countCode,sizeof(countCode))||
       !memory.read(base+0xbbf652,code,sizeof(bonusCode))||memcmp(code,bonusCode,sizeof(bonusCode))||
       !memory.get(base+0x25e7554,bonus)||bonus!=.25f||!memory.get(interaction+0x48,valid)||valid!=1||runtime!=priority+bonus||
       !memory.get(decl+0x318,after)||memcmp(&requirements,&after,24))return false;
    priority+=bonus;return true;
}
static bool readSyncOrdinaryGkPriority(const WallMemory& memory,uintptr_t base,uintptr_t interaction,uintptr_t decl,float runtime,float& priority) {
    const unsigned char copy[]{0xf3,0x0f,0x10,0x80,0x48,1,0,0,0xf3,0x0f,0x11,0x41,0x4c},
        equipment[]{0x49,0x8b,0x96,0xb8,2,0,0,0x48,0x3b,0x15,0x9c,0xf7,0xf3,0},specific[]{0xb0,1,0xc3};unsigned char code[14]{};
    uintptr_t vt=0,validator=0,empty=0,required=0,after=0;unsigned char flags[3]{};float last=0;
    if(!memory.read(base+0x18dab4d,code,sizeof(copy))||memcmp(code,copy,sizeof(copy))||
       !memory.read(base+0x16ba68e,code,sizeof(equipment))||memcmp(code,equipment,sizeof(equipment))||
       !memory.get(interaction,vt)||!vt||!memory.get(vt+0x38,validator)||validator!=base+0xf6af60||
       !memory.read(validator,code,sizeof(specific))||memcmp(code,specific,sizeof(specific))||
       !memory.get(base+0x25f9e38,empty)||!empty||!memory.get(decl+0x2b8,required)||required!=empty||
       !memory.read(decl+0x314,flags,sizeof(flags))||flags[0]||flags[1]||flags[2]||
       !memory.get(decl+0x148,priority)||!std::isfinite(priority)||priority<0||priority>1024||
       !memory.get(decl+0x148,last)||last!=priority||!memory.get(decl+0x2b8,after)||after!=required)return false;
    return readSyncGkAiPriority(memory,base,decl,priority,interaction,runtime);
}
static SyncAimFrame readSyncAimFrame(const WallMemory& memory,uintptr_t base,const EntityReference& target,const GameCamera& camera) {
    SyncAimFrame result;uintptr_t vt=0,getter=0,physics=0,pvt=0,boundsGetter=0,map=0,afterPhysics=0;float after[6]{};
    const unsigned char getPhysics[]{0x48,0x8b,0x81,0xd0,1,0,0,0xc3},getBounds[]{0x48,0x8d,0x81,0xe0,0,0,0,0xc3};unsigned char code[8]{};
    if(!memory.get(target.address,vt)||vt<base||vt-base>=0x1d551000||!memory.get(vt+0xe28,getter)||getter<base||getter-base>=0x2500000||!memory.read(getter,code,8)||memcmp(code,getPhysics,8)||
       !memory.get(target.address+0x1d0,physics)||!physics||!memory.get(physics,pvt)||pvt<base||pvt-base>=0x1d551000||!memory.get(pvt+0x38,boundsGetter)||boundsGetter<base||boundsGetter-base>=0x2500000||
       !memory.read(boundsGetter,code,8)||memcmp(code,getBounds,8)||!memory.get(physics+0x70,map)||map!=target.map||
       !memory.read(physics+0xe0,result.bounds,24)||!syncAimBoundsValid(result.bounds)||!memory.read(physics+0xe0,after,24)||memcmp(after,result.bounds,24)||
       !memory.get(target.address+0x1d0,afterPhysics)||afterPhysics!=physics)return result;
    result.quadrant=syncAimQuadrant(camera,result.bounds);result.known=result.quadrant!=0;return result;
}
static void syncShotBounds(const std::vector<SyncHitSphere>& spheres,float* bounds) {
    for(unsigned i=0;i<3;++i){bounds[i]=FLT_MAX;bounds[i+3]=-FLT_MAX;}
    for(const auto& sphere:spheres){const float center[]{sphere.center.x,sphere.center.y,sphere.center.z};for(unsigned i=0;i<3;++i){bounds[i]=std::min(bounds[i],center[i]-sphere.radius);bounds[i+3]=std::max(bounds[i+3],center[i]+sphere.radius);}}
}
static bool readSyncFacingPose(const WallMemory& memory,uintptr_t base,const EntityReference& ref,SyncFacingPose& pose) {
    uintptr_t component=0,vtable=0,getter=0,map=0;SyncFacingPose after{};
    const unsigned char expected[]{0x48,0x8d,0x81,0x2c,0x3d,0,0,0xc3};unsigned char code[8]{};
    if(!memory.get(ref.address+0x58a0,component)||!component||!memory.get(component+0x18,vtable)||
       !memory.get(vtable+0x70,getter)||getter!=base+0xd41ed0||!memory.read(getter,code,8)||memcmp(code,expected,8)||
       !memory.get(component+0x18+0x70,map)||map!=ref.map||
       !memory.read(component+0x18+0x3d2c,pose.axis,36)||!memory.get(component+0x18+0xb0,pose.origin)||
       !syncFacingOrthonormal(pose.axis)||!syncPreviewFinite(pose.origin)||
       fabsf(pose.axis[2])>.001f||fabsf(pose.axis[5])>.001f||pose.axis[8]<.999f||
       !memory.read(component+0x18+0x3d2c,after.axis,36)||!memory.get(component+0x18+0xb0,after.origin)||memcmp(&pose,&after,sizeof(pose)))return false;
    uintptr_t finalComponent=0;return memory.get(ref.address+0x58a0,finalComponent)&&component==finalComponent;
}
static bool readSyncPlayerIncoming(const WallMemory& memory,uintptr_t player,float& yaw) {
    float axis[9]{};if(!memory.read(player+0x377a4,axis,36)||!syncFacingOrthonormal(axis)||axis[8]<.999f)return false;
    yaw=atan2f(-axis[1],-axis[0]);return true;
}
static bool readSyncFacingRegion(const WallMemory& memory,uintptr_t decl,uintptr_t node,uintptr_t jointCache,
                                 short instigatorJoint,short targetJoint,const std::string& name,SyncFacingRegion& region) {
    unsigned char centric[5]{},position=0;float rotation=0,pitch=0,yaw=0,ranges[4]{};int32_t frame=-1;unsigned char valid=0;
    if(!decl||!node||instigatorJoint<0||targetJoint<0||instigatorJoint>=256||targetJoint>=256||
       !memory.read(decl+0x108,centric,5)||std::any_of(std::begin(centric),std::end(centric),[](unsigned char v){return v!=0;})||
       !memory.get(decl+0x144,rotation)||rotation!=0||!memory.get(decl+0x1d8,position)||position!=1||
       !memory.get(decl+0x264,pitch)||!std::isfinite(pitch)||pitch<45||pitch>180||!memory.get(decl+0x268,yaw)||!std::isfinite(yaw)||fabsf(yaw)<=0||fabsf(yaw)>180||
       !memory.read(decl+0x254,ranges,16)||!memory.get(jointCache+0x3018,frame)||frame!=0||
       !memory.get(jointCache+0x301c,valid)||valid!=1)return false;
    for(float v:ranges)if(!std::isfinite(v)||v<0||v>4096)return false;
    if(ranges[0]!=0||ranges[0]>ranges[1]||ranges[2]>ranges[3])return false;
    float instigator[12]{},target[12]{},check[12]{};
    if(!memory.read(jointCache+size_t(instigatorJoint)*48,instigator,48)||!memory.read(jointCache+size_t(targetJoint)*48,target,48)||
       !syncFacingIncoming(instigator,target,region.incoming)||fabsf(region.incoming.z)>.001f||
       !memory.read(jointCache+size_t(instigatorJoint)*48,check,48)||memcmp(check,instigator,48)||
       !memory.read(jointCache+size_t(targetJoint)*48,check,48)||memcmp(check,target,48))return false;
    if(pitch<90){for(const auto* matrix:{instigator,target})if(fabsf(matrix[2])>.001f||fabsf(matrix[6])>.001f||fabsf(matrix[8])>.001f||fabsf(matrix[9])>.001f||matrix[10]<.999f)return false;}
    const Vec3 difference{instigator[3]-target[3],instigator[7]-target[7],instigator[11]-target[11]};
    region.start={target[0]*difference.x+target[4]*difference.y+target[8]*difference.z,
                  target[1]*difference.x+target[5]*difference.y+target[9]*difference.z,
                  target[2]*difference.x+target[6]*difference.y+target[10]*difference.z};
    uint32_t filter=0;unsigned char lunge=2;
    if(!syncPreviewFinite(region.start)||!memory.get(decl+0xa4,filter)||!memory.get(decl+0x250,lunge)||lunge>1)return false;
    region.checkLunge=lunge!=0;region.flatLunge=(filter&0x190)!=0;
    region.aim=readSyncAimRequirement(memory,decl);
    region.name=name;region.halfYaw=fabsf(yaw)*syncFacingPi/180;region.minXY=ranges[0];region.maxXY=ranges[1];region.minZ=ranges[2];region.maxZ=ranges[3];return true;
}
#include "sync_range_reader.h"
#include "sync_player_context.h"
static bool readSyncHitSpheres(const WallMemory& memory,uintptr_t base,const EntityReference& ref,std::vector<SyncHitSphere>& result) {
    result.clear();uintptr_t wrapper=0,physics=0,map=0,root=0,duplicate=0;unsigned char dead=1;
    if(!memory.get(ref.address+0x6488,dead)||dead||!memory.get(ref.address+0x1a28,wrapper)||!wrapper||
       !memory.get(ref.address+0x1a38,physics)||!physics||!memory.get(physics+0x70,map)||map!=ref.map||
       !memory.get(physics+0x238,duplicate)||duplicate!=wrapper||!memory.get(wrapper,root)||!root) return false;
    struct Pose { float axis[9];Vec3 origin; } pose{},after{};
    int64_t time=0,finalTime=0;
    if(!memory.read(physics+0x8c,&pose,sizeof(pose))||!syncPreviewFinite(pose.origin)||!memory.get(physics+0x240,time)||time<0) return false;
    for(unsigned i=0;i<3;++i) for(unsigned j=0;j<3;++j) {
        double d=0;for(unsigned k=0;k<3;++k) d+=double(pose.axis[i*3+k])*pose.axis[j*3+k];
        if(!std::isfinite(d)||fabs(d-(i==j?1:0))>.004) return false;
    }
    unsigned char header[112]{};uintptr_t children=0;uint32_t count=0;
    if(!memory.read(root,header,sizeof(header))||header[0x22]!=13) return false;
    memcpy(&children,header+0x50,8);memcpy(&count,header+0x58,4);
    if(!children||!count||count>128) return false;
    SyncPreviewList materials;
    if(!syncPreviewList(memory,wrapper+8,256,materials)||materials.count<int(count+1)) return false;
    std::vector<unsigned char> rows(size_t(count)*112),again(rows.size()),materialRows(size_t(materials.count)*0x58),afterMaterials(materialRows.size());
    if(!memory.read(children,rows.data(),rows.size())||!memory.read(materials.data,materialRows.data(),materialRows.size())) return false;
    std::unordered_map<uintptr_t,std::string> groups;
    for(unsigned i=0;i<count;++i) {
        const auto* row=rows.data()+i*112;if(row[0x5c]) continue;
        uintptr_t shape=0,material=0,group=0;float leafRadius=0,scale[3]{},local[3]{};uint32_t contents=0;unsigned char leaf[64]{};
        memcpy(&shape,row+0x50,8);memcpy(scale,row+0x40,12);memcpy(local,row+0x30,12);
        if(!shape||!memory.read(shape,leaf,sizeof(leaf))) return false;
        uintptr_t vtable=0;memcpy(&vtable,leaf,8);
        // RTTI of this audited native kind/vtable is hknpSphereShape. No box,
        // capsule or unsupported primitive is substituted with a sphere.
        if(leaf[0x22]!=2||vtable!=base+0x2f265e8) return false;
        memcpy(&leafRadius,leaf+0x28,4);memcpy(&material,leaf+0x30,8);
        if(!std::isfinite(leafRadius)||leafRadius<=0||leafRadius>100||!std::isfinite(scale[0])||!std::isfinite(scale[1])||!std::isfinite(scale[2])||scale[0]<=0||scale[0]>100||
           fabsf(scale[0]-scale[1])>1e-5f||fabsf(scale[0]-scale[2])>1e-5f||
           material<materials.data||(material-materials.data)%0x58||(material-materials.data)/0x58>=unsigned(materials.count)) return false;
        const auto* materialRow=materialRows.data()+(material-materials.data);memcpy(&contents,materialRow+8,4);memcpy(&group,materialRow+0x50,8);
        if(!(contents&0x80)||(contents&0x40000000)) continue;
        auto found=groups.find(group);
        if(found==groups.end()) {
            std::string label;
            if(group&&!syncPreviewString(memory,group+8,label)) return false;
            found=groups.emplace(group,label.empty()?"unassigned":std::move(label)).first;
        }
        SyncHitSphere sphere;sphere.radius=leafRadius*scale[0];sphere.group=found->second;
        memcpy(&sphere.jointIndex,materialRow+0x3a,2);
        sphere.center={pose.origin.x,pose.origin.y,pose.origin.z};
        for(unsigned j=0;j<3;++j) {
            sphere.center.x+=pose.axis[j*3]*local[j];sphere.center.y+=pose.axis[j*3+1]*local[j];sphere.center.z+=pose.axis[j*3+2]*local[j];
        }
        if(!syncPreviewFinite(sphere.center)||!std::isfinite(sphere.radius)||sphere.radius>100) return false;
        result.push_back(std::move(sphere));
    }
    uintptr_t finalRoot=0,finalPhysics=0;SyncPreviewList finalMaterials;
    if(result.empty()||!memory.read(children,again.data(),again.size())||rows!=again||!memory.read(physics+0x8c,&after,sizeof(after))||
       !memory.get(wrapper+8,finalMaterials)||memcmp(&materials,&finalMaterials,24)||!memory.read(materials.data,afterMaterials.data(),afterMaterials.size())||materialRows!=afterMaterials||
       memcmp(&pose,&after,sizeof(pose))||!memory.get(physics+0x240,finalTime)||time!=finalTime||
       !memory.get(wrapper,finalRoot)||finalRoot!=root||!memory.get(ref.address+0x1a38,finalPhysics)||finalPhysics!=physics||
       !memory.get(ref.address+0x6488,dead)||dead) {result.clear();return false;}
    return true;
}
#include "sync_authored_preview.h"
static bool syncTargetWithinActionRadius(const SyncTargetFrame& value,float displayLimit=200) {
    if(!value.ranges.known||!syncFacingOrthonormal(value.facing.axis)||!syncPreviewFinite(value.ranges.playerOrigin)||
       !syncPreviewFinite(value.ranges.eyeOffset)||!syncPreviewFinite(value.ranges.viewTargetOffset))return false;
    if(value.action==1&&!value.hasChainsaw)return false;
    const float actionRadius=value.action==1?value.ranges.chainsawTarget:value.ranges.gloryKillTarget;
    if(!std::isfinite(actionRadius)||actionRadius<=0||actionRadius>200||!std::isfinite(displayLimit)||displayLimit<=0||displayLimit>200)return false;
    const auto eye=motionAdd(value.ranges.playerOrigin,value.ranges.eyeOffset);
    const auto target=motionAdd(value.facing.origin,syncRangeTransform(value.facing,value.ranges.viewTargetOffset));
    return syncSelectionDistance(eye,target)<=std::min(actionRadius,displayLimit);
}
class SyncTargetReader {
    LevelIdentity previous;
    unsigned budgetMicroseconds;
    std::unordered_map<uintptr_t,EntityReference> references;
    struct RuleCache {LevelIdentity level;EntityReference target,blueprint;std::vector<SyncFacingRegion> regions;bool chainsaw=false;float modifier=0;std::string state;} rules;
    struct AnimationCache {LevelIdentity level;EntityReference target,blueprint;std::vector<SyncAnimationRule> rows;} animations;
    LevelIdentity chainsawLevel;EntityReference chainsawTarget;std::vector<std::string> cachedChainsawAnimations;
    std::vector<SyncChainsawRule> cachedChainsawRules;
    LevelIdentity authoredLevel;EntityReference authoredTarget;std::vector<EntityReference> authoredBlueprints;
    ULONGLONG authoredSearch=0;bool authoredResolved=false;
    SyncTargetFrame retainedFrame;
    EntityReference reference(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const LevelIdentity& level,uintptr_t actor,const char* className="idAI2") {
        auto found=references.find(actor);
        if(found!=references.end()&&found->second.className==className&&enemyTelemetryIdentity(memory,base,profile,found->second)) return found->second;
        if(found!=references.end()) references.erase(found);
        SyncPreviewList list;if(!syncPreviewList(memory,level.map+profile.mapEntitiesOffset,32768,list)) return {};
        struct Entry {uint32_t id,generation;uintptr_t entity,previous,next;};std::vector<Entry> entries(list.count);
        if(!memory.read(list.data,entries.data(),entries.size()*sizeof(Entry))) return {};
        for(size_t i=0;i<entries.size();++i) if(entries[i].entity==actor) {
            uintptr_t vtable=0;if(!memory.get(actor,vtable)||readEntityClass(memory,vtable,base,profile)!=className) return {};
            EntityReference ref{actor,level.map,list.data+i*sizeof(Entry),entries[i].id,entries[i].generation,className};
            if(!enemyTelemetryIdentity(memory,base,profile,ref)) return {};
            if(references.size()>=128) references.clear();references[actor]=ref;return ref;
        }
        return {};
    }
public:
    explicit SyncTargetReader(unsigned budget=2000):budgetMicroseconds(budget) {}
    void clearRetainedTarget(){retainedFrame={};}
    SyncTargetFrame captureOnce(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const LevelIdentity& level,const GameCamera& camera,float distanceLimit,const GraphLookup* lookup=nullptr,const SyncTargetFrame* retained=nullptr) {
        const auto began=std::chrono::steady_clock::now();
        SyncTargetFrame result;result.level=level;result.captured=GetTickCount64();result.phase="no_target";result.action=syncTargetAction.load();
        result.selectedAnimation=(*std::atomic_load(&syncAnimationSelection))[result.action];
        if(!syncPreviewProfile(profile)||!level||!syncPreviewCode(memory,base)||!std::isfinite(distanceLimit)||distanceLimit<=0||distanceLimit>200) {result.phase="unsupported";return result;}
        if(previous!=level) {references.clear();rules={};chainsawLevel={};chainsawTarget={};cachedChainsawAnimations.clear();cachedChainsawRules.clear();previous=level;}
        Vec3 eye{},direction{};if(!inspectorRay(camera,eye,direction)) return result;
        uintptr_t game=0,player=0,vtable=0;if(!memory.get(base+0x6128718,game)||!game||!memory.get(level.map+profile.mapPlayerOffset,player)||
            !memory.get(player,vtable)||vtable!=base+profile.playerVtableRva) return result;
        const uintptr_t manager=game+0x136140;int32_t index=-1;SyncPreviewList views;
        if(!memory.get(manager+0x58,index)||index<0||index>1||!syncPreviewList(memory,manager+0x40,8,views)) return result;
        float nearest=distanceLimit;uintptr_t selectedView=0;SyncPreviewList processed,resolved;
        for(int i=0;i<views.count;++i) {
            const auto view=views.data+size_t(i)*0x178;uintptr_t owner=0;
            if(!memory.get(view+0xc8,owner)||owner!=player) continue;
            if(selectedView) {result.phase="changing";return result;}selectedView=view;
            if(!syncPreviewList(memory,view+0x60+index*24,1024,processed)||!syncPreviewList(memory,view+0x140+index*24,1024,resolved)||processed.count!=resolved.count) return result;
            std::vector<unsigned char> info(size_t(processed.count)*0x100);std::vector<uintptr_t> actors(processed.count);
            if(processed.count&&(!memory.read(processed.data,info.data(),info.size())||!memory.read(resolved.data,actors.data(),actors.size()*8))) return result;
            for(int j=0;j<processed.count;++j) {
                if(budgetMicroseconds&&std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-began).count()>budgetMicroseconds) {
                    result={};result.phase="limited";return result;
                }
                const auto* row=info.data()+j*0x100;
                if((row[0xf8]&0x41)!=0x41||!actors[j]||actors[j]==player) continue; // Native isAI + line of sight.
                uintptr_t physics=0;float bounds[6]{};unsigned char dead=1;
                if(!memory.get(actors[j]+0x6488,dead)||dead||!memory.get(actors[j]+0x1a38,physics)||!physics||
                   !memory.read(physics+0xe0,bounds,sizeof(bounds))) continue;
                bool finite=true;for(float value:bounds) finite&=std::isfinite(value)&&fabsf(value)<1e6f;
                if(!finite||!rayBounds(eye,direction,bounds,nearest)) continue;
                auto ref=reference(memory,base,profile,level,actors[j]);if(!ref) continue;
                uint32_t ownerToken=0;memcpy(&ownerToken,row+0xd0,4);if(ownerToken!=ref.id) continue;
                uintptr_t targeting=0;uint32_t targetType=0;
                if(!memory.get(ref.address+0xbd0,targeting)||(targeting&&!memory.get(targeting+0xc4,targetType))) {result.phase="changing";return result;}
                const bool hostile=(targetType&1)&&!(targetType&2); // Native HOSTILE / FRIENDLY targeting bits.
                std::vector<SyncHitSphere> spheres;if(!readSyncHitSpheres(memory,base,ref,spheres)) {result.phase="changing";return result;}
                bool hit=false;std::string group;
                for(const auto& sphere:spheres) if(syncSphereHit(eye,direction,sphere,nearest)) {hit=true;group=sphere.group;}
                if(hit) {
                    result.distance=nearest;
                    if(hostile) {result.entity=ref;result.spheres=std::move(spheres);result.group=std::move(group);memcpy(&result.viewTarget,row+0x30,12);if(!memory.get(view,result.viewPosition)){result.phase="changing";return result;}}
                    else {result.entity={};result.spheres.clear();result.group.clear();}
                }
            }
        }
        int32_t finalIndex=-1;uintptr_t finalOwner=0,finalPlayer=0,finalGame=0;SyncPreviewList finalViews,finalProcessed,finalResolved;
        if(!selectedView||!memory.get(manager+0x58,finalIndex)||finalIndex!=index||!memory.get(manager+0x40,finalViews)||memcmp(&views,&finalViews,24)||
           !memory.get(selectedView+0x60+index*24,finalProcessed)||memcmp(&processed,&finalProcessed,24)||
           !memory.get(selectedView+0x140+index*24,finalResolved)||memcmp(&resolved,&finalResolved,24)||
           !memory.get(selectedView+0xc8,finalOwner)||finalOwner!=player||!memory.get(level.map+profile.mapPlayerOffset,finalPlayer)||finalPlayer!=player||
           !memory.get(base+0x6128718,finalGame)||finalGame!=game||
           (result.entity&&!enemyTelemetryIdentity(memory,base,profile,result.entity))) {result={};result.phase="changing";return result;}
        if(!result.entity&&retained&&lookup&&retained->level==level&&retained->entity&&retained->ranges.known&&
           enemyTelemetryIdentity(memory,base,profile,retained->entity)) {
            result.entity=retained->entity;
            uintptr_t targeting=0;uint32_t targetType=0;
            if(!memory.get(result.entity.address+0xbd0,targeting)||!targeting||!memory.get(targeting+0xc4,targetType)||!(targetType&1)||(targetType&2))return SyncTargetFrame{};
            if(!readSyncHitSpheres(memory,base,result.entity,result.spheres)||!readSyncFacingPose(memory,base,result.entity,result.facing)||
               !memory.get(selectedView,result.viewPosition))return SyncTargetFrame{};
            result.viewTarget=motionAdd(result.facing.origin,syncRangeTransform(result.facing,retained->ranges.viewTargetOffset));
            result.ranges=readSyncRangeLimits(memory,base,profile,*lookup,player,result.entity,result.facing,result.viewPosition,result.viewTarget);
            result.hasChainsaw=retained->hasChainsaw;
            if(!syncTargetWithinActionRadius(result,distanceLimit)) {result.entity={};result.spheres.clear();return result;}
            result.hasChainsaw=false; // Re-establish support from current assigned authoring below.
            result.retainedTarget=true;result.retainedAim=retained->retainedTarget?retained->retainedAim:retained->aim;
            result.retainedFocus=retained->retainedTarget?retained->retainedFocus:retained->jointFocus;
            result.retainedSelectionKnown=retained->retainedTarget?retained->retainedSelectionKnown:retained->selectionKnown;
            result.retainedSelectionTolerance=retained->retainedTarget?retained->retainedSelectionTolerance:retained->selectionTolerance;
            result.retainedSelectionPose=retained->retainedTarget?retained->retainedSelectionPose:retained->facing;
            result.retainedChainsawRules=retained->retainedTarget?retained->retainedChainsawRules:retained->chainsawRules;
            result.distance=float(syncSelectionDistance(result.viewPosition,result.viewTarget));result.group=retained->group;
        }
        if(!result.entity) return result;
        result.phase="ready";
        if(lookup){uintptr_t definition=0,afterDefinition=0;std::string name,afterName;
            if(memory.get(result.entity.address+0xa8,definition)&&definition&&syncPreviewString(memory,definition+8,name)&&
               memory.get(result.entity.address+0xa8,afterDefinition)&&definition==afterDefinition&&syncPreviewString(memory,definition+8,afterName)&&name==afterName)result.enemyDefinition=std::move(name);}
        syncShotBounds(result.spheres,result.shotBounds);result.aim=readSyncAimFrame(memory,base,result.entity,camera);
        result.playerFootprint=readSyncPlayerFootprint(memory,player,level.map);result.jointFocus=readSyncJointFocus(memory,base,player,result.entity);
        readSyncSphereFocusGroups(memory,base,result.entity,result.spheres);
        bool facingReady=readSyncFacingPose(memory,base,result.entity,result.facing)&&readSyncPlayerIncoming(memory,player,result.incomingYaw);
        if(facingReady&&lookup)result.ranges=readSyncRangeLimits(memory,base,profile,*lookup,player,result.entity,result.facing,result.viewPosition,result.viewTarget);
        if(result.ranges.known){const auto point=result.retainedTarget?result.viewTarget:motionAdd(eye,motionScale(direction,nearest));result.ranges.aimOrigin=subtract(point,subtract(eye,result.ranges.playerOrigin));result.ranges.modelViewOffset=remainderf(result.incomingYaw-atan2f(-direction.y,-direction.x),2*syncFacingPi);}
        if(result.action==1&&result.ranges.known&&!result.ranges.ignoreQuickLunge){result.ranges.known=false;result.ranges.detail="Custom Chainsaw validation unavailable";}
        uintptr_t blueprint=0,blueprintWeb=0;uint32_t blueprintToken=0;SyncPreviewList participants,caches;EntityReference blueprintRef;
        short instigatorJoint=-1,targetJoint=-1;std::unordered_map<short,uintptr_t> joints;
        unsigned char facingRequirements[2]{1,1};
        if(facingReady&&memory.get(player+0x70e8+0x50,blueprint)&&blueprint&&
           (blueprintRef=reference(memory,base,profile,level,blueprint,"idSyncEntity"))&&
           memory.get(player+0x70e8+0x48,blueprintToken)&&blueprintToken==blueprintRef.id&&
           memory.get(blueprint+0x1bf8,blueprintWeb)&&blueprintWeb&&memory.read(blueprint+0x2bd0,facingRequirements,2)&&
           !facingRequirements[0]&&!facingRequirements[1]&&syncPreviewList(memory,blueprint+0x1c18,32,participants)&&syncPreviewList(memory,blueprint+0x2c08,128,caches)) {
            bool unique=true;
            for(int i=0;i<participants.count;++i){int type=-1;short joint=-1;const auto p=participants.data+size_t(i)*0xb8;if(!memory.get(p+0xa0,type)||!memory.get(p+0xaa,joint)){unique=false;break;}if(type==0){if(instigatorJoint!=-1)unique=false;instigatorJoint=joint;}if(type==1){if(targetJoint!=-1)unique=false;targetJoint=joint;}}
            if(unique&&instigatorJoint>=0&&targetJoint>=0)for(int i=0;i<caches.count;++i){short nodeIndex=-1;const auto at=caches.data+size_t(i)*0x3030;if(!memory.get(at,nodeIndex)||nodeIndex<0||!joints.emplace(nodeIndex,at+0x10).second){joints.clear();break;}}
        }
        result.regionState=joints.empty()?"direction data unavailable":"ready";
        SyncPreviewList interactions,finalInteractions;
        const auto list=player+0x70e8+0x178;
        if(!syncPreviewList(memory,list,128,interactions)) return result;
        bool coherent=true,sawGloryKill=false;
        // 18d2600 compares runtime priorities and distances to +84, then
        // retains a random band around the nearest equal-priority pose.
        // This is a CURRENT-POSE model; no cached poses or default priorities.
        unsigned char adjust=1,code[12]{};
        const unsigned char distanceCode[]{0xf3,0x0f,0x5c,0x8e,0x84,0,0,0},
            priorityCode[]{0xf3,0x0f,0x10,0x46,0x4c,0x0f,0x2f,0xc7,0x77,0x2d,0x0f,0x2e},
            bandCode[]{0xf3,0x0f,0x58,0x65,0x7f,0xf3,0x0f,0x59,0xe4};
        result.selectionKnown=blueprintRef&&memory.get(blueprint+0x1caa,adjust)&&adjust==0&&
            memory.read(base+0x18d2785,code,sizeof(distanceCode))&&!memcmp(code,distanceCode,sizeof(distanceCode))&&
            memory.read(base+0x18d27ae,code,sizeof(priorityCode))&&!memcmp(code,priorityCode,sizeof(priorityCode))&&
            memory.read(base+0x18d2943,code,sizeof(bandCode))&&!memcmp(code,bandCode,sizeof(bandCode))&&
            syncRangeCvar(memory,base+0x6189a40,"dp_gloryKillMaxAllowableDistanceForRandom",result.selectionTolerance)&&
            result.selectionTolerance>0&&result.selectionTolerance<=10;
        for(int i=0;i<interactions.count;++i) {
            uintptr_t interaction=0;if(!memory.get(interactions.data+i*8,interaction)||!interaction) {coherent=false;break;}
            unsigned char bytes[0x160]{},after[0x160]{};
            if(!memory.read(interaction,bytes,sizeof(bytes))) {coherent=false;break;}
            uintptr_t target=0,decl=0;uint32_t token=0;memcpy(&target,bytes+0x140,8);memcpy(&token,bytes+0x138,4);memcpy(&decl,bytes+0x40,8);
            if(target!=result.entity.address||token!=result.entity.id) continue;
            if(bytes[0x48]>1||bytes[0x110]>1) {coherent=false;break;}
            if(!decl)continue;
            int inputs[2]{},type=-1;unsigned char special[2]{1,1};std::string state;
            if(!memory.read(decl+0x98,inputs,8)||!memory.get(decl+0xa8,type)||type!=0||!memory.read(decl+0x314,special,2)||
               !syncPreviewString(memory,interaction+0xe8,state)||!memory.read(interaction,after,sizeof(after))) {coherent=false;break;}
            const bool sameAuthoring=!memcmp(bytes+0x40,after+0x40,8)&&!memcmp(bytes+0xe8,after+0xe8,8)&&
                !memcmp(bytes+0x100,after+0x100,16)&&!memcmp(bytes+0x138,after+0x138,32);
            if(!sameAuthoring){coherent=false;continue;}
            const bool sameCandidate=!memcmp(bytes,after,sizeof(bytes));if(!sameCandidate)coherent=false;
            if(sameCandidate&&(bytes[0x48]||bytes[0x110])&&(special[0]||special[1]||state.empty())&&syncInputMatches(inputs,result.action==0?2:16))result.selectionKnown=false;
            if(!special[0]&&!special[1]&&!state.empty()) {
                const auto aimRule=readSyncAimRequirement(memory,decl);
                for(int action:{0,1})if(syncInputMatches(inputs,action==0?2:16)){
                    SyncAnimationRule rule{state,action,aimRule,{}};if(lookup){rule.audit=readSyncRuleAudit(memory,decl,state,action,rule.aim);if(!rule.audit.matched)rule.aim.known=false;}result.animationRules.push_back(std::move(rule));}
                sawGloryKill|=syncInputMatches(inputs,2);
                const bool ready=sameCandidate&&bytes[0x48]&&!bytes[0x110];
                if(bytes[0x110]){result.pendingGloryKill|=syncInputMatches(inputs,2);result.pendingChainsaw|=syncInputMatches(inputs,16);}
                if(ready&&syncInputMatches(inputs,2)) result.gloryKills.push_back(state);
                if(ready&&syncInputMatches(inputs,16)) result.chainsaw.push_back(state);
                if(ready)for(int action:{0,1})if(syncInputMatches(inputs,action==0?2:16)){
                    SyncNativeChoice choice;choice.name=state;choice.action=action;memcpy(&choice.priority,bytes+0x4c,4);memcpy(&choice.position,bytes+0x84,12);
                    choice.known=std::isfinite(choice.priority)&&choice.priority>=0&&syncPreviewFinite(choice.position);result.nativeChoices.push_back(std::move(choice));}
                result.hasChainsaw|=syncInputMatches(inputs,16);
                if(syncInputMatches(inputs,16))result.chainsawAnimations.push_back(state);
                if(syncInputMatches(inputs,16)){
                    SyncChainsawRule rule{state,aimRule,inputs[0]==16||inputs[1]==16};
                    if(lookup){rule.audit=readSyncRuleAudit(memory,decl,state,1,rule.aim);if(!rule.audit.matched)rule.aim.known=false;}
                    memcpy(&rule.priority,bytes+0x4c,4);memcpy(&rule.selectionPosition,bytes+0x84,12);
                    rule.selectionKnown=sameCandidate&&bytes[0x48]&&std::isfinite(rule.priority)&&rule.priority>=0&&syncPreviewFinite(rule.selectionPosition);
                    result.chainsawRules.push_back(std::move(rule));}
                // Native 18db396..18db39f skips the facing/position stage for
                // QUICK_CHAINSAW in either slot. Never invent front/back CS
                // hemispheres from animation names or reuse the GK limits.
                if(syncInputMatches(inputs,2)&&inputs[0]!=16&&inputs[1]!=16&&!joints.empty()) {
                    uintptr_t web=0,node=0;short nodeIndex=-1;memcpy(&web,bytes+0x100,8);memcpy(&node,bytes+0x108,8);
                    if(web==blueprintWeb&&node&&memory.get(node+0x30,nodeIndex)) {
                        auto found=joints.find(nodeIndex);SyncFacingRegion region;
                        float modifier=0;memcpy(&modifier,bytes+0x50,4);
                        if(found!=joints.end()&&readSyncFacingRegion(memory,decl,node,found->second,instigatorJoint,targetJoint,state,region)&&std::isfinite(modifier)&&region.maxXY+modifier>0&&region.maxXY+modifier<=4096){
                            if(lookup){region.audit=readSyncRuleAudit(memory,decl,state,0,region.aim);if(!region.audit.matched){region.aim.known=false;result.regionState="partial facing preview";}}
                            float runtimePriority=0;memcpy(&runtimePriority,bytes+0x4c,4);
                            if(sameCandidate)region.priorityKnown=readSyncOrdinaryGkPriority(memory,base,interaction,decl,runtimePriority,region.priority);
                            region.maxXY+=modifier;result.regions.push_back(std::move(region));}
                        else result.regionState="partial facing preview";
                    } else result.regionState="partial facing preview";
                }
            }
        }
        const bool sameList=memory.get(list,finalInteractions)&&!memcmp(&interactions,&finalInteractions,24);coherent=coherent&&sameList;
        result.rulesCoherent=coherent;
        if(!coherent) {result.gloryKills.clear();result.chainsaw.clear();result.nativeChoices.clear();result.selectionKnown=false;}
        if(!sameList){result.regions.clear();result.animationRules.clear();result.chainsawAnimations.clear();result.chainsawRules.clear();result.hasChainsaw=false;result.regionState="checking direction data";}
        SyncPreviewList finalCaches,finalParticipants;uintptr_t finalBlueprint=0;uint32_t finalBlueprintToken=0;
        if(!result.regions.empty()&&(!memory.get(player+0x70e8+0x50,finalBlueprint)||finalBlueprint!=blueprint||
           !memory.get(player+0x70e8+0x48,finalBlueprintToken)||finalBlueprintToken!=blueprintToken||
           !enemyTelemetryIdentity(memory,base,profile,blueprintRef)||!memory.get(blueprint+0x2c08,finalCaches)||memcmp(&caches,&finalCaches,24)||
           !memory.get(blueprint+0x1c18,finalParticipants)||memcmp(&participants,&finalParticipants,24))) {result.regions.clear();result.regionState="checking direction data";}
        bool ownedAuthoringKnown=false;
        if(lookup&&sameList&&coherent) {
            const auto source=readSyncAuthoredSource(memory,result.entity,*lookup);
            if(source.known) {
                ownedAuthoringKnown=true;
                for(const auto& row:source.rows) {
                    auto aim=row.aim;if(!row.audit.matched)aim.known=false;
                    result.animationRules.push_back({row.state,row.action,aim,row.audit});
                    if(row.action==1){result.hasChainsaw=true;result.chainsawAnimations.push_back(row.state);}
                }
                std::vector<std::string> required;
                for(const auto& row:source.rows)if(row.action==0&&std::find(required.begin(),required.end(),row.state)==required.end())required.push_back(row.state);
                bool missing=result.regionState!="ready";
                for(const auto& row:source.rows)if(row.action==0&&std::none_of(result.regions.begin(),result.regions.end(),[&](const auto& r){
                    return r.name==row.state&&r.audit.matched&&r.audit.declaration==row.audit.declaration;}))missing=true;
                if(facingReady&&result.ranges.known&&missing&&!required.empty()) {
                    const bool sameTarget=authoredLevel==level&&authoredTarget.address==result.entity.address&&authoredTarget.slot==result.entity.slot&&
                        authoredTarget.id==result.entity.id&&authoredTarget.generation==result.entity.generation;
                    if(!sameTarget){authoredLevel=level;authoredTarget=result.entity;authoredBlueprints.clear();authoredSearch=0;authoredResolved=false;}
                    authoredBlueprints.erase(std::remove_if(authoredBlueprints.begin(),authoredBlueprints.end(),[&](const auto& ref){uintptr_t def=0;return !enemyTelemetryIdentity(memory,base,profile,ref)||
                        !memory.get(ref.address+0xa8,def)||std::find(source.blueprintDefinitions.begin(),source.blueprintDefinitions.end(),def)==source.blueprintDefinitions.end();}),authoredBlueprints.end());
                    if((authoredBlueprints.empty()||!authoredResolved)&&(!authoredSearch||GetTickCount64()-authoredSearch>=1000)) {
                        authoredSearch=GetTickCount64();authoredBlueprints.clear();SyncPreviewList entities;
                        if(syncPreviewList(memory,level.map+profile.mapEntitiesOffset,32768,entities)) {
                            struct Entry {uint32_t id,generation;uintptr_t entity,previous,next;};std::vector<Entry> rows(entities.count);
                            if(memory.read(entities.data,rows.data(),rows.size()*sizeof(Entry)))for(const auto& row:rows)if(row.entity) {
                                uintptr_t def=0;if(memory.get(row.entity+0xa8,def)&&std::find(source.blueprintDefinitions.begin(),source.blueprintDefinitions.end(),def)!=source.blueprintDefinitions.end()) {
                                    const auto ref=reference(memory,base,profile,level,row.entity,"idSyncEntity");if(ref)authoredBlueprints.push_back(ref);
                                }
                            }
                            SyncPreviewList after;if(!memory.get(level.map+profile.mapEntitiesOffset,after)||memcmp(&entities,&after,24))authoredBlueprints.clear();
                        }
                    }
                    const auto geometry=readSyncAuthoredGeometry(memory,base,source,authoredBlueprints,result.ranges.modifier,*lookup);
                    authoredResolved=geometry.known&&geometry.complete;
                    if(geometry.known) {
                        auto regions=geometry.regions;
                        for(auto& authored:regions)for(const auto& observed:result.regions)
                            if(authored.audit.matched&&observed.audit.matched&&authored.audit.declaration==observed.audit.declaration&&
                               authored.name==observed.name&&observed.priorityKnown){authored.priorityKnown=true;authored.priority=observed.priority;break;}
                        result.regions=std::move(regions);result.blueprint=geometry.blueprint;
                        result.regionState=geometry.complete?"ready":"partial facing preview";
                    }
                }
            }
        }
        if(!syncNormalizeFacingRegions(result.regions))result.regionState="partial facing preview";
        if(result.regions.size()>32){result.regions.clear();result.regionState="Too many native facing rules";}
        if(!result.blueprint&&(!result.regions.empty()||!result.animationRules.empty()))result.blueprint=blueprintRef;
        if(result.regions.empty()&&result.regionState=="ready")result.regionState="unsupported facing rules";
        // Keep copied, immutable authoring rules when the engine stops
        // submitting interactions outside its targeting radius. Revalidate
        // both owners: a boundary must not disappear as it is crossed.
        if(!result.regions.empty()&&result.ranges.known)rules={level,result.entity,result.blueprint,result.regions,result.hasChainsaw,result.ranges.modifier,result.regionState};
        else if(!ownedAuthoringKnown&&!sawGloryKill&&sameList&&result.ranges.known&&rules.level==level&&rules.target.address==result.entity.address&&rules.target.id==result.entity.id&&
                rules.target.generation==result.entity.generation&&rules.target.slot==result.entity.slot&&rules.modifier==result.ranges.modifier&&
                enemyTelemetryIdentity(memory,base,profile,rules.blueprint)) {
            result.regions=rules.regions;result.blueprint=rules.blueprint;result.hasChainsaw|=rules.chainsaw;result.regionState=rules.state;
        }
        std::sort(result.chainsawRules.begin(),result.chainsawRules.end(),[](const auto& a,const auto& b){return a.name<b.name;});
        for(size_t i=1;i<result.chainsawRules.size();++i)if(result.chainsawRules[i].name==result.chainsawRules[i-1].name){auto& a=result.chainsawRules[i];auto& b=result.chainsawRules[i-1];
            if(a.priority!=b.priority||memcmp(&a.selectionPosition,&b.selectionPosition,sizeof(Vec3))||!a.selectionKnown||!b.selectionKnown)a.selectionKnown=b.selectionKnown=false;
            if(a.aim.known!=b.aim.known||a.aim.quadrants!=b.aim.quadrants||a.aim.damageGroups!=b.aim.damageGroups||a.quick!=b.quick)a.aim.known=b.aim.known=false;}
        result.chainsawRules.erase(std::unique(result.chainsawRules.begin(),result.chainsawRules.end(),[](const auto& a,const auto& b){return a.name==b.name;}),result.chainsawRules.end());
        syncMergeAnimationRules(result.animationRules);
        if(sameList&&coherent&&!result.animationRules.empty()&&blueprintRef)animations={level,result.entity,blueprintRef,result.animationRules};
        else if(!ownedAuthoringKnown&&sameList&&coherent&&result.animationRules.empty()&&animations.level==level&&animations.target.address==result.entity.address&&
                animations.target.slot==result.entity.slot&&animations.target.id==result.entity.id&&animations.target.generation==result.entity.generation&&
                blueprintRef.address==animations.blueprint.address&&blueprintRef.id==animations.blueprint.id&&blueprintRef.generation==animations.blueprint.generation&&
                enemyTelemetryIdentity(memory,base,profile,animations.blueprint)){result.animationRules=animations.rows;result.blueprint=animations.blueprint;}
        if(sameList&&!result.chainsawAnimations.empty()){chainsawLevel=level;chainsawTarget=result.entity;cachedChainsawAnimations=result.chainsawAnimations;cachedChainsawRules=result.chainsawRules;}
        else if(!ownedAuthoringKnown&&sameList&&chainsawLevel==level&&chainsawTarget.address==result.entity.address&&chainsawTarget.id==result.entity.id&&chainsawTarget.generation==result.entity.generation&&chainsawTarget.slot==result.entity.slot){result.chainsawAnimations=cachedChainsawAnimations;result.chainsawRules=cachedChainsawRules;for(auto& rule:result.chainsawRules)rule.selectionKnown=false;result.hasChainsaw|=!result.chainsawAnimations.empty();}
        for(auto* values:{&result.gloryKills,&result.chainsaw,&result.chainsawAnimations}) {std::sort(values->begin(),values->end());values->erase(std::unique(values->begin(),values->end()),values->end());}
        uintptr_t selectorOwner=0;uint32_t selectorToken=0;
        if(result.selectionKnown&&(!memory.get(player+0x70e8+0x50,selectorOwner)||selectorOwner!=blueprint||
            !memory.get(player+0x70e8+0x48,selectorToken)||selectorToken!=blueprintToken||!enemyTelemetryIdentity(memory,base,profile,blueprintRef)))result.selectionKnown=false;
        if(!enemyTelemetryIdentity(memory,base,profile,result.entity)){result={};result.phase="changing";return result;}
        result.candidatesReady=coherent;return result;
    }
    SyncTargetFrame capture(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const LevelIdentity& level,const GameCamera& camera,float distanceLimit=50,const GraphLookup* lookup=nullptr) {
        auto value=captureOnce(memory,base,profile,level,camera,distanceLimit,lookup,&retainedFrame);
        // Retry a changing publication once. Never fill an incoherent frame
        // with an old actor, pose, target list or animation candidate.
        if(value.phase=="changing")value=captureOnce(memory,base,profile,level,camera,distanceLimit,lookup,&retainedFrame);
        if(value.phase=="ready"&&value.entity)retainedFrame=value;else retainedFrame={};
        return value;
    }
};
static std::string syncZoneLabel(std::string name) {
    for(const char* prefix:{"kill_","cut_"})if(name.rfind(prefix,0)==0)name.erase(0,strlen(prefix));
    std::replace(name.begin(),name.end(),'_',' ');if(!name.empty()&&name[0]>='a'&&name[0]<='z')name[0]-='a'-'A';return name;
}
static std::shared_ptr<const SyncTargetFrame> syncTargetFrame;
static std::vector<std::string> syncAnimationChoices(const SyncTargetFrame& value) {
    if(value.action==1)return value.chainsawAnimations;
    std::vector<std::string> choices;for(const auto& rule:value.animationRules)if(rule.action==0)choices.push_back(rule.name);
    for(const auto& region:value.regions)if(std::find(choices.begin(),choices.end(),region.name)==choices.end())choices.push_back(region.name);return choices;
}
static bool syncFocusMatches(const SyncAimRequirement& rule,const SyncJointFocus& focus){return rule.damageGroups.empty()||(focus.known&&std::find(rule.damageGroups.begin(),rule.damageGroups.end(),focus.group)!=rule.damageGroups.end());}
static std::string syncSelectedAnimation(const SyncTargetFrame& value) {
    if(!value.selectedAnimation.empty())return value.selectedAnimation;
    if(value.automaticResolved)return value.automaticAnimation;
    // Raw-reader callers also avoid defaulting to an aerial animation on the
    // ground. The HUD's automatic resolver additionally checks actual areas.
    if(value.action==0)for(const auto& region:value.regions)if(region.minZ==0&&syncAimDefinitelyMatches(region.aim,value.aim.quadrant)&&syncFocusMatches(region.aim,value.jointFocus))return region.name;
    const auto choices=syncAnimationChoices(value);return choices.empty()?std::string{}:choices.front();
}
static bool syncSelectedVolume(const SyncTargetFrame& value,SyncRangeVolume& volume,std::string& selected,bool requireInterior=true) {
    const auto choices=syncAnimationChoices(value);selected=syncSelectedAnimation(value);
    if(selected.empty()||std::find(choices.begin(),choices.end(),selected)==choices.end())return false;
    if(value.action==1){if(!value.hasChainsaw)return false;return syncRangeBuild(nullptr,value.facing,value.ranges,true,value.incomingYaw,volume,requireInterior);}
    bool found=false;
    for(const auto& region:value.regions)if(region.name==selected){SyncRangeVolume branch;
        if(!syncRangeBuild(&region,value.facing,value.ranges,false,value.incomingYaw,branch,requireInterior))continue;
        if(!found||syncRangeContains(branch,value.ranges.playerOrigin)){volume=std::move(branch);found=true;if(syncRangeContains(volume,value.ranges.playerOrigin))break;}}
    return found;
}
static const SyncAimRequirement* syncSelectedAim(const SyncTargetFrame& value,const std::string& selected) {
    for(const auto& rule:value.animationRules)if(rule.action==value.action&&rule.name==selected)return &rule.aim;
    if(value.action==1){for(const auto& rule:value.chainsawRules)if(rule.name==selected)return &rule.aim;}
    else for(const auto& rule:value.regions)if(rule.name==selected)return &rule.aim;return nullptr;
}
static const SyncRuleAudit* syncSelectedAudit(const SyncTargetFrame& value,const std::string& selected) {
    for(const auto& rule:value.animationRules)if(rule.action==value.action&&rule.name==selected)return &rule.audit;
    if(value.action==1){for(const auto& rule:value.chainsawRules)if(rule.name==selected)return &rule.audit;}
    else for(const auto& rule:value.regions)if(rule.name==selected)return &rule.audit;return nullptr;
}
static std::string syncAnimationAimText(const SyncTargetFrame& value,const std::string& name) {
    const auto* aim=syncSelectedAim(value,name);const auto* audit=syncSelectedAudit(value,name);
    if((audit&&audit->checked&&!audit->matched)||!aim||!aim->known)return "Aim rules unavailable for this variant";
    const auto kind=syncBodyAimKind(aim,value.spheres);std::string text;
    if(kind==SyncBodyAimKind::Specific)text="Aim at "+syncBodyGroupLabel(aim->damageGroups);
    else if(kind==SyncBodyAimKind::Missing)text="Required body part not available in this pose";
    else if(kind==SyncBodyAimKind::Unknown)text="Required body area unavailable";
    else if(kind==SyncBodyAimKind::AllCurrentGroups)text="No specific body part";
    else text="No specific body part";
    if(!syncAimUnrestricted(aim->quadrants))text+=" | "+syncAimMaskLabel(aim->quadrants);else if(kind==SyncBodyAimKind::None||kind==SyncBodyAimKind::AllCurrentGroups)text+=" | aim anywhere on target";
    return text;
}
static std::vector<SyncAnimationDisplayHint> syncAnimationDisplayHints(const SyncTargetFrame& value) {
    std::vector<SyncAnimationDisplayHint> result;for(const auto& name:syncAnimationChoices(value)){const auto* audit=syncSelectedAudit(value,name);
        result.push_back({name,audit&&audit->matched?audit->enemy:std::string{},syncAnimationAimText(value,name)});}return result;
}
static uint64_t syncAimMeshSignature(const GameCamera& camera,const SyncTargetFrame& value,const std::string& selected) {
    uint64_t hash=1469598103934665603ull;auto bytes=[&](const void* at,size_t count){const auto* data=static_cast<const unsigned char*>(at);for(size_t i=0;i<count;++i){hash^=data[i];hash*=1099511628211ull;}};
    auto string=[&](const std::string& s){bytes(s.data(),s.size());const unsigned char end=0;bytes(&end,1);};
    string(selected);bytes(&value.action,sizeof(value.action));for(uintptr_t p:{value.entity.address,value.entity.map,value.entity.slot})bytes(&p,sizeof(p));
    bytes(&value.entity.id,sizeof(value.entity.id));bytes(&value.entity.generation,sizeof(value.entity.generation));bytes(camera.matrix,64);bytes(camera.position,12);
    bytes(value.shotBounds,24);bytes(&value.aim.known,sizeof(bool));bytes(value.aim.bounds,24);
    if(const auto* aim=syncSelectedAim(value,selected)){bytes(&aim->known,sizeof(bool));bytes(&aim->quadrants,4);string(syncAimGroupKey(aim->damageGroups));}
    for(const auto& s:value.spheres){bytes(&s.center,sizeof(Vec3));bytes(&s.radius,4);bytes(&s.focusKnown,sizeof(bool));string(s.focusGroup);}return hash?hash:1;
}
static void syncRefreshAimMesh(const GameCamera& camera,SyncTargetFrame& value) {
    const auto selected=syncSelectedAnimation(value);value.aimPartMesh=std::make_shared<const SyncAimPartMesh>(syncBuildAimPartMesh(camera,value.shotBounds,value.aim,syncSelectedAim(value,selected),value.spheres));
    value.aimPartAnimation=selected;value.aimPartAction=value.action;value.aimPartEntity=value.entity;value.aimPartSignature=syncAimMeshSignature(camera,value,selected);
}
struct SyncNativeSelection {bool known=false;std::vector<std::string> possible;};
// Current valid native candidates only. Never extrapolate these sites into
// future player positions or turn this observation into an activation promise.
static SyncNativeSelection syncNativeSelection(const SyncTargetFrame& value) {
    SyncNativeSelection result;
    if(!value.selectionKnown||!value.candidatesReady||(value.action==0?value.pendingGloryKill:value.pendingChainsaw)||
       !std::isfinite(value.selectionTolerance)||value.selectionTolerance<=0||!syncRangeFinite(value.ranges.playerOrigin))return result;
    const auto& eligible=value.action==0?value.gloryKills:value.chainsaw;std::vector<const SyncNativeChoice*> candidates;
    for(const auto& name:eligible){bool found=false;for(const auto& row:value.nativeChoices)if(row.action==value.action&&row.name==name){if(!row.known)return result;candidates.push_back(&row);found=true;}if(!found)return result;}
    result.known=true;if(candidates.empty())return result;
    float best=-FLT_MAX;for(const auto* c:candidates)best=std::max(best,c->priority);
    double nearest=DBL_MAX;for(const auto* c:candidates)if(c->priority==best)nearest=std::min(nearest,syncSelectionDistance(value.ranges.playerOrigin,c->position));
    for(const auto* c:candidates)if(c->priority==best&&syncSelectionDistance(value.ranges.playerOrigin,c->position)<nearest+value.selectionTolerance+.0001)result.possible.push_back(c->name);
    std::sort(result.possible.begin(),result.possible.end());result.possible.erase(std::unique(result.possible.begin(),result.possible.end()),result.possible.end());return result;
}
static bool syncAimRuleReady(const SyncAimRequirement& rule,const SyncTargetFrame& value){return rule.known&&syncAimMaskKnown(rule.quadrants)&&(syncAimUnrestricted(rule.quadrants)||value.aim.known);}
static bool syncRulePossible(const SyncAimRequirement& rule,const SyncTargetFrame& value){return syncAimMatches(rule,value.aim.quadrant)&&(!value.jointFocus.known||syncFocusMatches(rule,value.jointFocus));}
// An uninstantiated alternative belongs to an existing interaction only when
// both names were audited against that exact assigned declaration. Its start
// pose is never borrowed for selecting that inactive alternative itself.
static bool syncChainsawAlternativeCovered(const SyncTargetFrame& value,const std::string& name) {
    for(const auto& authored:value.animationRules)if(authored.action==1&&authored.name==name&&authored.audit.matched)
        for(const auto& active:value.chainsawRules)if(active.audit.matched&&active.audit.declaration==authored.audit.declaration&&syncSameAimRule(active.aim,authored.aim))return true;
    return false;
}
static SyncExclusiveGuide syncBuildExclusiveGuide(const SyncTargetFrame& value,bool bodyClearance=false,bool requiredAimReference=false,size_t selectedBranch=SIZE_MAX) {
    SyncExclusiveGuide result;SyncRangeVolume selected;std::string name;
    if(!value.rulesCoherent){result.detail="Animation rules are changing | standing area unavailable";return result;}
    if(!syncSelectedVolume(value,selected,name,false)){result.detail=!value.ranges.known?value.ranges.detail:value.automaticResolved&&value.selectedAnimation.empty()&&!value.automaticDetail.empty()?value.automaticDetail:"No supported area for this selection on this enemy";return result;}
    if(value.action==0&&selectedBranch==SIZE_MAX){std::vector<size_t> indices;
        for(size_t i=0;i<value.regions.size();++i)if(value.regions[i].name==name)indices.push_back(i);
        if(indices.size()>1){std::vector<SyncExclusiveGuide> branches;
            for(auto i:indices)branches.push_back(syncBuildExclusiveGuide(value,bodyClearance,requiredAimReference,i));
            return syncExclusiveUnion(std::move(branches));}}
    if(value.action==0&&selectedBranch!=SIZE_MAX&&!syncRangeBuild(&value.regions[selectedBranch],value.facing,value.ranges,false,value.incomingYaw,selected,false)){
        result.detail="Same-animation native branch unsupported";return result;}
    result.selected=selected;result.z=value.ranges.playerOrigin.z;
    const auto* aim=syncSelectedAim(value,name);
    const bool planningAim=requiredAimReference&&value.action==0&&aim&&
        ((!aim->damageGroups.empty()&&syncBodyAimKind(aim,value.spheres)==SyncBodyAimKind::Specific)||
         (value.automaticResolved&&aim->damageGroups.empty()&&!syncAimUnrestricted(aim->quadrants)));
    if(!aim||!syncAimRuleReady(*aim,value)){result.detail="Aim requirements unavailable for this animation";return result;}
    if(!planningAim&&!syncAimDefinitelyMatches(*aim,value.aim.quadrant)){result.known=true;result.selected=selected;result.z=value.ranges.playerOrigin.z;
        result.detail="Aim at "+syncAimMaskLabel(aim->quadrants)+" of the target, away from centre lines; standing area hidden for this aim";return result;}
    if(!planningAim&&!aim->damageGroups.empty()&&!value.jointFocus.known){result.detail="Native joint focus unavailable for this animation";return result;}
    if(!planningAim&&!syncFocusMatches(*aim,value.jointFocus)){result.known=true;result.detail="Aim at the required joint group | current native focus does not match";return result;}
    if(bodyClearance&&!value.playerFootprint.known){result.detail="Player collider bounds unavailable | clearance area hidden";return result;}
    std::vector<SyncRangeVolume> competitors;
    const SyncChainsawRule* chosenRule=nullptr;std::vector<const SyncChainsawRule*> selectionOthers;
    if(value.action==0){if(value.regionState!="ready"){result.detail="Exclusive area unavailable: some animation rules are unknown";return result;}
        const auto chosen=selectedBranch==SIZE_MAX?std::find_if(value.regions.begin(),value.regions.end(),[&](const auto& r){return r.name==name;}):value.regions.begin()+selectedBranch;
        for(const auto& region:value.regions)if(region.name!=name){SyncRangeVolume other;
            if(!syncAimRuleReady(region.aim,value)){result.detail="Exclusive area unavailable: competing aim rule unknown";return result;}
            if(chosen!=value.regions.end()&&!syncGkCanCompete(*chosen,region))continue;
            auto effectiveAim=*aim;
            // Use the accepted current screen half while planning a specific
            // limb. Wrong-aim previews still retain the full authored scope.
            if(planningAim&&value.aim.known&&syncAimDefinitelyMatches(*aim,value.aim.quadrant))effectiveAim.quadrants=value.aim.quadrant;
            if(planningAim?!syncAimScopesOverlap(effectiveAim,region.aim):!syncRulePossible(region.aim,value))continue;
            if(planningAim&&aim->damageGroups.empty()&&value.jointFocus.known&&!syncFocusMatches(region.aim,value.jointFocus))continue;
            // Building constraints does not require a guessed interior seed.
            // A competing volume must never be ignored merely for failing it.
            if(!syncRangeBuild(&region,value.facing,value.ranges,false,value.incomingYaw,other,false)){result.detail="Exclusive area unavailable: competing rule unsupported";return result;}
            for(auto part:syncRangeConvexParts(other))competitors.push_back(std::move(part));}}
    else {bool chosenQuick=false;
        for(const auto& animation:value.chainsawAnimations){const auto found=std::find_if(value.chainsawRules.begin(),value.chainsawRules.end(),[&](const auto& rule){return rule.name==animation;});
            if(found==value.chainsawRules.end()&&animation!=name&&syncChainsawAlternativeCovered(value,animation))continue;
            if(found==value.chainsawRules.end()||!syncAimRuleReady(found->aim,value)){result.detail=animation==name?"Selected Chainsaw alternative is not currently prepared by the game":"Exclusive Chainsaw area unavailable: native aim rule unknown";return result;}
            if(animation==name){chosenQuick=found->quick;chosenRule=&*found;continue;}
            if(syncRulePossible(found->aim,value))selectionOthers.push_back(&*found);}
        if(!chosenQuick){result.detail="This Chainsaw interaction uses unsupported position checks";return result;}
        if(!selectionOthers.empty()){
            if(!value.selectionKnown||!chosenRule||!chosenRule->selectionKnown||std::any_of(selectionOthers.begin(),selectionOthers.end(),[](const auto* rule){return !rule->selectionKnown||!rule->quick;})){
                result.detail="Waiting for coherent native Chainsaw priorities and start poses";return result;}
            if(std::any_of(selectionOthers.begin(),selectionOthers.end(),[&](const auto* rule){return rule->priority>chosenRule->priority;})){
                result.known=true;result.selected=selected;result.z=value.ranges.playerOrigin.z;result.currentPoses=true;
                result.detail="Selected Chainsaw has lower native priority | no area for this snapshot";return result;}}
    }
    if(selected.wideFacing)competitors.push_back(syncRangeOutsideWide(selected));
    if(bodyClearance){selected=syncFootprintVolume(std::move(selected),value.playerFootprint,false);for(auto& other:competitors)other=syncFootprintVolume(std::move(other),value.playerFootprint,true);}
    result=syncExclusiveBuild(std::move(selected),std::move(competitors),value.ranges.playerOrigin.z);
    result.requiredAimReference=planningAim;
    if(value.action==1&&chosenRule&&!selectionOthers.empty()){
        std::vector<Vec3> equalPriority;for(const auto* rule:selectionOthers)if(rule->priority==chosenRule->priority)equalPriority.push_back(rule->selectionPosition);
        if(equalPriority.empty()){result.currentPoses=true;result.detail="Selected Chainsaw has sole highest native priority | CURRENT snapshot";}
        else {Vec3 site=chosenRule->selectionPosition;double tolerance=value.selectionTolerance;
            if(bodyClearance){const Vec3 center{(value.playerFootprint.minimum.x+value.playerFootprint.maximum.x)*.5f,(value.playerFootprint.minimum.y+value.playerFootprint.maximum.y)*.5f,0};
                site=subtract(site,center);for(auto& other:equalPriority)other=subtract(other,center);tolerance+=2*syncFootprintRadius(value.playerFootprint);}
            syncExclusiveSelectPoses(result,site,equalPriority,tolerance);}}
    return result;
}
static SyncExclusiveGuide syncBuildReferenceGuide(const SyncTargetFrame& value,bool bodyClearance=true) {
    auto reference=value;reference.ranges=syncTargetReferenceLimits(value.facing,value.ranges);
    if(value.retainedTarget){reference.aim=value.retainedAim;reference.jointFocus=value.retainedFocus;}
    bool currentSelector=value.selectionKnown;
    for(const auto& name:value.chainsawAnimations)if(std::none_of(value.chainsawRules.begin(),value.chainsawRules.end(),[&](const auto& rule){return rule.name==name&&rule.selectionKnown;})&&!syncChainsawAlternativeCovered(value,name))currentSelector=false;
    const bool retainedSelector=value.retainedTarget&&value.action==1&&!currentSelector&&value.retainedSelectionKnown&&
        syncFacingOrthonormal(value.retainedSelectionPose.axis)&&syncPreviewFinite(value.retainedSelectionPose.origin);
    if(retainedSelector){reference.selectionKnown=true;reference.selectionTolerance=value.retainedSelectionTolerance;reference.chainsawRules=value.retainedChainsawRules;
        // Keep the sampled selector arrangement in the enemy's frame. This is
        // a reference diagram, never a refreshed native candidate observation.
        for(auto& rule:reference.chainsawRules){const auto delta=subtract(rule.selectionPosition,value.retainedSelectionPose.origin);Vec3 local{};
            local={dot(delta,{value.retainedSelectionPose.axis[0],value.retainedSelectionPose.axis[1],value.retainedSelectionPose.axis[2]}),
                   dot(delta,{value.retainedSelectionPose.axis[3],value.retainedSelectionPose.axis[4],value.retainedSelectionPose.axis[5]}),
                   dot(delta,{value.retainedSelectionPose.axis[6],value.retainedSelectionPose.axis[7],value.retainedSelectionPose.axis[8]})};
            rule.selectionPosition=motionAdd(value.facing.origin,syncRangeTransform(value.facing,local));}}
    reference.exclusiveGuide.reset();reference.referenceGuide.reset();reference.sectorMesh.reset();
    auto guide=syncBuildExclusiveGuide(reference,bodyClearance,true);
    if(retainedSelector&&guide.known)guide.detail="Reference from last aimed Chainsaw poses | aim at target to refresh";
    return guide;
}
class SyncAutomaticSelection {
    LevelIdentity level;EntityReference target;std::array<std::string,2> choices;
public:
    void clear(){*this={};}
    void update(SyncTargetFrame& value) {
        if(value.level!=level||value.entity.address!=target.address||value.entity.id!=target.id||value.entity.generation!=target.generation||value.entity.slot!=target.slot){clear();level=value.level;target=value.entity;}
        value.automaticResolved=value.selectedAnimation.empty();value.automaticAnimation.clear();value.automaticDetail.clear();
        value.exclusiveGuide.reset();value.referenceGuide.reset();value.sectorMesh.reset();value.originSectorMesh.reset();value.clearanceBandMesh.reset();value.aimPartMesh.reset();value.aimPartSignature=0;value.sectorToneKnown=false;
        if(!value.automaticResolved)return;
        auto& previous=choices[value.action];const auto available=syncAnimationChoices(value);
        auto aimNow=[&](const std::string& name){const auto* aim=syncSelectedAim(value,name);return aim&&syncAimDefinitelyMatches(*aim,value.retainedTarget?value.retainedAim.quadrant:value.aim.quadrant)&&syncFocusMatches(*aim,value.retainedTarget?value.retainedFocus:value.jointFocus);};
        auto drawable=[&](const SyncExclusiveGuide& guide){if(!guide.known||guide.pieces.empty())return false;const auto mesh=syncBuildSectorMesh(guide,value.shotBounds);return mesh.known&&!mesh.fill.empty();};
        std::string best,unavailable;int bestRank=-1;
        const auto limits=syncTargetReferenceLimits(value.facing,value.ranges);
        // Check cheap enclosing native volumes first. A former selection may
        // remain a fallback, but it must not mask entry into a different zone.
        for(const auto& animation:available){value.automaticAnimation=animation;const bool matchingAim=aimNow(animation);
            const auto* aim=syncSelectedAim(value,animation);
            const bool conditional=value.action==0&&aim&&(aim->damageGroups.empty()||syncBodyAimKind(aim,value.spheres)==SyncBodyAimKind::Specific);
            if(!matchingAim&&!conditional)continue;
            SyncRangeVolume enclosing;bool supported=false;
            if(value.action==1)supported=syncRangeBuild(nullptr,value.facing,limits,true,value.incomingYaw,enclosing,false);
            else for(const auto& region:value.regions)if(region.name==animation){supported=syncRangeBuild(&region,value.facing,limits,false,value.incomingYaw,enclosing,false);if(supported&&syncRangeContains(enclosing,value.ranges.playerOrigin))break;}
            if(!supported||!syncRangeContains(enclosing,value.ranges.playerOrigin))continue;
            const auto guide=syncBuildReferenceGuide(value);
            if(!guide.known){if(unavailable.empty())unavailable=guide.detail;continue;}
            const bool core=syncExclusiveContains(guide,value.ranges.playerOrigin);
            const auto origin=core?SyncExclusiveGuide{}:syncBuildReferenceGuide(value,false);
            const bool entered=core||syncExclusiveContains(origin,value.ranges.playerOrigin);
            if(!entered||(!drawable(guide)&&!drawable(origin)))continue;
            const int rank=(matchingAim?4:2)+(core?1:0);
            if(rank>bestRank||(rank==bestRank&&animation==previous)){best=animation;bestRank=rank;}}
        if(best.empty()&&!previous.empty()&&std::find(available.begin(),available.end(),previous)!=available.end()&&aimNow(previous)){
            value.automaticAnimation=previous;if(drawable(syncBuildReferenceGuide(value)))best=previous;}
        if(best.empty()){double distance=DBL_MAX;
            for(const auto& animation:available){value.automaticAnimation=animation;if(!aimNow(animation))continue;const auto guide=syncBuildReferenceGuide(value);
                if(!guide.known){if(unavailable.empty())unavailable=guide.detail;continue;}if(!drawable(guide))continue;
                for(const auto& piece:guide.pieces){Vec3 point{0,0,guide.z};for(auto vertex:piece){point.x+=float(vertex.x/piece.size());point.y+=float(vertex.y/piece.size());}
                    if(!syncExclusiveNativeContains(guide,point))continue;const auto delta=subtract(point,value.ranges.playerOrigin);const double score=dot(delta,delta);
                    if(score<distance){distance=score;best=animation;}}}}
        previous=best;value.automaticAnimation=best;
        if(best.empty())value.automaticDetail=unavailable.empty()?"Automatic: no unique area for current aim, foot height and player-hitbox clearance":unavailable;
    }
};
struct SyncGuideFeedback {
    bool selectedOnlyObserved=false;
    SyncSectorTone tone=SyncSectorTone::Waiting;
    std::vector<std::string> otherGeometry;
    std::string detail;
};
static SyncGuideFeedback syncGuideFeedback(const SyncTargetFrame& value,const SyncRangeVolume& volume,const std::string& selected) {
    SyncGuideFeedback result;
    if(value.retainedTarget){result.detail="AMBER: retained target within action radius | aim at target to confirm";return result;}
    if(const auto* aim=syncSelectedAim(value,selected);aim){
        if(!syncAimDefinitelyMatches(*aim,value.aim.quadrant)||(!aim->damageGroups.empty()&&(!value.jointFocus.known||!syncFocusMatches(*aim,value.jointFocus)))){
            result.detail="AMBER: standing reference assumes aim at the highlighted aim area | current aim not confirmed";return result;}}
    const auto native=syncNativeSelection(value);const bool nativePossible=native.known&&std::find(native.possible.begin(),native.possible.end(),selected)!=native.possible.end();
    if(value.action==1&&!syncRangeContains(volume,value.ranges.playerOrigin)){result.tone=SyncSectorTone::Reference;result.detail="Outside selected geometry";return result;}
    SyncRangeVolume actual;std::string actualName;
    if(!syncSelectedVolume(value,actual,actualName,false)||actualName!=selected){result.detail="AMBER: current native geometry unavailable";return result;}
    if(!syncRangeContains(actual,value.ranges.playerOrigin)){
        if(nativePossible){result.detail="AMBER: reference geometry differs from current native eligibility | snapshot";return result;}
        result.tone=SyncSectorTone::Rejected;
        const auto status=syncGuideStatus(actual,value.ranges.playerOrigin,value.action==1);
        result.detail=status.direction?"RED: current aim / body facing does not fit this animation | sector stays fixed":"RED: current camera / position fails native limits | sector is a reference";return result;}
    if(value.action==0){const SyncFacingRegion* chosen=nullptr;
        for(const auto& r:value.regions)if(r.name==selected){SyncRangeVolume branch;
            if(syncRangeBuild(&r,value.facing,value.ranges,false,value.incomingYaw,branch,false)&&syncRangeContains(branch,value.ranges.playerOrigin)&&
               (!chosen||(r.priorityKnown&&(!chosen->priorityKnown||r.priority>chosen->priority))))chosen=&r;}
        for(const auto& region:value.regions){SyncRangeVolume other;
        if(chosen&&!syncGkCanCompete(*chosen,region))continue;
        if(region.name!=selected&&(!syncAimRuleReady(region.aim,value)||syncRulePossible(region.aim,value))&&syncRangeBuild(&region,value.facing,value.ranges,false,value.incomingYaw,other,false)&&syncRangeContains(other,value.ranges.playerOrigin))result.otherGeometry.push_back(region.name);}}
    else {const auto guide=value.exclusiveGuide?value.exclusiveGuide:std::make_shared<const SyncExclusiveGuide>(syncBuildExclusiveGuide(value));
        for(const auto& rule:value.chainsawRules)if(rule.name!=selected&&(!syncAimRuleReady(rule.aim,value)||syncRulePossible(rule.aim,value))&&
            (!guide->known||!syncExclusiveContains(*guide,value.ranges.playerOrigin)))result.otherGeometry.push_back(rule.name);}
    if(!result.otherGeometry.empty()){result.tone=SyncSectorTone::Waiting;result.detail="AMBER: competing geometry | selected animation may still occur | also fits ";
        for(size_t i=0;i<std::min(size_t(3),result.otherGeometry.size());++i){if(i)result.detail+=" / ";result.detail+=syncZoneLabel(result.otherGeometry[i]);}return result;}
    if(!value.playerFootprint.known){result.detail="AMBER: player collider bounds unavailable";return result;}
    const auto bodyGuide=syncBuildExclusiveGuide(value,true);
    if(!bodyGuide.known||!syncExclusiveContains(bodyGuide,value.ranges.playerOrigin)){
        result.detail="AMBER: full player footprint does not fit a unique native area | boundary clearance";return result;}
    if(!value.candidatesReady||(value.action==0?value.pendingGloryKill:value.pendingChainsaw)){result.detail="AMBER: inside geometry | engine checks still pending";return result;}
    if(native.known&&!nativePossible){result.tone=SyncSectorTone::Rejected;result.detail="RED: selected is outside the current native priority / distance band";return result;}
    const auto& candidates=value.action==0?value.gloryKills:value.chainsaw;
    if(std::find(candidates.begin(),candidates.end(),selected)==candidates.end()){result.tone=SyncSectorTone::Rejected;result.detail="RED: selected GK not confirmed by engine";if(value.action==1)result.detail="RED: selected Chainsaw not confirmed by engine";return result;}
    if(value.action==1){const auto guide=value.exclusiveGuide?value.exclusiveGuide:std::make_shared<const SyncExclusiveGuide>(syncBuildExclusiveGuide(value));
        if(guide->currentPoses&&syncExclusiveContains(*guide,value.ranges.playerOrigin)){
            result.detail="AMBER: sole distance choice for current poses | activation not guaranteed";return result;}}
    if(candidates.size()!=1){result.detail="AMBER: multiple engine candidates | selected animation not unique";return result;}
    if(value.action==0&&value.regionState!="ready"){result.detail="AMBER: partial geometry | other rules unavailable";return result;}
    result.selectedOnlyObserved=true;result.tone=SyncSectorTone::Confirmed;result.detail="GREEN: footprint fits + selected is sole observed candidate | snapshot";return result;
}
static SyncSectorTone syncTargetSectorTone(const SyncTargetFrame& value,const SyncExclusiveGuide& guide,const SyncSectorMesh& mesh,const std::string& selected) {
    if(!syncExclusiveContains(guide,value.ranges.playerOrigin))return value.playerFootprint.known&&syncSectorNearBoundary(mesh,value.ranges.playerOrigin,syncFootprintRadius(value.playerFootprint))?SyncSectorTone::Waiting:SyncSectorTone::Reference;
    return syncGuideFeedback(value,guide.selected,selected).tone;
}
static std::vector<std::string> syncPositionGuideLines(const SyncTargetFrame& value) {
    std::vector<std::string> lines{value.action==1?"LAB Plus | Enemy-anchored Chainsaw reference":"LAB Plus | Enemy-anchored GK reference"};
    const std::string selected=syncSelectedAnimation(value);
    if(const auto* audit=syncSelectedAudit(value,selected);audit&&audit->matched)lines.push_back("Rule set: "+audit->enemy);
    lines.push_back((value.selectedAnimation.empty()?"Automatic: ":"Selected: ")+(selected.empty()?std::string("no matching unique area"):syncZoneLabel(selected)));
    if(!selected.empty())lines.push_back(syncAnimationAimText(value,selected));
    const auto observed=syncNativeSelection(value);
    if(!selected.empty()){
        const bool possible=std::find(observed.possible.begin(),observed.possible.end(),selected)!=observed.possible.end();
        lines.push_back(!observed.known?"Engine now: selection checks unavailable / pending":!possible?"Engine now: selected animation is outside the observed choice set":observed.possible.size()==1?"Engine now: selected is sole distance-band choice | snapshot":"Engine now: selected remains possible with other animations | snapshot");}
    const auto* aim=syncSelectedAim(value,selected);
    if(aim&&aim->known){lines.push_back("Aim quadrants: "+syncAimMaskLabel(aim->quadrants));
        if(!aim->damageGroups.empty()){const auto bodyKind=syncBodyAimKind(aim,value.spheres);
            if(bodyKind==SyncBodyAimKind::AllCurrentGroups)lines.push_back("Body focus: all current hit groups accepted | no specific part to aim at");
            else {std::string groups;for(const auto& group:aim->damageGroups){if(!groups.empty())groups+=" / ";groups+=group;}lines.push_back("Required native joint focus: "+groups);
            if(value.aimPartMesh)lines.push_back(!value.aimPartMesh->known?"Required body aim area unavailable":value.aimPartMesh->fill.empty()?"Required body aim area hidden or too small from this view":"Yellow on target box: aim at the highlighted body area");
            }lines.push_back("Current native joint focus: "+(value.jointFocus.known?(value.jointFocus.group.empty()?std::string("none"):value.jointFocus.group):std::string("unavailable")));}
        if(!syncAimUnrestricted(aim->quadrants))lines.push_back(value.aim.known?"Current aim: "+syncAimMaskLabel(value.aim.quadrant):"Current aim reference unavailable");}
    if(value.action==0)for(const auto& region:value.regions)if(region.name==selected){std::ostringstream text;text.imbue(std::locale::classic());text<<std::fixed<<std::setprecision(2)<<"Authored start XY limit: "<<region.maxXY<<" m | foot-height difference: "<<region.minZ<<" - "<<region.maxZ<<" m";lines.push_back(text.str());break;}
    lines.push_back("Amber boundary band = origin fits, full player-hitbox clearance does not");
    if(observed.known&&observed.possible.size()>1)lines.push_back("Shared native choices are deliberately unfilled | either animation may occur there");
    const auto guide=value.referenceGuide?value.referenceGuide:std::make_shared<const SyncExclusiveGuide>(syncBuildReferenceGuide(value));
    if(guide->requiredAimReference)lines.push_back("Standing area assumes aim at the highlighted aim area | current aim checked separately");
    SyncRangeVolume actual;std::string actualName;const bool actualKnown=syncSelectedVolume(value,actual,actualName,false);
    const auto status=syncGuideStatus(guide->selected,value.ranges.playerOrigin,value.action==1);
    auto metres=[](double distance){std::ostringstream text;text.imbue(std::locale::classic());text<<std::fixed<<std::setprecision(2)<<distance;return text.str();};
    if(actualKnown&&!actual.balls.empty()){const auto current=syncGuideStatus(actual,value.ranges.playerOrigin,value.action==1);lines.push_back("Camera to target: "+metres(current.cameraDistance)+" m | limit "+metres(current.cameraLimit)+" m");}
    lines.push_back("Enemy-anchored reference sector | current aim / body facing checked separately");
    if(value.retainedTarget)lines.push_back("Target retained within action radius | last aimed area is a reference; aim back to confirm");
    if(value.playerFootprint.known)lines.push_back("Player-hitbox clearance: "+metres(value.playerFootprint.maximum.x-value.playerFootprint.minimum.x)+" x "+metres(value.playerFootprint.maximum.y-value.playerFootprint.minimum.y)+" m (horizontal)");
    lines.push_back("Grey = reference | Amber = competition / clearance / pending | Red = rejected | Green = sole observed choice");
    if(!guide->known){lines.push_back(guide->detail);return lines;}
    if(guide->currentPoses){lines.push_back(guide->detail);lines.push_back("Conditional on current poses / priorities | activation checks still apply");}
    if(guide->pieces.empty()){lines.push_back(guide->detail);lines.push_back("No reference area or movement destination for these conditions");return lines;}
    const auto intervals=syncExclusiveApproach(*guide,value.ranges.playerOrigin);
    if(intervals.empty())lines.push_back("No reference interval on this approach | move around the enemy");
    else {std::string text="Reference distance on this approach: ";for(size_t i=0;i<std::min(size_t(3),intervals.size());++i){if(i)text+=" / ";text+=metres(intervals[i].first)+" - "+metres(intervals[i].second)+" m";}
        if(intervals.size()>3)text+=" (+"+std::to_string(intervals.size()-3)+" intervals)";lines.push_back(text+" (reference camera distance)");}
    const bool inside=syncExclusiveContains(*guide,value.ranges.playerOrigin);const auto feedback=syncGuideFeedback(value,guide->selected,selected);
    if(inside)lines.push_back(feedback.detail);
    else {
        if(status.inside)lines.push_back("Outside reference area: competing reference regions or boundary clearance");
        else {std::string reasons;auto add=[&](const char* text){if(!reasons.empty())reasons+=" + ";reasons+=text;};
            if(status.direction)add("player hitbox reaches side boundary");if(status.height)add("height");if(status.cameraRange)add("player hitbox reaches range limit");if(status.startRange)add("animation start clearance");if(status.lunge)add("lunge clearance");lines.push_back("Outside reference clearance: "+reasons);}
        Vec3 destination{};if(syncExclusiveDestination(*guide,value.ranges.playerOrigin,destination)){const auto delta=subtract(destination,value.ranges.playerOrigin);
            lines.push_back("Nearest reference area: "+metres(sqrtf(dot(delta,delta)))+" m away (straight line)");}}
    lines.push_back("Filled sector: height follows enemy box | standing conditions evaluated at your feet");
    lines.push_back("Current aim and stance | enemy state, obstacles and late checks still apply");return lines;
}

class SyncCandidatePresentation {
    LevelIdentity level;EntityReference entity;ULONGLONG windowAt=0;
    bool observed=false;std::vector<std::string> gk,cs,directions;
    static void merge(std::vector<std::string>& target,const std::vector<std::string>& values){for(const auto& v:values)if(target.size()<128)target.push_back(v);std::sort(target.begin(),target.end());target.erase(std::unique(target.begin(),target.end()),target.end());}
public:
    bool ready=false;std::vector<std::string> gloryKills,chainsaw,facing;
    void clear(){*this={};}
    void update(const SyncTargetFrame& value,ULONGLONG now) {
        const bool changed=level!=value.level||entity.slot!=value.entity.slot||entity.id!=value.entity.id||entity.generation!=value.entity.generation||entity.address!=value.entity.address;
        if(changed){clear();level=value.level;entity=value.entity;windowAt=now;}
        if(!changed&&now-windowAt>=125){gloryKills=std::move(gk);chainsaw=std::move(cs);facing=std::move(directions);ready=observed;gk.clear();cs.clear();directions.clear();observed=false;windowAt=now;}
        if(value.candidatesReady){observed=true;merge(gk,value.gloryKills);merge(cs,value.chainsaw);}
        std::vector<std::string> matching;
        for(const auto& r:value.regions)if(syncFacingContains(syncFacingYaw(value.facing,r.incoming),r.halfYaw,value.incomingYaw))matching.push_back(r.name);
        merge(directions,matching);
    }
};
class SyncTargetHud {
    SyncTargetReader reader;SyncCandidatePresentation presentation;SyncAutomaticSelection automatic;LevelIdentity commandLevel;bool layout=false,requested=false;ULONGLONG retryAt=0;
public:
    void clear(bool deactivate=false) {std::atomic_store(&syncTargetFrame,std::shared_ptr<const SyncTargetFrame>{});debugHudStack().clear(DebugHudBlock::SyncTarget);presentation.clear();reader.clearRetainedTarget();if(deactivate){automatic.clear();requested=false;commandLevel={};}}
    void frame(const MeathookHudApi* api,const GameProfile* profile,const GraphLookup& lookup) {
        if(!showSyncTarget.load()||!profile) {clear(true);return;}
        const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));const WallMemory memory;
        const auto level=readLevelIdentity(memory,base,*profile);
        if(level!=commandLevel){requested=false;commandLevel=level;}
        if(!level||!gameplayVisible(memory,base,*profile)||readConsoleFraction(memory,base,*profile)>0) {clear();return;}
        if(!syncPreviewProfile(*profile)) {clear();return;}
        if(!layout&&GetTickCount64()>=retryAt) {retryAt=GetTickCount64()+250;layout=syncPreviewLayouts(lookup);}
        if(!layout) {clear();return;}
        GameCamera camera{};if(!readGameCamera(memory,base,*profile,camera)) {clear();return;}
        auto value=reader.capture(memory,base,*profile,level,camera,syncTargetViewDistance.load(),&lookup);
        if(value.phase!="ready"||readLevelIdentity(memory,base,*profile)!=level) {clear();return;}
        automatic.update(value);
        syncRefreshAimMesh(camera,value);
        presentation.update(value,GetTickCount64());
        if(value.action==1)value.exclusiveGuide=std::make_shared<const SyncExclusiveGuide>(syncBuildExclusiveGuide(value));
        value.referenceGuide=std::make_shared<const SyncExclusiveGuide>(syncBuildReferenceGuide(value));
        auto lines=syncPositionGuideLines(value);
        auto options=[&](const char* title,const std::vector<std::string>& choices,const char* empty) {
            std::string text=title;
            if(!presentation.ready) return text+"checking...";
            if(choices.empty()) return text+empty;
            for(size_t i=0;i<std::min(size_t(4),choices.size());++i) {if(i)text+=" / ";text+=syncZoneLabel(choices[i]).substr(0,50);}
            if(choices.size()>4)text+=" (+"+std::to_string(choices.size()-4)+" more)";
            return text;
        };
        lines.push_back(options("Engine candidates (recent): ",value.action==1?presentation.chainsaw:presentation.gloryKills,"none observed"));
        if((value.action==1?presentation.chainsaw:presentation.gloryKills).size()>1)lines.push_back("Several engine candidates | selected animation is not guaranteed");
        if(value.regionState=="partial facing preview"&&value.action==0)lines.push_back("Partial preview | some animation rules unavailable");
        if(!requested&&api&&api->executeCommand) {api->executeCommand("com_debugHUD 1");requested=true;}
        debugHudStack().publish(DebugHudBlock::SyncTarget,api,lines,16,.72f,0xffffd040u);
        std::atomic_store(&syncTargetFrame,std::make_shared<const SyncTargetFrame>(std::move(value)));
    }
};
static std::shared_ptr<const SyncTargetFrame> syncTargetForDraw(const GameCamera& camera) {
    const auto value=std::atomic_load(&syncTargetFrame);
    if(!showSyncTarget.load()||!value||value->action!=syncTargetAction.load()||value->selectedAnimation!=(*std::atomic_load(&syncAnimationSelection))[value->action]||value->phase!="ready"||GetTickCount64()-value->captured>100) return {};
    const auto* profile=currentGameProfile();const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));const WallMemory memory;
    if(!profile||!syncPreviewProfile(*profile)||readLevelIdentity(memory,base,*profile)!=value->level||!enemyTelemetryIdentity(memory,base,*profile,value->entity)) return {};
    if(value->action==0&&!value->regions.empty()&&!enemyTelemetryIdentity(memory,base,*profile,value->blueprint))return {};
    auto fresh=std::make_shared<SyncTargetFrame>(*value);
    if((!readSyncHitSpheres(memory,base,value->entity,fresh->spheres)&&!readSyncHitSpheres(memory,base,value->entity,fresh->spheres))||
       !enemyTelemetryIdentity(memory,base,*profile,value->entity)||!readSyncFacingPose(memory,base,value->entity,fresh->facing)) return {};
    uintptr_t player=0;
    if(!memory.get(value->level.map+profile->mapPlayerOffset,player)||!readSyncPlayerIncoming(memory,player,fresh->incomingYaw))return {};
    Vec3 eye{},direction{};if(!inspectorRay(camera,eye,direction)) return {};
    float nearest=syncTargetViewDistance.load();bool hit=false;for(const auto& sphere:fresh->spheres)if(syncSphereHit(eye,direction,sphere,nearest))hit=true;
    if(fresh->ranges.known){if(!memory.get(player+0x7d28+0xb0,fresh->ranges.playerOrigin))return {};fresh->ranges.eyeOffset=subtract(eye,fresh->ranges.playerOrigin);
        fresh->viewPosition=eye;fresh->viewTarget=motionAdd(fresh->facing.origin,syncRangeTransform(fresh->facing,fresh->ranges.viewTargetOffset));
        const auto point=hit?motionAdd(eye,motionScale(direction,nearest)):fresh->viewTarget;fresh->ranges.aimOrigin=subtract(point,fresh->ranges.eyeOffset);fresh->ranges.modelViewOffset=remainderf(fresh->incomingYaw-atan2f(-direction.y,-direction.x),2*syncFacingPi);}
    if(!hit){if(!syncTargetWithinActionRadius(*fresh,syncTargetViewDistance.load()))return {};
        if(!fresh->retainedTarget){fresh->retainedAim=value->aim;fresh->retainedFocus=value->jointFocus;fresh->retainedSelectionKnown=value->selectionKnown;
            fresh->retainedSelectionTolerance=value->selectionTolerance;fresh->retainedSelectionPose=value->facing;fresh->retainedChainsawRules=value->chainsawRules;}
        fresh->retainedTarget=true;
    }else fresh->retainedTarget=false;
    // A render refresh must not move old selector poses into a new situation.
    float rotationDelta=0;for(unsigned i=0;i<9;++i)rotationDelta=std::max(rotationDelta,fabsf(fresh->facing.axis[i]-value->facing.axis[i]));
    if(syncSelectionDistance(fresh->ranges.playerOrigin,value->ranges.playerOrigin)>.05||syncSelectionDistance(fresh->facing.origin,value->facing.origin)>.01||rotationDelta>.001f)fresh->selectionKnown=false;
    {syncShotBounds(fresh->spheres,fresh->shotBounds);fresh->aim=readSyncAimFrame(memory,base,value->entity,camera);}
    {fresh->playerFootprint=readSyncPlayerFootprint(memory,player,value->level.map);fresh->jointFocus=readSyncJointFocus(memory,base,player,value->entity);
        readSyncSphereFocusGroups(memory,base,value->entity,fresh->spheres);}
    {fresh->exclusiveGuide=fresh->action==1?std::make_shared<const SyncExclusiveGuide>(syncBuildExclusiveGuide(*fresh)):nullptr;
        fresh->referenceGuide=std::make_shared<const SyncExclusiveGuide>(syncBuildReferenceGuide(*fresh));
        fresh->sectorMesh=std::make_shared<const SyncSectorMesh>(syncBuildSectorMesh(*fresh->referenceGuide,fresh->shotBounds));
        if(fresh->playerFootprint.known){const auto origin=syncBuildReferenceGuide(*fresh,false);
            fresh->originSectorMesh=std::make_shared<const SyncSectorMesh>(syncBuildSectorMesh(origin,fresh->shotBounds));
            fresh->clearanceBandMesh=std::make_shared<const SyncSectorMesh>(syncBuildClearanceBand(origin,*fresh->referenceGuide,fresh->shotBounds));}}
    {const std::string selected=syncSelectedAnimation(*fresh);
        syncRefreshAimMesh(camera,*fresh);
        fresh->sectorTone=syncTargetSectorTone(*fresh,*fresh->referenceGuide,*fresh->sectorMesh,selected);
        fresh->sectorToneKnown=true;}
    return fresh;
}
static void appendSyncTargetGeometry(const GameCamera& camera,const SyncTargetFrame& value,std::vector<Vertex>& output,bool lines) {
    const auto guide=value.referenceGuide?value.referenceGuide:std::make_shared<const SyncExclusiveGuide>(syncBuildReferenceGuide(value));
    const std::string selected=syncSelectedAnimation(value);
    const auto* aim=syncSelectedAim(value,selected);SyncAimRequirement visualAim;
    if(syncBodyAimKind(aim,value.spheres)==SyncBodyAimKind::AllCurrentGroups){visualAim=*aim;visualAim.damageGroups.clear();aim=&visualAim;}
    appendSyncAimBox(camera,value.shotBounds,value.aim,aim,output,lines);
    const auto aimParts=value.aimPartMesh&&value.aimPartSignature==syncAimMeshSignature(camera,value,selected)?value.aimPartMesh:std::make_shared<const SyncAimPartMesh>(syncBuildAimPartMesh(camera,value.shotBounds,value.aim,syncSelectedAim(value,selected),value.spheres));
    appendSyncAimPartMesh(*aimParts,output,lines);
    const auto mesh=value.sectorMesh?value.sectorMesh:std::make_shared<const SyncSectorMesh>(syncBuildSectorMesh(*guide,value.shotBounds));
    const auto tone=value.sectorToneKnown?value.sectorTone:syncTargetSectorTone(value,*guide,*mesh,selected);
    auto originMesh=value.originSectorMesh,bandMesh=value.clearanceBandMesh;
    if(!originMesh&&!bandMesh&&value.playerFootprint.known&&guide->known){const auto origin=syncBuildReferenceGuide(value,false);
        originMesh=std::make_shared<const SyncSectorMesh>(syncBuildSectorMesh(origin,value.shotBounds));
        bandMesh=std::make_shared<const SyncSectorMesh>(syncBuildClearanceBand(origin,*guide,value.shotBounds));}
    if(originMesh&&originMesh->known&&bandMesh&&bandMesh->known){
        if(lines){appendSyncSectorMesh(*originMesh,output,true,SyncSectorTone::Reference);return;}
        appendSyncSectorMesh(*bandMesh,output,false,SyncSectorTone::Waiting);
        appendSyncSectorFaces(*originMesh,output,bandMesh->fill.empty()?tone:SyncSectorTone::Waiting,false);
        appendSyncSectorFaces(*mesh,output,tone,true);return;}
    appendSyncSectorMesh(*mesh,output,lines,tone);
}
