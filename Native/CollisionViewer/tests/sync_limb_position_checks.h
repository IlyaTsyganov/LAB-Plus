#pragma once

static SyncTargetFrame loadRightArmPose(GameCamera& camera) {
    SyncTargetFrame frame;frame.selectedAnimation="kill_front_rightarm";frame.regionState="ready";frame.ranges.known=true;
    std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-zombie-rightarm-11.tsv");requireTarget(bool(input),"native Zombie tier3 right-arm fixture exists");
    std::string line;while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);char kind;fields>>kind;
        auto vec=[&](Vec3& v){fields>>v.x>>v.y>>v.z;};
        if(kind=='P')vec(frame.ranges.playerOrigin);else if(kind=='E')vec(frame.ranges.eyeOffset);else if(kind=='A')vec(frame.ranges.aimOrigin);else if(kind=='V')vec(frame.ranges.viewTargetOffset);
        else if(kind=='M')fields>>frame.ranges.modelViewOffset>>frame.incomingYaw>>frame.ranges.gloryKillTarget>>frame.ranges.chainsawTarget>>frame.ranges.lunge>>frame.ranges.modifier>>frame.ranges.ignoreQuickLunge;
        else if(kind=='T'){vec(frame.facing.origin);for(auto& f:frame.facing.axis)fields>>f;}
        else if(kind=='B'){for(auto& f:frame.shotBounds)fields>>f;fields>>frame.playerFootprint.minimum.x>>frame.playerFootprint.minimum.y>>frame.playerFootprint.maximum.x>>frame.playerFootprint.maximum.y>>frame.playerFootprint.known;}
        else if(kind=='Q')fields>>frame.aim.known>>frame.aim.quadrant>>frame.jointFocus.known>>frame.jointFocus.group;
        else if(kind=='R'){SyncFacingRegion r;size_t n=0;fields>>r.name;vec(r.incoming);vec(r.start);fields>>r.halfYaw>>r.minXY>>r.maxXY>>r.minZ>>r.maxZ>>r.checkLunge>>r.flatLunge>>r.aim.quadrants>>r.aim.known>>n;
            requireTarget(n<=32,"bounded required group list");while(n--){std::string name;fields>>name;r.aim.damageGroups.push_back(name);}frame.regions.push_back(std::move(r));}
        else if(kind=='D'){std::string name;bool known=false;float priority=0;fields>>name>>known>>priority;for(auto& r:frame.regions)if(r.name==name){r.priorityKnown=known;r.priority=priority;}}
        else if(kind=='C'){for(auto& f:camera.matrix)fields>>f;for(auto& f:camera.position)fields>>f;}
        else if(kind=='N'){fields>>frame.aim.known;for(auto& f:frame.aim.bounds)fields>>f;}
        else if(kind=='S'){SyncHitSphere s;vec(s.center);fields>>s.radius>>s.jointIndex>>s.focusKnown>>s.focusGroup>>s.group;frame.spheres.push_back(std::move(s));}
        else requireTarget(false,"known right-arm fixture row");requireTarget(!fields.fail(),"native right-arm fixture parses");}
    return frame;
}
static void checkLimbStandingReference() {
    GameCamera camera{};auto frame=loadRightArmPose(camera);
    requireTarget(frame.regions.size()==13&&!frame.spheres.empty()&&frame.jointFocus.known&&frame.jointFocus.group=="gut","fixture reproduces selected right arm while current native focus is the gut");
    const auto actual=syncBuildExclusiveGuide(frame),planned=syncBuildReferenceGuide(frame);
    requireTarget(actual.known&&actual.pieces.empty(),"current native focus still rejects a right-arm activation prediction");
    requireTarget(planned.known&&planned.requiredAimReference&&!planned.pieces.empty(),"the required-focus standing reference is visible before aiming at the arm");
    for(const char* focus:{"gut","head","left_arm","right_arm"})for(uint32_t quadrant:{1u,2u,4u,8u}){
        auto aim=frame;aim.jointFocus={true,focus};aim.aim.quadrant=quadrant;const auto same=syncBuildReferenceGuide(aim);
        requireTarget(same.known&&same.pieces.size()==planned.pieces.size(),"moving aim within the enemy does not erase or rotate the right-arm standing reference");
        for(size_t i=0;i<same.pieces.size();++i)requireTarget(same.pieces[i].size()==planned.pieces[i].size()&&!memcmp(same.pieces[i].data(),planned.pieces[i].data(),same.pieces[i].size()*sizeof(SyncExclusivePoint)),"limb standing reference remains enemy-anchored and independent of the current wrong focus");}
    auto unknown=frame;unknown.jointFocus={};requireTarget(!syncBuildReferenceGuide(unknown).pieces.empty(),"unknown native cached focus cannot erase an authored conditional standing guide");
    frame.candidatesReady=true;frame.gloryKills={frame.selectedAnimation};
    const auto feedback=syncGuideFeedback(frame,planned.selected,frame.selectedAnimation);
    requireTarget(!feedback.selectedOnlyObserved&&feedback.tone==SyncSectorTone::Waiting,"a conditional limb reference never confirms activation with the wrong native focus");
    const auto mesh=syncBuildSectorMesh(planned,frame.shotBounds);requireTarget(mesh.known&&!mesh.fill.empty()&&!mesh.outline.empty(),"the actual zombie parameters produce a filled standing volume as well as the arm aim patch");
    std::vector<Vertex> fill;appendSyncTargetGeometry(camera,frame,fill,false);bool limb=false,standing=false;
    for(const auto& v:fill){limb|=v.color[0]==1&&v.color[1]==.48f&&v.color[2]==.08f;standing|=v.position.z==frame.shotBounds[2]||v.position.z==frame.shotBounds[5];}
    requireTarget(limb&&standing,"arm aiming and standing geometry are rendered together for the verified live enemy");
    std::ofstream output("build/sync-limb-v16-zombie-mesh.tsv");output.precision(9);for(auto p:mesh.fill)output<<"F\t"<<p.x<<'\t'<<p.y<<'\t'<<p.z<<'\n';for(auto p:mesh.outline)output<<"L\t"<<p.x<<'\t'<<p.y<<'\t'<<p.z<<'\n';
}
static void checkNearFarAndAimScopedExclusions() {
    SyncTargetFrame frame;frame.regionState="ready";frame.selectedAnimation="far_right_arm";frame.facing.axis[0]=frame.facing.axis[4]=frame.facing.axis[8]=1;
    frame.ranges.known=true;frame.ranges.gloryKillTarget=12;frame.ranges.lunge=12;frame.ranges.playerOrigin={6,0,0};frame.aim.known=true;frame.aim.quadrant=1;
    frame.playerFootprint={true,{-.3f,-.3f,0},{.3f,.3f,0}};
    frame.spheres={{{0,4,1},.2f,"",1,true,"right_arm"},{{-.5f,4,.5f},.2f,"",2,true,"left_arm"},{{0,4,.5f},.2f,"",3,true,"chest"}};
    SyncFacingRegion distant;distant.name=frame.selectedAnimation;distant.incoming={1,0,0};distant.halfYaw=syncFacingPi/3;distant.maxXY=9;distant.maxZ=3;distant.aim.damageGroups={"right_arm"};distant.priorityKnown=true;distant.priority=2;
    auto close=distant;close.name="near_right_arm";close.maxXY=3;frame.regions={distant,close};
    auto origin=syncBuildReferenceGuide(frame,false),body=syncBuildReferenceGuide(frame,true);
    requireTarget(origin.known&&body.known&&!origin.pieces.empty()&&!syncExclusiveContains(origin,{1,0,0})&&syncExclusiveContains(origin,{6,0,0}),"a distant animation retains its outer region and removes the close competitor's full eligible range");
    requireTarget(!syncExclusiveContains(body,{3.1f,0,0})&&syncExclusiveContains(body,{6,0,0}),"the near exclusion expands by the real player footprint for the clearance core");
    const float bounds[]{-.5f,-.5f,0,.5f,.5f,2};const auto band=syncBuildClearanceBand(origin,body,bounds);requireTarget(band.known,"near/distant exclusions still allow the collider-clearance band");
    for(size_t i=0;i<band.fill.size();i+=3){const auto& a=band.fill[i];const auto& b=band.fill[i+1];const auto& c=band.fill[i+2];const Vec3 p{(a.x+b.x+c.x)/3,(a.y+b.y+c.y)/3,0};requireTarget(syncExclusiveNativeContains(origin,p),"amber clearance cannot paint back over the removed close-animation region");}
    frame.regions[1].aim.damageGroups={"left_arm"};auto separate=syncBuildReferenceGuide(frame,false);
    requireTarget(separate.known&&syncExclusiveContains(separate,{1,0,0})&&syncExclusiveContains(separate,{6,0,0}),"identical overlapping positions may remain when the other animation needs a different body part");
    frame.regions[1].maxXY=distant.maxXY;separate=syncBuildReferenceGuide(frame,false);
    requireTarget(syncExclusiveContains(separate,{6,0,0}),"exactly identical positional regions with disjoint limb focus are not mutually subtracted");
    frame.regions[1]=close;frame.regions[1].priority=0;auto lower=syncBuildReferenceGuide(frame,false);
    requireTarget(syncExclusiveContains(lower,{1,0,0}),"a verified lower-priority fallback cannot erase the arm's higher-priority region");
    frame.regions[1].priorityKnown=false;auto unknown=syncBuildReferenceGuide(frame,false);requireTarget(!syncExclusiveContains(unknown,{1,0,0}),"unknown priority remains a conservative potential competitor");
    frame.regions[1]=close;frame.regions[1].start={2,0,0};auto offset=syncBuildReferenceGuide(frame,false);
    requireTarget(!syncExclusiveContains(offset,{4,0,0})&&syncExclusiveContains(offset,{7,0,0}),"near-range holes follow the authored start pose rather than a guessed enemy-centred radius");
    requireTarget(!syncAimScopesOverlap(SyncAimRequirement{3,true,{"right_arm"}},SyncAimRequirement{12,true,{"right_arm"}}),"disjoint required screen halves also prevent competition away from native centre lines");
    memcpy(frame.shotBounds,bounds,24);auto automatic=frame;automatic.selectedAnimation.clear();SyncAutomaticSelection selection;selection.update(automatic);
    requireTarget(automatic.automaticAnimation==distant.name&&!syncGuideFeedback(automatic,syncBuildReferenceGuide(automatic).selected,distant.name).selectedOnlyObserved,"entering a limb area selects its conditional aiming guide without confirming the wrong current focus");
    automatic.jointFocus={true,"right_arm"};selection.update(automatic);requireTarget(automatic.automaticAnimation==distant.name,"automatic selection may choose the limb after native focus actually matches");
}
static void checkOrdinaryPriorityReader(unsigned char* image,uintptr_t base) {
    for(size_t at:{size_t(0x18dab4d),size_t(0x16ba68e),size_t(0x25f9e38),size_t(0xf6af60)})requireTarget(VirtualAlloc(image+(at&~size_t(4095)),4096,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit priority guard fixture page");
    const unsigned char copy[]{0xf3,0x0f,0x10,0x80,0x48,1,0,0,0xf3,0x0f,0x11,0x41,0x4c},equipment[]{0x49,0x8b,0x96,0xb8,2,0,0,0x48,0x3b,0x15,0x9c,0xf7,0xf3,0},specific[]{0xb0,1,0xc3};
    memcpy(image+0x18dab4d,copy,sizeof(copy));memcpy(image+0x16ba68e,equipment,sizeof(equipment));memcpy(image+0xf6af60,specific,3);
    const auto empty=reinterpret_cast<uintptr_t>("empty");memcpy(image+0x25f9e38,&empty,8);const auto validator=base+0xf6af60;memcpy(image+0x1038,&validator,8);
    std::vector<unsigned char> interaction(0x160),decl(0x398);targetPut(interaction,0,base+0x1000);targetPut(decl,0x148,2.f);targetPut(decl,0x2b8,empty);float priority=0;
    requireTarget(readSyncOrdinaryGkPriority(WallMemory{},base,targetAddress(interaction),targetAddress(decl),2,priority)&&priority==2,"audited ordinary GK reads the real default priority through the native empty equipment sentinel");
    requireTarget(VirtualAlloc(image+0x626d000,4096,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit declared generic interaction type fixture");
    const auto genericName=reinterpret_cast<uintptr_t>("idSyncAttackInteraction_Generic");memcpy(image+0x626d410,&genericName,8);targetPut(decl,0x90,base+0x626d410);
    requireTarget(readSyncAuthoredGkPriority(WallMemory{},base,targetAddress(decl),priority)&&priority==2,"owned generic source priority is available before a runtime candidate exists");
    targetPut(decl,0x90,base+0x626d418);requireTarget(!readSyncAuthoredGkPriority(WallMemory{},base,targetAddress(decl),priority),"custom interaction classes cannot inherit generic fixed-priority authoring");targetPut(decl,0x90,base+0x626d410);
    requireTarget(!readSyncOrdinaryGkPriority(WallMemory{},base,targetAddress(interaction),targetAddress(decl),3.25f,priority),"dynamic runtime priority cannot be treated as a fixed ordinary default");
    decl[0x314]=1;requireTarget(!readSyncOrdinaryGkPriority(WallMemory{},base,targetAddress(interaction),targetAddress(decl),2,priority),"Berserk priority boost paths remain outside the ordinary reference model");decl[0x314]=0;
    for(size_t at:{size_t(0xbbf4a8),size_t(0xbbf652),size_t(0x25e7554)})requireTarget(VirtualAlloc(image+(at&~size_t(4095)),4096,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit native AI bonus guard pages");
    const unsigned char countCode[]{0x41,0x8b,0x85,0x20,3,0,0,0x85,0xc0,0x0f,0x8e,0xb0,1,0,0},bonusCode[]{0x48,0x8b,0x44,0x24,0x38,0xf3,0x0f,0x10,0,0xf3,0x0f,0x58,5,0xf1,0x7e,0xa2,1,0xf3,0x0f,0x11,0};
    memcpy(image+0xbbf4a8,countCode,sizeof(countCode));memcpy(image+0xbbf652,bonusCode,sizeof(bonusCode));float bonus=.25f;memcpy(image+0x25e7554,&bonus,4);
    std::vector<unsigned char> actor(0x20),actorVtable(0x13a8),requirements(64);targetPut(actor,0,targetAddress(actorVtable));targetPut(actorVtable,0x13a0,base+0xbbf0b0);
    targetPut(interaction,0x140,targetAddress(actor));interaction[0x48]=1;targetPut(decl,0x318,SyncPreviewList{targetAddress(requirements),1,1,0});
    requireTarget(readSyncOrdinaryGkPriority(WallMemory{},base,targetAddress(interaction),targetAddress(decl),2.25f,priority)&&priority==2.25f,"eligible native AI requirement bonus is read instead of rejecting observed Soldier arm priority");
    requireTarget(!readSyncAuthoredGkPriority(WallMemory{},base,targetAddress(decl),priority),"authoring alone cannot guess whether a nonempty AI requirement bonus or an early-success bypass applies");
    requireTarget(!readSyncOrdinaryGkPriority(WallMemory{},base,targetAddress(interaction),targetAddress(decl),2,priority),"base priority on a nonempty AI branch cannot be used as a fixed competing priority");
    image[0xbbf652]^=1;requireTarget(!readSyncOrdinaryGkPriority(WallMemory{},base,targetAddress(interaction),targetAddress(decl),2.25f,priority),"changed native AI bonus consumer disables the priority inference");image[0xbbf652]^=1;
    targetPut(decl,0x318,SyncPreviewList{});
    targetPut(decl,0x2b8,uintptr_t(0));requireTarget(!readSyncOrdinaryGkPriority(WallMemory{},base,targetAddress(interaction),targetAddress(decl),2,priority),"a nonempty or unknown equipment requirement cannot suppress competitors using the base priority");
}
