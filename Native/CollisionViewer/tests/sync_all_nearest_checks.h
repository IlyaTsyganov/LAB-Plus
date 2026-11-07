#pragma once

struct NearestPriorityFixture {float base=0;unsigned ai=0;};
static const std::unordered_map<std::string,NearestPriorityFixture>& nearestPriorityFixture() {
    static const auto rows=[](){std::unordered_map<std::string,NearestPriorityFixture> result;
        std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-selector-priorities-11.tsv");requireTarget(bool(input),"native priority pool recording exists");std::string line;
        while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream f(line);std::string name;NearestPriorityFixture row;unsigned chosen;
            f>>name>>row.base>>row.ai>>chosen;requireTarget(!f.fail(),"native priority recording parses");result.emplace(name,row);}
        requireTarget(result.size()==338,"complete loaded native declaration priority pool");return result;}();return rows;
}
// Independent comparator, operating on original declarations rather than
// merged coverage. The family replay explicitly enables matching AI conditions.
static const SyncBodyChoiceRow* nearestOracle(const std::vector<SyncBodyChoiceRow>& rows,const std::string& group,unsigned q,Vec3 player) {
    const SyncBodyChoiceRow* best=nullptr;float bestPriority=-FLT_MAX,bestDistance=FLT_MAX;
    for(const auto& row:rows){const auto& a=row.aim;const bool any=a.quadrants==0||a.quadrants==15||a.quadrants==0x7fffffff;
        if(!any&&!(a.quadrants&q))continue;
        if(!a.damageGroups.empty()&&std::find(a.damageGroups.begin(),a.damageGroups.end(),group)==a.damageGroups.end())continue;
        const float dx=player.x-row.site.x,dy=player.y-row.site.y,dz=player.z-row.site.z;
        const float distance=(dy*dy+dx*dx)+dz*dz;
        if(!best||row.priority>bestPriority||(row.priority==bestPriority&&(distance<bestDistance||(distance==bestDistance&&row.order<best->order)))){
            best=&row;bestPriority=row.priority;bestDistance=distance;}
    }return best;
}
static size_t nearestFamilyViews=0,nearestContexts=0,nearestTriangles=0,nearestRegionViews=0;
static void checkNearestEnemyFamily(SyncTargetFrame frame,const std::string& family) {
    frame.nearestChoiceKnown=frame.nearestSelectionCurrent=true;frame.chainsawAnimations.clear();for(const auto& r:frame.chainsawRules)frame.chainsawAnimations.push_back(r.name);frame.selectorOrder.clear();frame.gloryKillOrder.clear();frame.aimSelectorSites.clear();
    frame.ranges.eyeOffset={0,0,1.6f};frame.ranges.gloryKillTarget=9;frame.ranges.chainsawTarget=6;
    size_t order=0;
    for(auto& r:frame.regions){const auto at=nearestPriorityFixture().find(r.audit.declaration);requireTarget(at!=nearestPriorityFixture().end(),"every reviewed GK declaration has a captured priority");
        r.priorityKnown=true;r.priority=at->second.base+(at->second.ai?.25f:0);
        SyncNativeChoice entry;entry.name=r.name;entry.declaration=r.audit.declaration;entry.action=0;entry.order=order++;frame.selectorOrder.push_back(entry);}
    order=0;
    for(auto& r:frame.chainsawRules){const auto at=nearestPriorityFixture().find(r.audit.declaration);requireTarget(at!=nearestPriorityFixture().end(),"every reviewed CS declaration has a captured priority");
        r.priority=at->second.base;r.selectionKnown=true;
        // Controlled QUICK_CHAINSAW sites: never infer facing from clip names.
        const float angle=float(order)*2.1f;r.selectionPosition=motionAdd(frame.facing.origin,syncRangeTransform(frame.facing,{2*cosf(angle),2*sinf(angle),.2f*float(order)}));
        SyncNativeChoice entry;entry.name=r.name;entry.declaration=r.audit.declaration;entry.action=1;entry.order=order++;frame.selectorOrder.push_back(entry);}
    std::vector<std::string> groups;for(const auto& sphere:frame.spheres)if(std::find(groups.begin(),groups.end(),sphere.focusGroup)==groups.end())groups.push_back(sphere.focusGroup);
    auto sample=[&](int action,Vec3 player,bool render){frame.action=action;frame.ranges.playerOrigin=player;
        const auto rows=syncBodyChoiceRows(frame);const auto plan=syncBodyChoicePlan(frame);requireTarget(plan.applies,"both ordinary actions use generic nearest-body planning");
        for(const auto& group:groups)for(unsigned q:{1u,2u,4u,8u}){const auto* expected=nearestOracle(rows,group,q,player);const auto* actual=syncBodyChoiceWinner(rows,group,q);
            requireTarget(bool(actual)==bool(expected)&&(!actual||actual->name==expected->name),"all-enemy original-scope comparator agrees in every quadrant at near/far/height positions");
            bool covered=false;for(const auto& a:plan.coverage)if((a.quadrants&q)&&std::find(a.damageGroups.begin(),a.damageGroups.end(),group)!=a.damageGroups.end())covered=true;
            requireTarget(covered==bool(expected),"body coverage contains exactly contexts with an eligible native winner");++nearestContexts;
        }
        if(render){const auto camera=carcassChoiceCamera(frame);const auto mesh=syncBuildTargetAimMesh(camera,frame);double cx,cy,w;
            static std::ofstream atlas("build/nearest-v27-atlas.tsv");
            const auto delta=subtract(player,frame.facing.origin);
            const float localX=dot(delta,{frame.facing.axis[0],frame.facing.axis[1],frame.facing.axis[2]}),localY=dot(delta,{frame.facing.axis[3],frame.facing.axis[4],frame.facing.axis[5]});
            const bool exportView=action==0&&(family=="imp"||family=="carcass");
            const auto* b=frame.aim.bounds;requireTarget(syncAimProject(camera,{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},cx,cy,w),"all-family target centre projects");
            for(unsigned channel=0;channel<3;++channel){const auto& points=channel==0?mesh.fill:channel==1?mesh.quadrantFill:mesh.lowerFill;
                for(size_t i=0;i<points.size();i+=3){double x=0,y=0;for(unsigned j=0;j<3;++j){double px,py;requireTarget(syncAimProject(camera,points[i+j],px,py,w),"nearest body triangle projects");x+=px/3;y+=py/3;}
                    const auto* hit=bodyColourFirstHit(camera,frame,x,y);requireTarget(hit,"nearest colouring does not paint empty space");
                    const unsigned q=y>cy?(x<cx?1u:2u):(x<cx?4u:8u);const auto* expected=nearestOracle(rows,hit->focusGroup,q,player);requireTarget(expected,"painted context has an eligible winner");
                    const unsigned colour=expectedNativeBodyColour(frame,{expected->aim},hit->focusGroup,q);
                    requireTarget(colour==channel+1,"every painted triangle has the colour of the winning original animation scope");++nearestTriangles;
                    if(exportView){atlas<<family<<'\t'<<hypotf(localX,localY)<<'\t'<<atan2f(localY,localX)<<'\t'<<channel;
                        for(unsigned j=0;j<3;++j){double px,py;syncAimProject(camera,points[i+j],px,py,w);atlas<<'\t'<<px<<'\t'<<py;}atlas<<'\n';}
                }
            }
        }++nearestFamilyViews;
    };
    for(int action:{0,1})for(unsigned side=0;side<8;++side)for(float height:{0.f,.5f,2.5f})for(float distance:{.5f,2.f,2.7f,3.3f,5.f,6.1f,9.1f}){
        const float angle=side*syncFacingPi/4;
        const auto player=motionAdd(frame.facing.origin,syncRangeTransform(frame.facing,{distance*cosf(angle),distance*sinf(angle),height}));
        sample(action,player,height==0&&side%2==0&&(distance==2.f||distance==5.f));
    }
    // Visit each supported native sector, including narrower sectors between
    // the eight radial samples. Range caps remain independent for both actions.
    for(const auto& r:frame.regions){const float height=r.minZ>0?(r.minZ+r.maxZ)*.5f:0;
        const float angle=atan2f(r.incoming.y,r.incoming.x);for(float distance:{1.f,3.f,5.f}){
            sample(0,motionAdd(frame.facing.origin,syncRangeTransform(frame.facing,{distance*cosf(angle),distance*sinf(angle),height})),false);++nearestRegionViews;}}
}
static void checkOrdinaryChainsawTrace() {
    std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-imp-chainsaw-nearest-11.tsv");requireTarget(bool(input),"hardware-captured ordinary Imp chainsaw fixture exists");
    SyncTargetFrame frame;frame.action=1;frame.nearestChoiceKnown=frame.nearestSelectionCurrent=frame.candidatesReady=true;std::string line,expected;
    while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream f(line);char kind;f>>kind;
        if(kind=='P')f>>frame.ranges.playerOrigin.x>>frame.ranges.playerOrigin.y>>frame.ranges.playerOrigin.z;
        if(kind=='C'){SyncNativeChoice row;float distance;row.action=1;row.known=true;row.order=frame.nativeChoices.size();f>>row.name>>row.priority>>distance>>row.position.x>>row.position.y>>row.position.z;
            requireTarget(syncNearestDistanceSquared(frame.ranges.playerOrigin,row.position)==distance,"Imp capture replays exact native float distance");frame.chainsaw.push_back(row.name);frame.nativeChoices.push_back(row);}
        if(kind=='E')f>>expected;
    }
    requireTarget(frame.nativeChoices.size()==2&&syncNativeSelection(frame).possible==std::vector<std::string>{expected},"ordinary QUICK_CHAINSAW executes nearest start with no random band");
    auto f=distanceFixture();f.nearestChoiceKnown=f.nearestSelectionCurrent=true;f.ranges.playerOrigin={-.05f,3,0};f.selectedAnimation="left";
    const auto guide=syncBuildExclusiveGuide(f);requireTarget(guide.known&&syncExclusiveContains(guide,f.ranges.playerOrigin),"nearest CS guide includes positions previously hidden by the unrelated random band");
    f.nativeChoices={{"right",1,1,{2,0,0},true},{"left",1,1,{-2,0,0},true}};f.chainsaw={"left","right"};f.candidatesReady=true;
    f.nativeChoices[0].order=1;f.nativeChoices[1].order=0;f.ranges.playerOrigin={0,3,0};
    requireTarget(syncNativeSelection(f).possible==std::vector<std::string>{"left"},"CS exact ties retain native order regardless of menu order");
    f.pendingChainsaw=true;requireTarget(!syncNativeSelection(f).known,"pending CS checks cannot certify execution");
    printf("PASS: hardware-captured Imp chainsaw route, float distances, nearest guide, tie order and pending safety.\n");
}
static void checkNearestBranchesAndGuides() {
    auto f=capturedCarcassNearest();f.aim.known=true;f.aim.quadrant=1;f.jointFocus={true,"head"};f.selectedAnimation="kill_front_head";
    f.ranges.playerOrigin=motionAdd(f.facing.origin,syncRangeTransform(f.facing,{3.3f,0,0}));
    const auto nearPosition=motionAdd(f.facing.origin,syncRangeTransform(f.facing,{2.9f,0,0}));
    const auto farPosition=motionAdd(f.facing.origin,syncRangeTransform(f.facing,{3.1f,0,0}));
    const auto head=syncBuildReferenceGuide(f,false);f.selectedAnimation="kill_front_chest";const auto chest=syncBuildReferenceGuide(f,false);
    requireTarget(head.known&&chest.known&&syncExclusiveContains(head,nearPosition)&&!syncExclusiveContains(head,farPosition)&&!syncExclusiveContains(chest,nearPosition)&&syncExclusiveContains(chest,farPosition),"GK standing boundaries switch at the real nearest-site plane, rather than removing the full overlap");
    // Same clip, distinct declaration: a live site belongs only to its branch.
    f.regions.resize(2);for(auto& r:f.regions){r.name="shared";r.aim={0,true,{}};r.halfYaw=syncFacingPi;r.minZ=0;r.maxZ=10;r.maxXY=20;r.priorityKnown=true;r.priority=3;}
    f.regions[0].audit.declaration="branch-A";f.regions[1].audit.declaration="branch-B";f.regions[1].priority=0;
    f.aimSelectorSites={{"shared",0,0,f.ranges.playerOrigin,true,"branch-B",1}};
    const auto rows=syncBodyChoiceRows(f);requireTarget(rows.size()==2&&rows[0].priority==3&&rows[1].priority==0&&rows[1].distanceSquared==0,"same-name live positions and priorities cannot leak between declarations");
    f=capturedCarcassNearest();f.ranges.playerOrigin=motionAdd(f.facing.origin,syncRangeTransform(f.facing,{5,0,0}));
    for(const auto& r:f.regions)if(r.name=="kill_front_head"||r.name=="kill_front_chest"){
        const size_t order=r.name=="kill_front_head"?0:1;
        f.aimSelectorSites.push_back({r.name,0,0,f.ranges.playerOrigin,true,r.audit.declaration,order});
        if(order==0)f.aimSelectorSites.push_back({r.name,0,0,f.ranges.playerOrigin,true,r.audit.declaration,2});
    }
    const auto duplicates=syncBodyChoiceRows(f);const auto* first=syncBodyChoiceWinner(duplicates,"head",1);
    requireTarget(first&&first->name=="kill_front_head","duplicate coherent sites retain the earliest interaction, even with another clip between copies");
    f=distanceFixture();f.nearestChoiceKnown=true;f.selectedAnimation.clear();f.spheres={{{0,0,1},.2f,"",1,true,"head"},{{0,0,0},.2f,"",2,true,"left_leg"}};
    f.ranges.playerOrigin={0,3,0};for(auto& r:f.chainsawRules)r.selectionKnown=false;
    requireTarget(!syncBodyChoicePlan(f).coverage.empty(),"unknown CS site cannot hide a body scope shared by every possible clip");
    f.selectedAnimation="left";requireTarget(syncBodyChoicePlan(f).coverage.empty(),"shared-scope fallback does not invent a specific winning CS clip");
    f.selectedAnimation.clear();f.chainsawRules[0].aim={3,true,{"head"}};
    const auto uncertain=syncBodyChoicePlan(f);bool upperHead=false,lowerHead=false;
    for(const auto& a:uncertain.coverage)if(std::find(a.damageGroups.begin(),a.damageGroups.end(),"head")!=a.damageGroups.end()){upperHead|=bool(a.quadrants&3);lowerHead|=bool(a.quadrants&12);}
    requireTarget(!upperHead&&lowerHead,"uncertain overlapping head competitors hide upper contexts while preserving uncontested lower contexts");
    f=distanceFixture();f.nearestChoiceKnown=true;f.selectedAnimation="left";f.ranges.playerOrigin={0,3,0};
    f.chainsawRules[1].selectionPosition=f.chainsawRules[0].selectionPosition;
    f.selectorOrder={{"left",1,0,{},false,"",0},{"right",1,0,{},false,"",1}};
    requireTarget(syncExclusiveContains(syncBuildExclusiveGuide(f),f.ranges.playerOrigin),"CS identical sites preserve the first native interaction's standing area");
    std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-hellknight-executions-11.tsv");std::string line;unsigned cases=0;
    while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);unsigned id,pool;double at,tolerance;std::string clip;Vec3 front,back,enemy,eye;
        auto vec=[&](Vec3& v){fields>>v.x>>v.y>>v.z;};fields>>id>>at>>clip;vec(f.ranges.playerOrigin);vec(enemy);vec(eye);vec(front);vec(back);fields>>tolerance>>pool;
        f.nearestSelectionCurrent=f.candidatesReady=true;f.chainsaw={"cut_front_01","cut_back_01"};f.nativeChoices={{"cut_front_01",1,1,front,true,"",0},{"cut_back_01",1,1,back,true,"",1}};
        requireTarget(!fields.fail()&&syncNativeSelection(f).possible==std::vector<std::string>{clip},"all six actual Hell Knight clips agree with ordinary nearest selection, including former false random-band cases");++cases;
    }
    requireTarget(cases==6,"six independent Hell Knight execution snapshots replayed");
    printf("PASS: GK nearest standing boundary, declaration isolation, uncertain common scopes, CS identical-site order, six Hell Knight executions.\n");
}
static void checkCompactFinisherPresentation() {
    auto frame=capturedCarcassNearest();frame.ranges.playerOrigin=motionAdd(frame.facing.origin,syncRangeTransform(frame.facing,{3.3f,0,0}));
    frame.aim.quadrant=1;frame.aim.known=true;frame.jointFocus={true,"head"};SyncAutomaticSelection automatic;automatic.update(frame);
    const auto camera=carcassChoiceCamera(frame);syncRefreshAimMesh(camera,frame);const auto gk=frame.aimPartMesh;
    const auto lines=syncPositionGuideLines(frame);requireTarget(lines.size()<=6,"finisher HUD stays within six useful lines");
    for(const auto& line:lines)for(const char* removed:{"native","quadrants","priority","Engine candidates","Authored","player-hitbox"})
        requireTarget(line.find(removed)==std::string::npos,"technical selection and geometry explanations belong in the guide, not the HUD");
    frame.action=1;frame.selectedAnimation="cut_front_01";frame.automaticResolved=false;
    SyncAimRequirement anywhere;anywhere.quadrants=0x7fffffff;
    const auto audit=syncAuditEnemyRule("syncdeath/playervsai/zombie_tier1/chainsaw/cut_front","cut_front_01",1,anywhere);
    frame.animationRules={{"cut_front_01",1,anywhere,audit}};frame.chainsawAnimations={"cut_front_01","cut_front_02"};frame.hasChainsaw=true;
    const auto chainsawLines=syncPositionGuideLines(frame);
    requireTarget(chainsawLines.size()<=6&&std::find(chainsawLines.begin(),chainsawLines.end(),"Selected: Front 01 / 02 (random)")!=chainsawLines.end(),"HUD does not promise a prepared Zombie front clip");
    frame.aimPartMesh=gk;frame.aimPartSignature=syncAimMeshSignature(camera,frame,frame.selectedAnimation);
    frame.ranges.known=false;frame.referenceGuide=std::make_shared<const SyncExclusiveGuide>();
    std::vector<Vertex> fill;appendSyncTargetGeometry(camera,frame,fill,false);
    requireTarget(fill.empty(),"switching to Chainsaw cannot draw even a stale cached GK body mesh");
    printf("PASS: compact finisher HUD, truthful random-variant label and stale-mesh isolation in Chainsaw.\n");
}
