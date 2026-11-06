#pragma once

struct CapturedSyncDeclaration {
    std::string declaration,state,groups;int action=0;uint32_t quadrants=0;
    float priority=0;unsigned char position=0,lunge=0;float limits[6]{};
};
struct CapturedSyncMatrix {std::string blueprint,state;float player[12]{},target[12]{};};
static void checkCapturedZombieAndSoldier() {
    std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-zombie-soldier-authoring-11.tsv");
    requireTarget(bool(input),"actual Zombie/Soldier declaration and attachment capture exists");
    std::vector<CapturedSyncDeclaration> declarations;std::vector<CapturedSyncMatrix> matrices;std::string line;
    while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);char kind;fields>>kind;
        if(kind=='D'){CapturedSyncDeclaration d;unsigned position,lunge;fields>>d.declaration>>d.state>>d.action>>d.quadrants>>d.groups>>d.priority>>position;
            for(auto& f:d.limits)fields>>f;fields>>lunge;d.position=static_cast<unsigned char>(position);d.lunge=static_cast<unsigned char>(lunge);declarations.push_back(d);}
        else if(kind=='M'){CapturedSyncMatrix m;fields>>m.blueprint>>m.state;for(auto& f:m.player)fields>>f;for(auto& f:m.target)fields>>f;matrices.push_back(m);}
        else requireTarget(false,"known captured authoring row");requireTarget(!fields.fail(),"captured authoring parses");}
    auto makeFrame=[&](const std::string& blueprint,const std::string& family){
        SyncTargetFrame frame;frame.regionState="ready";frame.ranges.known=true;frame.ranges.gloryKillTarget=10;frame.ranges.chainsawTarget=10;frame.ranges.lunge=10;frame.ranges.ignoreQuickLunge=true;
        frame.facing.axis[0]=frame.facing.axis[4]=frame.facing.axis[8]=1;frame.aim.known=true;frame.aim.quadrant=4;frame.jointFocus={true,"right_arm"};
        frame.playerFootprint={true,{-.25f,-.25f,0},{.25f,.25f,0}};const float bounds[]{-.5f,-.5f,0,.5f,.5f,2};memcpy(frame.shotBounds,bounds,24);
        frame.spheres={{{0,0,1},.2f,"",1,true,"right_arm"},{{0,0,1.5f},.2f,"",2,true,"left_arm"},{{0,0,1.8f},.2f,"",3,true,"head"}};
        for(const auto& d:declarations)if(d.declaration.find("/"+family+"/")!=std::string::npos||(family=="zombie_tier1"&&d.declaration=="syncdeath/playervsai/zombie_tier3/front_rightarm")){
            SyncAimRequirement aim;aim.quadrants=d.quadrants;if(d.groups!="-"){std::istringstream groups(d.groups);std::string group;while(std::getline(groups,group,'|'))aim.damageGroups.push_back(group);}
            const auto audit=syncAuditEnemyRule(d.declaration,d.state,d.action,aim);requireTarget(audit.matched,"actual declaration/state/groups match reviewed native authoring");
            frame.animationRules.push_back({d.state,d.action,aim,audit});
            if(d.action){frame.hasChainsaw=true;frame.chainsawAnimations.push_back(d.state);continue;}
            const auto matrix=std::find_if(matrices.begin(),matrices.end(),[&](const auto& m){return m.blueprint==blueprint&&m.state==d.state;});requireTarget(matrix!=matrices.end(),"actual GK state has a captured frame-zero matrix");
            std::vector<unsigned char> decl(0x398),node(0xb0),joints(0x3030);targetPut(decl,0x1d8,d.position);targetPut(decl,0x250,d.lunge);
            memcpy(decl.data()+0x254,d.limits,sizeof(d.limits));targetPut(decl,0x310,d.quadrants);
            const auto strings=d.groups=="-"?std::vector<std::string>{}:aim.damageGroups;std::vector<uintptr_t> ptrs;for(const auto& g:strings)ptrs.push_back(reinterpret_cast<uintptr_t>(g.c_str()));
            targetPut(decl,0x2a0,SyncPreviewList{reinterpret_cast<uintptr_t>(ptrs.data()),int(ptrs.size()),int(ptrs.size()),0});
            memcpy(joints.data(),matrix->player,48);memcpy(joints.data()+48,matrix->target,48);joints[0x301c]=1;
            SyncFacingRegion region;requireTarget(readSyncFacingRegion(WallMemory{},targetAddress(decl),targetAddress(node),targetAddress(joints),0,1,d.state,region),"production facing reader accepts actual matrices and ranges");
            region.audit=audit;region.priorityKnown=true;region.priority=d.priority;frame.regions.push_back(region);}
        requireTarget(syncNormalizeFacingRegions(frame.regions),"same-state native branches retain compatible actual aim requirements");syncMergeAnimationRules(frame.animationRules);return frame;
    };
    for(const char* variant:{"zombie_tier1","zombie_tier1_hell","zombie_tier1_scientist","zombie_worshipper"}){
        auto frame=makeFrame(std::string("syncmelee/")+variant,"zombie_tier1");frame.selectedAnimation="kill_front_rightarm";
        requireTarget(frame.regions.size()==15,"T1 preserves both native right-arm interactions instead of deleting or disabling all zones");
        const auto* aim=syncSelectedAim(frame,frame.selectedAnimation);requireTarget(aim&&aim->known,"borrowed T3 declaration with identical native aim does not invalidate T1 arm highlighting");
        const auto guide=syncBuildReferenceGuide(frame,false);requireTarget(guide.known&&guide.branches.size()==2&&!guide.pieces.empty(),"all T1 model templates build a union of distinct right-arm branches");
        size_t samples=0;for(float x=-8;x<8;x+=.2f)for(float y=-8;y<8;y+=.2f)if(syncExclusiveContains(guide,{x,y,0})){
            ++samples;bool expected=false;for(const auto& branch:guide.branches)expected|=syncExclusiveContains(branch,{x,y,0});requireTarget(expected,"T1 union cannot fill a point rejected by both original branches");}
        requireTarget(samples>50,"the repaired T1 standing union has a useful area");
        const auto mesh=syncBuildSectorMesh(guide,frame.shotBounds);requireTarget(mesh.known&&!mesh.fill.empty(),"T1 native branch union produces filled sectors");
        frame.action=1;frame.selectionKnown=true;frame.selectionTolerance=.381f;frame.selectedAnimation="cut_back";
        for(const char* state:{"cut_front_01","cut_back"}){const auto found=std::find_if(frame.animationRules.begin(),frame.animationRules.end(),[&](const auto& r){return r.action==1&&r.name==state;});
            requireTarget(found!=frame.animationRules.end(),"T1 front and back chainsaw authoring exists");SyncChainsawRule rule{state,found->aim,true,true,1.25f,state==std::string("cut_back")?Vec3{-2,0,0}:Vec3{2,0,0},found->audit};frame.chainsawRules.push_back(rule);}
        const auto back=syncBuildExclusiveGuide(frame);requireTarget(back.known&&syncExclusiveContains(back,{-5,0,0})&&!syncExclusiveContains(back,{0,0,0}),"inactive front alternative does not hide T1 back chainsaw, while RNG overlap remains excluded");
        frame.selectedAnimation="cut_front_02";requireTarget(!syncBuildExclusiveGuide(frame).known,"inactive front clip never borrows the active alternative's selector pose");
        frame.selectedAnimation="cut_back";frame.chainsawRules[0].audit.matched=false;requireTarget(!syncBuildExclusiveGuide(frame).known,"an unaudited independent missing competitor still prevents false chainsaw zones");
    }
    auto soldier=makeFrame("syncmelee/soldier_blaster","soldier_blaster");soldier.selectedAnimation="kill_front_rightarm";
    const auto lowerArm=syncBuildReferenceGuide(soldier,false);requireTarget(lowerArm.known&&!lowerArm.pieces.empty(),"actual soldier right arm has a standing area with lower-screen aim");
    auto automatic=soldier;automatic.selectedAnimation.clear();automatic.ranges.playerOrigin={3,0,0};SyncAutomaticSelection resolver;resolver.update(automatic);
    requireTarget(automatic.automaticAnimation=="kill_front_rightarm","automatic mode displays the Soldier right-arm zone when entering it with lower-arm native focus");
    soldier.aim.quadrant=1;const auto upperArm=syncBuildReferenceGuide(soldier,false);requireTarget(upperArm.known&&syncExclusiveContains(lowerArm,{3,0,0})&&!syncExclusiveContains(upperArm,{3,0,0}),"generic equal-priority front-upper overlap is excluded when aiming at the upper right arm");
    soldier.selectedAnimation="kill_front_leftarm";soldier.jointFocus.group="left_arm";
    for(auto& r:soldier.regions)if(r.name==soldier.selectedAnimation)r.priority=1.25f; // Observed before four actual left-arm clips.
    requireTarget(!syncBuildReferenceGuide(soldier,false).pieces.empty(),"recorded eligible AI priority bonus preserves soldier left-arm standing positions");
    automatic=soldier;automatic.selectedAnimation.clear();automatic.ranges.playerOrigin={3,0,0};resolver.update(automatic);
    requireTarget(automatic.automaticAnimation=="kill_front_leftarm","automatic mode displays the eligible Soldier left-arm zone instead of its lower-priority generic fallback");
    printf("PASS: captured T1 variants, same-animation native branches, inactive Chainsaw alternatives and Soldier arm aim/priority regressions.\n");
}
