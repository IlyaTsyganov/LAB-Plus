#pragma once

static void checkAutomaticBodyAvailability() {
    auto frame=automaticPositionFixture();
    frame.facing.origin={0,4,0};frame.ranges.playerOrigin={3,4,0};
    frame.spheres={{{0,4,1},.3f,"",1,true,"head"},{{-.6f,4,-.65f},.24f,"",2,true,"left_leg"},
        {{.6f,4,-.65f},.24f,"",3,true,"right_leg"},{{.6f,4,.6f},.23f,"",4,true,"right_arm"},{{0,4,.1f},.25f,"",5,true,"chest"}};
    syncShotBounds(frame.spheres,frame.shotBounds);frame.aim.known=true;memcpy(frame.aim.bounds,frame.shotBounds,24);
    frame.aim.quadrant=1;frame.jointFocus={true,"head"};
    auto upper=frame.regions[0];upper.name="upper";upper.aim.quadrants=3;
    auto lower=upper;lower.name="lower";lower.aim.quadrants=12;
    auto arm=upper;arm.name="arm";arm.aim={0,true,{"right_arm"}};
    frame.regions={upper,lower,arm};
    GameCamera camera{};camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    SyncAutomaticSelection automatic;automatic.update(frame);
    auto hasLower=[&](const SyncTargetFrame& value){std::vector<Vertex> fill;appendSyncTargetGeometry(camera,value,fill,false);
        for(const auto& v:fill)if((v.color[0]==1&&v.color[1]==.48f)||(v.color[0]==.15f&&v.color[1]==.45f)||(v.color[0]==.65f&&v.color[1]==.22f)){double x,y,w;
            if(syncAimProject(camera,v.position,x,y,w)&&y<-.08)return true;}return false;};
    // This assertion fails on the previous rendering path: only upper is drawn.
    requireTarget(hasLower(frame),"automatic upper aim must not hide legs accepted by another animation in the same standing zone");
    syncRefreshAimMesh(camera,frame);
    requireTarget(!frame.aimPartMesh->quadrantFill.empty()&&!frame.aimPartMesh->fill.empty(),"automatic mode shows simultaneous quadrant and named-part aim scopes");
    const auto requirements=syncVisibleAimRequirements(frame);
    frame.aim.quadrant=12;frame.jointFocus={true,"left_leg"};automatic.update(frame);
    requireTarget(hasLower(frame)&&syncVisibleAimRequirements(frame).size()==requirements.size(),"moving aim between head and legs does not remove available body guidance");
    frame.automaticAnimation.clear();frame.automaticResolved=true;
    requireTarget(hasLower(frame),"shared areas with no unique automatic animation still show valid aim references");
    const auto signature=syncAimMeshSignature(camera,frame,"");
    frame.ranges.playerOrigin={-3,4,0};
    requireTarget(!hasLower(frame)&&signature!=syncAimMeshSignature(camera,frame,""),"leaving all native zones removes aim references and invalidates the cached mesh");
    frame.ranges.playerOrigin={3,4,0};frame.regions.push_back(lower); // Identical scope in a second native branch.
    requireTarget(syncVisibleAimRequirements(frame).size()==requirements.size(),"repeated native branches do not stack the same aim mask");
    frame.regions[1].incoming={-1,0,0};frame.regions.back().incoming={-1,0,0};
    requireTarget(!hasLower(frame),"a lower rule in the opposite sector cannot leak into the occupied upper sector");
    frame.ranges.playerOrigin={-3,4,0};requireTarget(hasLower(frame),"the same lower aim scope becomes visible in its second native sector");
    frame.regions[1].minZ=1;frame.regions.back().minZ=1;
    requireTarget(!hasLower(frame),"aerial-only lower rules cannot be advertised at grounded foot height");
    frame.ranges.playerOrigin={3,4,0};frame.selectedAnimation="upper";
    requireTarget(!hasLower(frame),"manual selection still shows only its chosen aim scope");
    frame.selectedAnimation="lower";requireTarget(hasLower(frame),"manual lower selection shows actual body patches even while aiming upper");
    frame.selectedAnimation.clear();frame.rulesCoherent=false;requireTarget(!hasLower(frame),"changing native authoring cannot publish stale availability");frame.rulesCoherent=true;
    frame.action=1;frame.hasChainsaw=true;frame.ranges.ignoreQuickLunge=true;
    frame.chainsawRules={{"cut",SyncAimRequirement{12,true,{}},true}};
    requireTarget(!hasLower(frame),"Chainsaw never paints body parts, even with an authored lower-aim rule");
    const auto chainsawMesh=syncBuildTargetAimMesh(camera,frame);requireTarget(chainsawMesh.known&&chainsawMesh.fill.empty()&&chainsawMesh.quadrantFill.empty()&&chainsawMesh.lowerFill.empty(),"Chainsaw suppression applies to every body-colour channel");
    frame.chainsawRules[0].aim.known=false;requireTarget(!hasLower(frame),"unknown Chainsaw authoring cannot invent lower-body guidance");
    printf("PASS: integrated automatic/manual body availability, shared zones, native branches, height, cache and action isolation.\n");
}

static void checkCapturedBodyQuadrants() {
    size_t cases=0,visible=0;const auto enemies=loadCapturedLimbEnemies();
    for(const auto& enemy:enemies){const auto& frame=enemy.frame;if(!enemy.pose||!frame.aim.known)continue;
        for(const auto& rule:frame.animationRules)if(rule.action==0&&rule.aim.quadrants==12&&rule.aim.damageGroups.empty()){
            for(unsigned side=0;side<8;++side)for(bool close:{false,true}){
                ++cases;const auto camera=limbOrbitCamera(frame,side,close);
                const auto mesh=syncBuildAimPartMesh(camera,frame.shotBounds,frame.aim,&rule.aim,frame.spheres,true,true);
                requireTarget(mesh.known&&!mesh.required&&mesh.fill.empty(),"captured lower rules retain their quadrant-only identity");
                double cx,cy,w;const auto* b=frame.aim.bounds;
                requireTarget(syncAimProject(camera,{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},cx,cy,w),"captured physics centre projects");
                auto all=rule.aim;for(const auto& s:frame.spheres)if(std::find(all.damageGroups.begin(),all.damageGroups.end(),s.focusGroup)==all.damageGroups.end())all.damageGroups.push_back(s.focusGroup);
                const bool core=limbVisibleCore(camera,frame,all);visible+=core;
                if(core)requireTarget(!mesh.quadrantFill.empty(),"clearly visible lower body must produce an actual body patch on every captured model/view");
                for(size_t i=0;i<mesh.quadrantFill.size();i+=3){double x[3],y[3];
                    for(unsigned j=0;j<3;++j)requireTarget(syncAimProject(camera,mesh.quadrantFill[i+j],x[j],y[j],w)&&y[j]<=cy+1e-6,"lower body patch stays below native physics centre");
                    requireTarget(limbIndependentRay(camera,frame,all,(x[0]+x[1]+x[2])/3,(y[0]+y[1]+y[2])/3),"lower patch interiors hit a real shot sphere, never empty box space");
                }
            }
        }
    }
    requireTarget(cases==640&&visible>200,"captured lower-body regression covers all 640 model/direction/distance cases");
    printf("PASS: actual lower-body sphere masks: %zu cases, %zu visible cores.\n",cases,visible);
}

static void checkRecordedAutomaticBodyZones() {
    GameCamera camera{};auto frame=loadRightArmPose(camera);frame.selectedAnimation.clear();
    const auto limits=syncTargetReferenceLimits(frame.facing,frame.ranges);size_t zones=0;
    for(const auto& region:frame.regions)if(region.aim.quadrants==12&&region.aim.damageGroups.empty()){
        SyncRangeVolume volume;requireTarget(syncRangeBuild(&region,frame.facing,limits,false,frame.incomingYaw,volume),"recorded lower-aim zone has a native interior");
        auto entered=frame;entered.ranges.playerOrigin=volume.interior;
        SyncAutomaticSelection automatic;automatic.update(entered);
        const auto scopes=syncVisibleAimRequirements(entered);
        requireTarget(std::any_of(scopes.begin(),scopes.end(),[](const auto& aim){return aim.quadrants==12&&aim.damageGroups.empty();}),"every recorded lower-aim direction remains present in the automatic overlay");
        syncRefreshAimMesh(camera,entered);requireTarget(!entered.aimPartMesh->fill.empty()||!entered.aimPartMesh->quadrantFill.empty()||!entered.aimPartMesh->lowerFill.empty(),"actual lower-body guidance is drawn in every captured native direction");++zones;
    }
    requireTarget(zones==4,"recorded Zombie front, back, left and right lower-aim zones are covered");
    const auto at=std::chrono::steady_clock::now();size_t vertices=0;
    for(unsigned i=0;i<20;++i){const auto mesh=syncBuildTargetAimMesh(camera,frame);vertices=mesh.fill.size()+mesh.quadrantFill.size()+mesh.lowerFill.size();}
    printf("PASS: four recorded automatic lower-body zones; aim mesh mean %.3f ms, %zu fill vertices (offline CPU).\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-at).count()/20,vertices);
}
