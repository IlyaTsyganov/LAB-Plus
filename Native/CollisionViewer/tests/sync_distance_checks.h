#pragma once

static SyncTargetFrame distanceFixture() {
    SyncTargetFrame f;f.action=1;f.hasChainsaw=true;f.regionState="ready";f.selectionKnown=true;f.selectionTolerance=.381f;
    f.facing.axis[0]=f.facing.axis[4]=f.facing.axis[8]=1;
    f.ranges.known=true;f.ranges.ignoreQuickLunge=true;f.ranges.gloryKillTarget=f.ranges.chainsawTarget=f.ranges.lunge=10;
    f.aim.known=true;f.aim.quadrant=4;f.jointFocus={true,"left_leg"};
    f.chainsawAnimations={"left","right"};f.chainsawRules={{"left",{},true,true,1,{-2,0,0}},{"right",{},true,true,1,{2,0,0}}};
    const float box[]{-.4f,-.4f,0,.4f,.4f,2};memcpy(f.shotBounds,box,24);return f;
}
static void checkDistanceSelection() {
    auto f=distanceFixture();auto rules=syncReadDistanceRules(f);
    requireTarget(syncDistanceState(rules,{0,3,0}).tone==SyncDistanceTone::Random,"equal-priority equidistant clips form a random band");
    requireTarget(syncDistanceState(rules,{-4,0,0}).possible==1&&syncDistanceState(rules,{4,0,0}).possible==2,"opposite sides of a native distance band select different clips");
    requireTarget(syncDistanceState(rules,{20,0,0}).tone==SyncDistanceTone::Outside,"outside the actual action cap no clip is available");
    f.chainsawRules[0].priority=2;rules=syncReadDistanceRules(f);
    requireTarget(syncDistanceState(rules,{4,0,0}).possible==1,"priority wins before distance, including the nearer lower-priority clip");
    f.chainsawRules[0].selectionKnown=false;rules=syncReadDistanceRules(f);
    requireTarget(syncDistanceState(rules,{0,3,0}).tone==SyncDistanceTone::Unknown,"unread priority/site cannot be reported as random");
    f=distanceFixture();f.chainsawRules[0].aim={3,true,{"head"}};f.chainsawRules[1].aim={12,true,{"left_leg"}};
    requireTarget(syncReadDistanceRules(f).names==std::vector<std::string>{"right"},"disjoint upper/head and lower/leg aim conditions never create fake random overlap");
    f.jointFocus.known=false;requireTarget(syncDistanceState(syncReadDistanceRules(f),{0,3,0}).tone==SyncDistanceTone::Unknown,"unknown native focus is uncertainty rather than random choice");
    f=distanceFixture();f.chainsawRules[1].name="left";f.chainsawAnimations.clear();
    requireTarget(syncDistanceState(syncReadDistanceRules(f),{0,3,0}).tone==SyncDistanceTone::Single,"multiple interaction branches of one clip are not multiple animations");
    f.chainsawAnimations={"left"};f.chainsawRules[0].aim={3,true,{"head"}};f.chainsawRules[1].aim={12,true,{"left_leg"}};
    requireTarget(syncDistanceState(syncReadDistanceRules(f),{0,3,0}).tone==SyncDistanceTone::Single,"a later same-name Chainsaw branch retains its independent lower/leg aim scope");
    // An uncertain second branch of an already available clip may change the
    // winning priority; it must not certify the whole cell as random/single.
    auto branch=rules.rules.front();branch.animation=1;branch.priorityKnown=true;branch.priority=10;branch.volumes[0].balls={{Vec3{0,0,0},3,true}};
    rules=syncReadDistanceRules(distanceFixture());rules.rules.push_back(branch);
    requireTarget(syncDistanceState(rules,{0,3,0},.1).tone==SyncDistanceTone::Boundary,"same-name branch eligibility participates in conservative selector bounds");
    f=distanceFixture();const auto cached=syncDistanceMapFor(f);f.ranges.playerOrigin={4,0,0};
    requireTarget(syncDistanceMapFor(f)==cached&&syncDistanceChoiceText(f).find("single choice")!=std::string::npos,"XY motion reuses the diagram but recomputes current choice");
    const auto signature=syncDistanceSignature(f);
    for(unsigned kind=0;kind<8;++kind){auto changed=f;
        if(kind==0)changed.ranges.playerOrigin.z+=.2f;if(kind==1)changed.ranges.chainsawTarget-=1;
        if(kind==2)changed.jointFocus.group="head";if(kind==3)changed.chainsawRules[0].priority+=1;
        if(kind==4)changed.chainsawRules[0].selectionPosition.x+=1;if(kind==5)changed.selectionKnown=false;
        if(kind==6)changed.action=0;if(kind==7)changed.entity.generation++;
        requireTarget(syncDistanceSignature(changed)!=signature,"height/range/aim/priority/site/validity/action/identity changes invalidate the distance diagram");
    }
    f=distanceFixture();f.action=0;
    for(unsigned i=0;i<2;++i){SyncFacingRegion r;r.name=i?"right":"left";r.start={i?2.f:-2.f,0,0};r.incoming={1,0,0};r.halfYaw=syncFacingPi;r.maxXY=8;r.maxZ=5;r.priorityKnown=true;r.priority=1;f.regions.push_back(r);}
    rules=syncReadDistanceRules(f);
    requireTarget(rules.authored&&syncDistanceState(rules,{0,3,0}).tone==SyncDistanceTone::Random&&syncDistanceState(rules,{4,0,0}).possible==2,"GK uses its own authored start poses and equal-priority 3D random distance band");
    f.nativeChoices={{"left",0,1,{4,0,0},true},{"right",0,1,{-4,0,0},true}};rules=syncReadDistanceRules(f);
    requireTarget(!rules.authored&&syncDistanceState(rules,{4,0,0}).possible==1,"unique prepared GK branches use actual adjusted selector sites instead of authored reference sites");
    f.regions[0].maxXY=2;f.nativeChoices[0].priority=10;
    requireTarget(syncDistanceState(syncReadDistanceRules(f),{4,0,0}).possible==2,"an out-of-range high-priority GK branch is removed before choosing clips");
    printf("PASS: distance selector priority, 3D tolerance, uncertainty, aim isolation, same-clip branches and live cache invalidation.\n");
}
static void checkDistanceBodyFilter() {
    auto f=distanceFixture();f.action=0;f.facing.origin={0,4,0};
    SyncFacingRegion legs;legs.name="legs";legs.incoming={0,-1,0};legs.halfYaw=syncFacingPi;legs.maxXY=3;legs.maxZ=5;legs.aim={0,true,{"left_leg"}};
    auto upper=legs;upper.name="head";upper.maxXY=6;upper.aim={0,true,{"head"}};f.regions={legs,upper};
    f.animationRules={{"legs",0,legs.aim,{}},{"head",0,upper.aim,{}}};
    f.spheres={{{-.6f,4,-.6f},.3f,"",1,true,"left_leg"},{{0,4,1},.3f,"",2,true,"head"}};
    syncShotBounds(f.spheres,f.shotBounds);memcpy(f.aim.bounds,f.shotBounds,24);
    GameCamera camera{};camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    f.ranges.playerOrigin={0,2,0};auto mesh=syncBuildTargetAimMesh(camera,f);
    requireTarget(!mesh.lowerFill.empty()&&!mesh.fill.empty(),"near range includes separate leg and head animations");
    f.ranges.playerOrigin={0,-.5f,0};mesh=syncBuildTargetAimMesh(camera,f);
    requireTarget(mesh.lowerFill.empty()&&!mesh.fill.empty(),"only the still-available head is painted beyond the leg clip's start radius");
    f.selectedAnimation="legs";requireTarget(syncVisibleAimRequirements(f).empty(),"manual selection cannot keep a known out-of-range leg highlighted");
    f.selectedAnimation.clear();f.ranges.playerOrigin={0,-3,0};requireTarget(syncVisibleAimRequirements(f).empty(),"beyond every start radius all body guidance disappears");
    f.action=1;f.ranges.playerOrigin={0,-7,0};requireTarget(syncVisibleAimRequirements(f).empty(),"Chainsaw body guidance also disappears past its separate cap");
    printf("PASS: near/far partial body eligibility, manual range gate and independent Chainsaw cap.\n");
}
static void checkRecordedDistanceMaps() {
    std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-hellknight-executions-11.tsv");std::string line;unsigned cases=0,random=0;size_t certified=0;
    std::ofstream exportMap("build/distance-map-v24.tsv");exportMap.precision(9);double totalMs=0;
    while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;auto f=distanceFixture();std::istringstream fields(line);unsigned id,pool;double at;std::string clip;Vec3 front,back;
        fields>>id>>at>>clip;auto vec=[&](Vec3& v){fields>>v.x>>v.y>>v.z;};vec(f.ranges.playerOrigin);vec(f.facing.origin);vec(f.ranges.eyeOffset);vec(front);vec(back);fields>>f.selectionTolerance>>pool;
        requireTarget(!fields.fail(),"recorded execution distance inputs parse");f.chainsawRules[0].selectionPosition=front;f.chainsawRules[1].selectionPosition=back;
        f.shotBounds[0]=f.facing.origin.x-.5f;f.shotBounds[3]=f.facing.origin.x+.5f;f.shotBounds[1]=f.facing.origin.y-.5f;f.shotBounds[4]=f.facing.origin.y+.5f;
        const auto rules=syncReadDistanceRules(f);const auto state=syncDistanceState(rules,f.ranges.playerOrigin);++cases;
        requireTarget((state.tone==SyncDistanceTone::Random)==(pool==2),"archived random-route model classification remains reproducible; not the ordinary input policy");random+=state.tone==SyncDistanceTone::Random;
        const auto started=std::chrono::steady_clock::now();const auto map=syncBuildDistanceMap(f);totalMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
        requireTarget(map.known&&!map.fill.empty()&&!map.lines.empty()&&map.precision<.08,"recorded positions produce bounded detailed distance contours and random hatching");
        for(size_t i=0;i<map.fill.size();i+=3){const bool pink=map.fill[i].color[0]==1&&map.fill[i].color[1]==.12f;
            // Independent native formula at corners and triangle interior.
            for(unsigned j=0;j<4;++j){auto p=j<3?map.fill[i+j].position:motionScale(motionAdd(map.fill[i].position,motionAdd(map.fill[i+1].position,map.fill[i+2].position)),1.f/3);p.z=f.ranges.playerOrigin.z;
                auto distance=[&](Vec3 site){const auto d=subtract(p,site);return sqrt(double(d.x)*d.x+double(d.y)*d.y+double(d.z)*d.z);};
                const double a=distance(front),b=distance(back);const bool isRandom=fabs(a-b)<f.selectionTolerance;
                requireTarget(pink==isRandom,"every filled distance cell preserves the native random/single classification throughout its triangles");++certified;
            }
            if(cases==1){exportMap<<"F";for(unsigned j=0;j<3;++j)exportMap<<'\t'<<map.fill[i+j].position.x<<'\t'<<map.fill[i+j].position.y;for(unsigned j=0;j<4;++j)exportMap<<'\t'<<map.fill[i].color[j];exportMap<<'\n';}
        }
        if(cases==1){for(size_t i=0;i<map.lines.size();i+=2){exportMap<<"L";for(unsigned j=0;j<2;++j)exportMap<<'\t'<<map.lines[i+j].position.x<<'\t'<<map.lines[i+j].position.y;for(unsigned j=0;j<4;++j)exportMap<<'\t'<<map.lines[i].color[j];exportMap<<'\n';}
            exportMap<<"P\t"<<f.ranges.playerOrigin.x<<'\t'<<f.ranges.playerOrigin.y<<"\nE\t"<<f.facing.origin.x<<'\t'<<f.facing.origin.y<<'\n';}
    }
    requireTarget(cases==6&&random==3&&certified>10000,"archived distance-band model includes three overlap and three sole model predictions");
    printf("PASS: archived v24 model on six snapshots (not ordinary input), %zu distance triangle probes; map build mean %.3f ms (offline CPU).\n",certified,totalMs/cases);
}
// Called by the all-family fixture replay, after production declaration and
// cached-template parsing. Independent direct cylinder/sphere/angle predicates
// audit the map over directions, heights, ranges and both actions.
static void checkFamilyDistanceRules(SyncTargetFrame frame) {
    static size_t checks=0,families=0;++families;
    frame.ranges.eyeOffset={0,0,1.6f};frame.selectionTolerance=.381f;frame.aim.known=true;frame.jointFocus.known=true;
    auto accepted=[&](const SyncAimRequirement& a){return a.known&&(!a.quadrants||a.quadrants==15||a.quadrants==0x7fffffffu||(a.quadrants&frame.aim.quadrant))&&
        (a.damageGroups.empty()||std::find(a.damageGroups.begin(),a.damageGroups.end(),frame.jointFocus.group)!=a.damageGroups.end());};
    for(int action:{0,1})for(unsigned quadrant:{1u,4u}){frame.action=action;frame.aim.quadrant=quadrant;frame.jointFocus.group="";
        for(const auto& r:frame.animationRules)if(r.action==action&&!r.aim.damageGroups.empty()){frame.jointFocus.group=r.aim.damageGroups.front();break;}
        frame.ranges.playerOrigin=motionAdd(frame.facing.origin,{0,0,.23f});const auto rules=syncReadDistanceRules(frame);requireTarget(rules.known,"each reviewed family uses the generic distance-rule path");
        const auto map=syncBuildDistanceMap(frame);requireTarget(map.known,"all-family distance diagram remains within its geometry budget");
        for(float h:{.23f,1.13f,2.53f})for(unsigned side=0;side<8;++side)for(unsigned step=1;step<=40;++step){const double angle=(side+.137)*syncFacingPi/4,distance=step*.251;
            const auto p=motionAdd(frame.facing.origin,{float(distance*cos(angle)),float(distance*sin(angle)),h});uint64_t expected=0;
            const auto target=motionAdd(frame.facing.origin,syncRangeTransform(frame.facing,frame.ranges.viewTargetOffset));
            const Vec3 eye=motionAdd(p,{0,0,frame.ranges.eyeOffset.z}),delta=subtract(eye,target);const double actionRadius=action?frame.ranges.chainsawTarget:frame.ranges.gloryKillTarget;
            const bool targetFits=double(delta.x)*delta.x+double(delta.y)*delta.y+double(delta.z)*delta.z<=actionRadius*actionRadius;
            auto bit=[&](const std::string& name){const auto found=std::find(rules.names.begin(),rules.names.end(),name);requireTarget(found!=rules.names.end(),"eligible native rule exists in the map inventory");expected|=uint64_t(1)<<size_t(found-rules.names.begin());};
            if(targetFits){if(action){if(frame.hasChainsaw)for(const auto& r:frame.chainsawRules)if(r.quick&&accepted(r.aim))bit(r.name);}
                else for(const auto& r:frame.regions)if(accepted(r.aim)){
                    const auto start=motionAdd(frame.facing.origin,syncRangeTransform(frame.facing,r.start));const auto s=subtract(p,start),d=subtract(p,frame.facing.origin);
                    const auto incoming=syncRangeTransform(frame.facing,r.incoming);const double yaw=fabs(remainder(atan2(d.y,d.x)-atan2(incoming.y,incoming.x),2*syncFacingPi));
                    if(h<r.minZ||h>r.maxZ||yaw>r.halfYaw||double(s.x)*s.x+double(s.y)*s.y>double(r.maxXY)*r.maxXY)continue;bit(r.name);
                }}
            const auto measured=syncDistanceState(rules,p);
            if(measured.available!=expected)fprintf(stderr,"BAD DISTANCE %s action=%d q=%u height=%g side=%u step=%u actual=%llu expected=%llu\n",frame.enemyDefinition.c_str(),action,quadrant,h,side,step,static_cast<unsigned long long>(measured.available),static_cast<unsigned long long>(expected));
            requireTarget(measured.available==expected,"all-family distance availability agrees with independent native sphere/cylinder/height/direction predicates");++checks;
        }
    }
    if(families==25)printf("PASS: distance availability for all 25 families, both actions, aim scopes and %zu direction/height/range samples.\n",checks);
}
