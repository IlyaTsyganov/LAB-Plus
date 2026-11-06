#pragma once

static SyncTargetFrame displayFixtureFor(const SyncAuditedEnemyRule& authored,const SyncAimRequirement& aim) {
    SyncTargetFrame frame;frame.level={0x10000,0x20000,"fixture/display"};frame.entity={0x30000,0x10000,0x40000,1,1,"idAI2"};
    frame.action=authored.action;frame.selectedAnimation=authored.animation;
    frame.animationRules.push_back({authored.animation,authored.action,aim,syncAuditEnemyRule(authored.declaration,authored.animation,authored.action,aim)});
    if(frame.action==1)frame.chainsawAnimations.push_back(authored.animation);
    // Synthetic current-pose model: this verifies presentation of real authored
    // rules, not anatomical accuracy or execution of all enemy models in-game.
    std::vector<std::string> groups{"head","chest","gut","left_upper_arm","left_lower_arm","right_upper_arm","right_lower_arm","left_upper_leg","left_lower_leg","right_upper_leg","right_lower_leg"};
    if(std::string(authored.family)=="soldier_blaster"||std::string(authored.family)=="zombie_tier1"||std::string(authored.family)=="zombie_tier3")
        groups={"head","chest","gut","left_arm","right_arm","left_leg","right_leg","left_foot","right_foot"};
    if(std::string(authored.family)=="carcass")groups={"head","chest","gut","left_arm","right_arm","left_arm_mech","right_arm_mech"};
    for(const auto& group:groups){SyncHitSphere s;s.focusKnown=true;s.focusGroup=group;s.radius=.1f;s.center={0,4,0};
        s.center.x=group.find("left")!=std::string::npos?-.65f:group.find("right")!=std::string::npos?.65f:0;
        s.center.z=group=="head"?1.15f:group=="chest"?.6f:group=="gut"?.15f:group.find("foot")!=std::string::npos?-.9f:
            group.find("leg")!=std::string::npos?(group.find("lower")!=std::string::npos?-.9f:-.5f):group.find("lower")!=std::string::npos?.1f:.55f;
        frame.spheres.push_back(std::move(s));}
    const float bounds[]{-1,3,-1.1f,1,5,1.35f};memcpy(frame.shotBounds,bounds,24);frame.aim.known=true;frame.aim.quadrant=3;memcpy(frame.aim.bounds,bounds,24);
    return frame;
}
static void checkEveryEnemyDisplay() {
    GameCamera camera{};camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    std::ifstream fixture("Native/CollisionViewer/tests/fixtures/sync-enemy-rules-11.tsv");requireTarget(bool(fixture),"per-enemy display audit fixture exists");
    size_t count=0;std::unordered_set<std::string> families,partFamilies;
    std::string line;while(std::getline(fixture,line)){if(line.empty()||line[0]=='#')continue;
        std::istringstream fields(line);std::string family,state,action,groups;SyncAimRequirement aim;fields>>family>>state>>action>>aim.quadrants>>groups;
        std::string declaration,ignored;for(unsigned i=0;i<11;++i)fields>>ignored;fields>>declaration;requireTarget(!fields.fail(),"reviewed display fixture includes the full source declaration");
        if(groups!="-"){std::istringstream names(groups);std::string name;while(std::getline(names,name,'|'))aim.damageGroups.push_back(name);}
        const int requested=action=="GK"?0:1;const SyncAuditedEnemyRule* authored=nullptr;
        for(const auto& rule:syncAuditedEnemyRules)if(family==rule.family&&state==rule.animation&&requested==rule.action&&declaration==rule.declaration){requireTarget(!authored,"one reviewed source per declaration and state in the saved inventory");authored=&rule;}
        requireTarget(authored!=nullptr,"every report entry has a native reviewed display profile");
        auto frame=displayFixtureFor(*authored,aim);families.insert(family);++count;
        requireTarget(frame.animationRules[0].audit.matched,"real authored quadrant and group requirements match their reviewed source declaration");
        const auto choices=syncAnimationChoices(frame);requireTarget(choices==std::vector<std::string>{state},"available animation remains selectable when its standing transform has no supported diagram");
        syncRefreshAimMesh(camera,frame);std::vector<Vertex> fill,outline;appendSyncTargetGeometry(camera,frame,fill,false);appendSyncTargetGeometry(camera,frame,outline,true);
        const auto body=syncBodyAimKind(&aim,frame.spheres);const auto hints=syncAnimationDisplayHints(frame);
        requireTarget(hints.size()==1&&hints[0].animation==state&&hints[0].enemy==authored->label&&!hints[0].aim.empty(),"current target and selected action receive their own authored requirement hint");
        if(aim.damageGroups.empty()||body==SyncBodyAimKind::AllCurrentGroups){
            for(const auto& v:fill)requireTarget(v.color[0]<.5f,"quadrant-only or whole-body rules cannot produce yellow limb patches");
            requireTarget(!frame.aimPartMesh->required,"orientation-only or whole-body rules request no anatomical mesh");
            if(syncAimUnrestricted(aim.quadrants))requireTarget(fill.empty()&&outline.size()==24,"unrestricted variants keep only the square enemy box");
            else requireTarget(!fill.empty()&&outline.size()==28,"restricted variants preserve the actual physics-centre quadrant tiles");
        } else {
            if(requested==0)partFamilies.insert(family);
            requireTarget(frame.aimPartMesh->required&&frame.aimPartMesh->known,"specific native group requirements produce an explicit anatomical presentation");
            for(const auto& v:fill)requireTarget(v.color[0]>.9f,"body-guided variants show only the group/quadrant intersection, without a coarse blue quadrant underneath");
            requireTarget(!fill.empty(),"synthetic current pose offers at least one visible required group for every reviewed body-guided variant");
        }
        if(family=="hellknight"&&state=="kill_front_head")requireTarget(hints[0].aim.find("No specific body part")!=std::string::npos&&hints[0].aim.find("top left")!=std::string::npos,"Hell Knight front_head is shown as upper quadrants instead of an invented head target");
        if(family=="mancubus_fire"&&state=="kill_front_head")requireTarget(hints[0].aim.find("Aim at head")!=std::string::npos,"Mancubus head GK retains real head guidance");
        if(family=="zombie_tier3"&&state=="kill_back_head")requireTarget(!frame.aimPartMesh->required,"Zombie tier3 back_head remains quadrant-only");
        if(requested==1&&family=="hellknight")requireTarget(body==SyncBodyAimKind::AllCurrentGroups&&fill.empty(),"both Hell Knight cuts show no artificial limb or front/back aiming patch");
        auto mismatch=aim;mismatch.damageGroups.push_back("invented_group");requireTarget(!syncAuditEnemyRule(authored->declaration,state,requested,mismatch).matched,"changed native group authoring cannot inherit the reviewed presentation silently");
    }
    requireTarget(count==224&&families.size()==25&&partFamilies.size()==8,"all 224 states across 25 report families exercise their actual rendering path; eight families use anatomical GK guidance");
    printf("PASS: per-enemy presentation for 224 authored states / 25 families with synthetic current poses.\n");
}
static void checkDisplaySwitchesAndUnknowns() {
    GameCamera camera{};camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    const SyncAuditedEnemyRule* imp=nullptr;const SyncAuditedEnemyRule* knight=nullptr;
    for(const auto& r:syncAuditedEnemyRules){if(std::string(r.family)=="imp"&&std::string(r.animation)=="kill_front_head")imp=&r;
        if(std::string(r.family)=="hellknight"&&std::string(r.animation)=="kill_front_head")knight=&r;}
    requireTarget(imp&&knight,"same animation-name counterexample profiles exist");SyncAimRequirement head;head.quadrants=3;head.damageGroups={"head"};
    auto old=displayFixtureFor(*imp,head);syncRefreshAimMesh(camera,old);requireTarget(!old.aimPartMesh->fill.empty(),"initial head patch is available");
    SyncAimRequirement upper;upper.quadrants=3;auto next=displayFixtureFor(*knight,upper);
    next.aimPartMesh=old.aimPartMesh;next.aimPartSignature=old.aimPartSignature;next.aimPartAnimation=old.aimPartAnimation;next.aimPartAction=old.aimPartAction;next.aimPartEntity=old.aimPartEntity;
    std::vector<Vertex> stale,expected;appendSyncTargetGeometry(camera,next,stale,false);next.aimPartMesh.reset();appendSyncTargetGeometry(camera,next,expected,false);
    requireTarget(stale.size()==expected.size()&&!memcmp(stale.data(),expected.data(),stale.size()*sizeof(Vertex)),"switching enemy with the SAME animation key cannot reuse another enemy's anatomical patch");
    auto changed=old;changed.spheres[0].center.x+=.3f;
    requireTarget(syncAimMeshSignature(camera,changed,changed.selectedAnimation)!=old.aimPartSignature,"animated body-pose changes invalidate the mesh cache");
    changed=old;changed.entity.generation++;requireTarget(syncAimMeshSignature(camera,changed,changed.selectedAnimation)!=old.aimPartSignature,"slot reuse invalidates a target's patch");
    auto cameraChanged=camera;cameraChanged.position[0]=.1f;cameraChanged.matrix[3]=-.1f;
    requireTarget(syncAimMeshSignature(cameraChanged,old,old.selectedAnimation)!=old.aimPartSignature,"moving the viewpoint invalidates its screen-facing mask");
    SyncAutomaticSelection automatic;automatic.update(old);requireTarget(!old.aimPartMesh,"a new automatic/manual presentation cannot retain a pre-selection mesh");
    auto unknown=next;unknown.animationRules[0].audit=syncAuditEnemyRule("syncdeath/playervsai/unknown/front_head",unknown.selectedAnimation,0,upper);unknown.animationRules[0].aim.known=false;
    requireTarget(syncAnimationAimText(unknown,unknown.selectedAnimation)=="Aim rules unavailable for this variant","unknown enemy resources have an explicit unavailable hint");
    std::vector<Vertex> noGuess;appendSyncTargetGeometry(camera,unknown,noGuess,false);requireTarget(noGuess.empty(),"unreviewed rules never fall back to another family's head or quadrant drawing");
    auto conflicts=next.animationRules;auto alternate=conflicts[0];alternate.aim.damageGroups={"head"};conflicts.push_back(alternate);syncMergeAnimationRules(conflicts);
    requireTarget(conflicts.size()==1&&!conflicts[0].aim.known,"duplicate animation states with incompatible aim requirements stay unknown");
    SyncFacingRegion a,b;a.name=b.name="same_state";b.maxXY=5;
    requireTarget(!syncSameFacingRule(a,b),"duplicate state names with different distance limits cannot silently collapse into one spatial prediction");
    next.rulesCoherent=false;requireTarget(!syncBuildReferenceGuide(next).known,"an incomplete native authoring pool cannot claim a complete exclusive area");
}
static void checkRecordedHellKnightDisplay() {
    GameCamera camera{};SyncTargetFrame frame;frame.aim.known=true;
    std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-display-hellknight-11.tsv");requireTarget(bool(input),"verified current Hell Knight pose fixture exists");
    std::string line;while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);char kind;fields>>kind;
        if(kind=='C'){for(auto& f:camera.matrix)fields>>f;for(auto& f:camera.position)fields>>f;}
        else if(kind=='B')for(auto& f:frame.shotBounds)fields>>f;
        else if(kind=='Q'){fields>>frame.aim.known;for(auto& f:frame.aim.bounds)fields>>f;}
        else if(kind=='S'){SyncHitSphere s;fields>>s.center.x>>s.center.y>>s.center.z>>s.radius>>s.jointIndex>>s.focusKnown>>s.focusGroup>>s.group;if(s.focusGroup=="-")s.focusGroup.clear();frame.spheres.push_back(std::move(s));}
        else requireTarget(false,"known actual Hell Knight fixture record");requireTarget(!fields.fail(),"actual Hell Knight fixture parses");}
    requireTarget(frame.spheres.size()==35,"actual Hell Knight pose contains the 35 native shot spheres observed in the production reader");
    std::ofstream vertices("build/sync-display-v15-hellknight-mesh.tsv");vertices.precision(9);vertices<<"C";for(float f:camera.matrix)vertices<<'\t'<<f;vertices<<'\n';
    unsigned cases=0;
    for(const auto& r:syncAuditedEnemyRules)if(std::string(r.family)=="hellknight"&&(std::string(r.animation)=="kill_front_head"||std::string(r.animation)=="kill_front_lower"||r.action==1)){
        ++cases;frame.action=r.action;frame.selectedAnimation=r.animation;SyncAimRequirement aim;aim.quadrants=r.quadrants;
        std::istringstream groups(r.groups);std::string group;while(std::getline(groups,group,'|'))if(!group.empty())aim.damageGroups.push_back(group);
        frame.animationRules={{r.animation,r.action,aim,syncAuditEnemyRule(r.declaration,r.animation,r.action,aim)}};frame.chainsawAnimations={r.animation};syncRefreshAimMesh(camera,frame);
        std::vector<Vertex> fill,wire;appendSyncTargetGeometry(camera,frame,fill,false);appendSyncTargetGeometry(camera,frame,wire,true);
        requireTarget(!frame.aimPartMesh->required,"verified Hell Knight variants never create head/arm/leg patches");
        if(r.action==1)requireTarget(fill.empty()&&wire.size()==24,"actual Hell Knight Chainsaw keeps only the target box without an invented anatomical or directional tile");
        else {requireTarget(!fill.empty()&&wire.size()==28,"actual Hell Knight front_head/front_lower use real upper/lower physics-centre tiles");
            const auto* b=frame.aim.bounds;double cx,cy,w;requireTarget(syncAimProject(camera,{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},cx,cy,w),"actual Hell Knight native aim centre projects");
            for(const auto& v:fill){double x=0,y=0,z=0;requireTarget(v.color[0]<.5f&&syncAimProject(camera,v.position,x,y,z),"real Hell Knight quadrants are drawn as quadrant tiles");
                requireTarget(r.quadrants==3?y>=cy-.00001:y<=cy+.00001,"actual native physics centre determines upper/lower, independently of the shot-box centre");}}
        for(const auto& v:fill)vertices<<"F\t"<<r.animation<<'\t'<<v.position.x<<'\t'<<v.position.y<<'\t'<<v.position.z<<'\n';
        for(const auto& v:wire)vertices<<"L\t"<<r.animation<<'\t'<<v.position.x<<'\t'<<v.position.y<<'\t'<<v.position.z<<'\n';
    }
    requireTarget(cases==4,"actual Hell Knight front upper/lower and both cut variants are checked separately");
}
static void checkRecordedChainsawClearanceBand() {
    std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-hellknight-executions-11.tsv");requireTarget(bool(input),"recorded Hell Knight execution positions exist");
    std::string line;unsigned cases=0,unique=0,random=0;std::ofstream meshFile("build/sync-display-v15-clearance-mesh.tsv");meshFile.precision(9);
    auto containsCap=[](const SyncSectorMesh& mesh,Vec3 point,float z){for(size_t i=0;i<mesh.fill.size();i+=3){const auto a=mesh.fill[i],b=mesh.fill[i+1],c=mesh.fill[i+2];if(a.z!=z||b.z!=z||c.z!=z)continue;
            if(fabs(double(b.x-a.x)*(c.y-a.y)-double(b.y-a.y)*(c.x-a.x))<1e-9)continue;
            auto side=[&](Vec3 u,Vec3 v){return double(v.x-u.x)*(point.y-u.y)-double(v.y-u.y)*(point.x-u.x);};
            const double x=side(a,b),y=side(b,c),w=side(c,a);if((x>=-1e-6&&y>=-1e-6&&w>=-1e-6)||(x<=1e-6&&y<=1e-6&&w<=1e-6))return true;}return false;};
    while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);unsigned id=0,pool=0;double at=0;std::string clip;SyncTargetFrame frame;Vec3 front{},back{};
        fields>>id>>at>>clip;auto vec=[&](Vec3& v){fields>>v.x>>v.y>>v.z;};vec(frame.ranges.playerOrigin);vec(frame.facing.origin);vec(frame.ranges.eyeOffset);vec(front);vec(back);fields>>frame.selectionTolerance>>pool;requireTarget(!fields.fail(),"recorded execution selector fields parse");++cases;
        frame.action=1;frame.selectedAnimation="cut_front_01";frame.hasChainsaw=true;frame.selectionKnown=true;frame.ranges.known=true;frame.ranges.ignoreQuickLunge=true;frame.ranges.chainsawTarget=10;frame.ranges.gloryKillTarget=6.5f;frame.ranges.lunge=6.5f;
        frame.facing.axis[0]=frame.facing.axis[4]=frame.facing.axis[8]=1;frame.playerFootprint={true,{-.395f,-.395f,0},{.395f,.395f,0}};
        frame.chainsawAnimations={"cut_front_01","cut_back"};frame.chainsawRules={{"cut_front_01",{},true,true,1.25f,front},{"cut_back",{},true,true,1.25f,back}};
        const auto origin=syncBuildReferenceGuide(frame,false),body=syncBuildReferenceGuide(frame,true);
        const float bounds[]{frame.facing.origin.x-.6f,frame.facing.origin.y-.6f,frame.facing.origin.z,frame.facing.origin.x+.6f,frame.facing.origin.y+.6f,frame.facing.origin.z+2.5f};
        const auto bandStarted=std::chrono::steady_clock::now();const auto band=syncBuildClearanceBand(origin,body,bounds);requireTarget(origin.known&&body.known&&band.known,"recorded selector snapshots produce bounded origin/body/border meshes");
        if(id==0)printf("Collider-clearance band %.3f ms (offline CPU, not FPS).\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-bandStarted).count());
        const bool onBand=containsCap(band,frame.ranges.playerOrigin,bounds[5]);
        if(pool==1){++unique;requireTarget(syncExclusiveContains(origin,frame.ranges.playerOrigin)&&!syncExclusiveContains(body,frame.ranges.playerOrigin),"recorded sole-front cases reproduce the missing area caused only by the preview's full-body buffer");
            requireTarget(onBand,"recorded sole-front cases now lie in the amber collider-clearance band");}
        else {++random;requireTarget(!syncExclusiveContains(origin,frame.ranges.playerOrigin)&&!onBand,"recorded random-choice positions remain completely unfilled");}
        for(size_t i=0;i<band.fill.size();i+=3){const auto a=band.fill[i],b=band.fill[i+1],c=band.fill[i+2];if(a.z!=bounds[5]||b.z!=bounds[5]||c.z!=bounds[5])continue;
            const Vec3 p{(a.x+b.x+c.x)/3,(a.y+b.y+c.y)/3,frame.ranges.playerOrigin.z};
            requireTarget(syncExclusiveNativeContains(origin,p),"every amber band triangle remains outside the true native random band");}
        if(pool==1&&unique==1){for(const auto& p:origin.pieces)for(size_t i=0;i<p.size();++i)meshFile<<"L\t"<<p[i].x<<'\t'<<p[i].y<<'\t'<<p[(i+1)%p.size()].x<<'\t'<<p[(i+1)%p.size()].y<<'\n';
            memcpy(frame.shotBounds,bounds,24);frame.referenceGuide=std::make_shared<const SyncExclusiveGuide>(body);frame.sectorMesh=std::make_shared<const SyncSectorMesh>(syncBuildSectorMesh(body,bounds));
            frame.originSectorMesh=std::make_shared<const SyncSectorMesh>(syncBuildSectorMesh(origin,bounds));frame.clearanceBandMesh=std::make_shared<const SyncSectorMesh>(band);
            GameCamera camera{};std::vector<Vertex> lines;appendSyncTargetGeometry(camera,frame,lines,true);
            requireTarget(lines.size()==frame.originSectorMesh->outline.size()+24,"only the outer origin contour is outlined; the clearance ring cannot duplicate arc uprights or add internal fan lines");
            for(const auto* mesh:{&band})for(size_t i=0;i<mesh->fill.size();i+=3)if(mesh->fill[i].z==bounds[5]&&mesh->fill[i+1].z==bounds[5]&&mesh->fill[i+2].z==bounds[5])for(unsigned j=0;j<3;++j)meshFile<<"F\t"<<mesh->fill[i+j].x<<'\t'<<mesh->fill[i+j].y<<'\n';
            meshFile<<"P\t"<<frame.ranges.playerOrigin.x<<'\t'<<frame.ranges.playerOrigin.y<<'\n';}
    }
    requireTarget(cases==6&&unique==3&&random==3,"three observed sole-front cases gain a clearance band; three genuinely ambiguous front cases stay unpainted");
}
