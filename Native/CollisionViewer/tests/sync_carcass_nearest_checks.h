#pragma once

static SyncTargetFrame capturedCarcassNearest() {
    const auto enemies=loadCapturedLimbEnemies("Native/CollisionViewer/tests/fixtures/sync-body-world-v23.tsv",false);
    const auto enemy=std::find_if(enemies.begin(),enemies.end(),[](const auto& e){return e.definition=="ai/fodder/carcass";});
    requireTarget(enemy!=enemies.end()&&enemy->geometryComplete,"Carcass selection replay has real model and complete native templates");
    auto value=enemy->frame;value.enemyDefinition=enemy->definition;value.action=0;value.regionState="ready";value.nearestChoiceKnown=value.nearestSelectionCurrent=true;
    value.ranges.known=true;value.ranges.gloryKillTarget=value.ranges.lunge=10;value.ranges.eyeOffset={0,0,1.65735f};value.selectedAnimation.clear();
    for(const auto& r:value.regions)value.gloryKillOrder.push_back(r.name);return value;
}
static GameCamera carcassChoiceCamera(const SyncTargetFrame& value) {
    const auto* b=value.shotBounds;Vec3 centre{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f};
    const auto eye=motionAdd(value.ranges.playerOrigin,{0,0,1.65735f});auto forward=subtract(centre,eye);forward=motionScale(forward,1/sqrtf(dot(forward,forward)));
    auto right=cross(forward,{0,0,1});right=motionScale(right,1/sqrtf(dot(right,right)));const auto up=cross(right,forward);GameCamera camera{};
    camera.position[0]=eye.x;camera.position[1]=eye.y;camera.position[2]=eye.z;
    for(const auto& row:{std::make_pair(0u,right),std::make_pair(4u,up),std::make_pair(12u,forward)}){const float xyz[]{row.second.x,row.second.y,row.second.z};for(unsigned j=0;j<3;++j)camera.matrix[row.first+j]=xyz[j];camera.matrix[row.first+3]=-dot(row.second,eye);}return camera;
}
static void checkCarcassNativeTrace() {
    std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-carcass-nearest-11.tsv");requireTarget(bool(input),"hardware-captured Carcass selector fixture exists");
    SyncTargetFrame value;value.enemyDefinition="ai/fodder/carcass";value.action=0;value.nearestChoiceKnown=value.nearestSelectionCurrent=true;value.candidatesReady=true;
    value.selectionKnown=true;value.selectionTolerance=.381f;std::string line,expected;std::vector<std::pair<uintptr_t,std::vector<unsigned char>>> fingerprints;
    while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);char kind;fields>>kind;
        if(kind=='P')fields>>value.ranges.playerOrigin.x>>value.ranges.playerOrigin.y>>value.ranges.playerOrigin.z;
        if(kind=='C'){SyncNativeChoice row;float squared=0;row.action=0;row.known=true;fields>>row.name>>row.priority>>squared>>row.position.x>>row.position.y>>row.position.z;
            requireTarget(syncNearestDistanceSquared(value.ranges.playerOrigin,row.position)==squared,"native single-precision distance exactly matches register capture");
            value.nativeChoices.push_back(row);value.gloryKills.push_back(row.name);value.gloryKillOrder.push_back(row.name);}
        if(kind=='E')fields>>expected;
        if(kind=='B'){std::string at,hex;fields>>at>>hex;std::vector<unsigned char> bytes;for(size_t i=0;i<hex.size();i+=2)bytes.push_back(static_cast<unsigned char>(std::stoul(hex.substr(i,2),nullptr,16)));fingerprints.push_back({std::stoull(at,nullptr,16),bytes});}
    }
    requireTarget(value.nativeChoices.size()==2&&fingerprints.size()==4&&expected=="kill_front_chest","captured ordinary selector compared head/chest and executed chest");
    requireTarget(syncNativeSelection(value).possible==std::vector<std::string>{expected},"normal Carcass input picks the nearer chest, not both clips inside the unrelated random band");
    auto legacy=value;legacy.nearestChoiceKnown=false;requireTarget(syncNativeSelection(legacy).possible.size()==2,"regression fixture reproduces the former wrong random-band interpretation");
    value.selectionTolerance=0;value.selectionKnown=false;requireTarget(syncNativeSelection(value).possible==std::vector<std::string>{expected},"normal selector does not depend on the random-route cvar or guard");
    value.nativeChoices[0].priority=1;requireTarget(syncNativeSelection(value).possible==std::vector<std::string>{"kill_front_head"},"runtime priority is evaluated before nearest distance");
    value.nativeChoices[0].priority=0;value.nativeChoices[0].position=value.nativeChoices[1].position;
    std::reverse(value.nativeChoices.begin(),value.nativeChoices.end());requireTarget(syncNativeSelection(value).possible==std::vector<std::string>{"kill_front_head"},"an exact tie retains the first native interaction, independent of menu sorting");
    value.pendingGloryKill=true;requireTarget(!syncNativeSelection(value).known,"pending observations still cannot claim final execution eligibility");
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1900000,MEM_RESERVE,PAGE_READWRITE));requireTarget(image!=nullptr,"reserve selector fingerprint fixture");
    for(const auto& entry:fingerprints){requireTarget(VirtualAlloc(image+(entry.first&~uintptr_t(4095)),4096,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit captured selector code page");memcpy(image+entry.first,entry.second.data(),entry.second.size());}
    requireTarget(syncNearestChoiceProfile(WallMemory{},reinterpret_cast<uintptr_t>(image)),"actual ordinary-selector instructions pass the code guard");
    image[0x16996c7]^=1;requireTarget(!syncNearestChoiceProfile(WallMemory{},reinterpret_cast<uintptr_t>(image)),"changed normal-input call site disables the nearest-selector assumption");VirtualFree(image,0,MEM_RELEASE);
    printf("PASS: exact hardware-captured Carcass selection, code guard, native float math, priority, tie order and ignored random tolerance.\n");
}
static void checkCarcassBodyThresholds() {
    auto value=capturedCarcassNearest();
    auto place=[&](float distance,float angle=0,float height=0){value.ranges.playerOrigin=motionAdd(value.facing.origin,syncRangeTransform(value.facing,{distance*cosf(angle),distance*sinf(angle),height}));};
    auto winner=[&](const char* group,uint32_t quadrant){const auto rows=syncBodyChoiceRows(value);const auto* r=syncBodyChoiceWinner(rows,group,quadrant);return r?r->region->name:std::string{};};
    auto contains=[](const auto& groups,const std::string& group){return std::find(groups.begin(),groups.end(),group)!=groups.end();};
    place(2.55f);requireTarget(winner("head",1)=="kill_front_head"&&winner("left_arm",4)=="kill_front_leftarm"&&winner("right_arm",8)=="kill_front_rightarm","nearby head and both arms retain their separate native clips");
    const auto camera=carcassChoiceCamera(value);const auto nearSignature=syncAimMeshSignature(camera,value,"");
    place(2.56f);requireTarget(winner("left_arm",4)=="kill_front_lower"&&winner("left_arm",1)=="kill_front_leftarm","lower arm aim switches near 2.556766 m while upper arm remains independently selectable");
    auto plan=syncBodyChoicePlan(value);requireTarget(!contains(plan.colours.namedGroups[2],"left_arm")&&contains(plan.colours.lowerGroups[2],"left_arm")&&contains(plan.colours.namedGroups[0],"left_arm"),"one anatomical arm can have orange upper and purple lower aim, following the winning animation");
    requireTarget(nearSignature!=syncAimMeshSignature(camera,value,""),"winner colour changes invalidate cached patches even if geometric availability is unchanged");
    place(2.95f);requireTarget(winner("head",1)=="kill_front_head","head remains selectable just inside its true straight-front switch");
    place(2.97f);requireTarget(winner("head",1)=="kill_front_chest"&&winner("left_arm",1)=="kill_front_chest","upper head/arm aim switches near 2.957593 m to chest");
    place(5);plan=syncBodyChoicePlan(value);requireTarget(plan.detail==SyncAimDetail::Halves&&plan.colours.namedGroups[0].empty()&&plan.colours.namedGroups[1].empty(),"far Carcass exposes actual upper/lower winners instead of invented separate head/arm choices");
    value.aim.known=true;value.aim.quadrant=1;value.jointFocus={true,"head"};SyncAutomaticSelection automatic;automatic.update(value);
    requireTarget(value.automaticAnimation=="kill_front_chest","automatic label uses the actual nearest clip even when old exclusive standing geometry is empty");
    const auto hud=syncPositionGuideLines(value);requireTarget(hud.size()<=6&&std::find(hud.begin(),hud.end(),"Automatic: Front chest")!=hud.end(),"compact HUD retains the actual resolved Carcass choice");
    value.aimSelectorSites={{"kill_front_head",0,0,value.ranges.playerOrigin,true}};
    for(const auto& r:value.regions)if(r.name=="kill_front_head")value.aimSelectorSites[0].declaration=r.audit.declaration;
    requireTarget(winner("head",1)=="kill_front_head","a current adjusted native site overrides an authored reference start");value.aimSelectorSites.clear();
    value.selectedAnimation="kill_front_head";requireTarget(syncVisibleAimRequirements(value).empty(),"manual head selection cannot advertise a losing native clip at long range");value.selectedAnimation.clear();
    place(2,0,.2f);plan=syncBodyChoicePlan(value);requireTarget(winner("head",1)=="kill_above_front"&&plan.detail==SyncAimDetail::WholeBody&&plan.colours.lowerGroups[2].empty(),"higher-priority aerial clip wins and does not invent a lower-half colour from coverage masks");
    place(3,.2f);const auto a=winner("head",1);place(3,-.2f);const auto b=winner("head",1);requireTarget(a!=b,"the switching boundary depends on lateral approach, not a universal guessed radius");
    value.nearestChoiceKnown=false;requireTarget(!syncBodyChoicePlan(value).applies,"unverified selector retains the previous availability path");
    value.nearestChoiceKnown=true;value.enemyDefinition="ai/fodder/zombie_tier_1";requireTarget(syncBodyChoicePlan(value).applies,"the verified ordinary selector applies to other enemy models too");
    printf("PASS: Carcass true near/far thresholds, per-quadrant arm detail, cache, manual selection, aerial priority and lateral dependence.\n");
}
static void checkCarcassWinnerColours() {
    auto value=capturedCarcassNearest();size_t triangles=0,cases=0;std::ofstream atlas("build/carcass-v26-atlas.tsv");atlas.precision(9);
    for(unsigned side=0;side<8;++side)for(float distance:{2.3f,2.7f,3.3f,5.f}){
        const float angle=side*syncFacingPi/4;value.ranges.playerOrigin=motionAdd(value.facing.origin,syncRangeTransform(value.facing,{distance*cosf(angle),distance*sinf(angle),0}));
        const auto camera=carcassChoiceCamera(value);const auto mesh=syncBuildTargetAimMesh(camera,value);const auto* b=value.aim.bounds;double cx,cy,w;
        requireTarget(syncAimProject(camera,{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},cx,cy,w),"Carcass native physics centre projects");
        for(unsigned channel=0;channel<3;++channel){const auto& points=channel==0?mesh.fill:channel==1?mesh.quadrantFill:mesh.lowerFill;
            for(size_t i=0;i<points.size();i+=3){double x=0,y=0;for(unsigned j=0;j<3;++j){double px,py;syncAimProject(camera,points[i+j],px,py,w);x+=px/3;y+=py/3;}
                const auto* hit=bodyColourFirstHit(camera,value,x,y);requireTarget(hit,"winner colouring always hits actual Carcass geometry");
                const unsigned quadrant=y>cy?(x<cx?1u:2u):(x<cx?4u:8u);const SyncFacingRegion* best=nullptr;double bestDistance=DBL_MAX;
                // Independent oracle: original scope + native volume, then
                // priority / Euclidean distance; no merged-scope colour rules.
                for(const auto& r:value.regions){if(!syncAimMatches(r.aim,quadrant)||(!r.aim.damageGroups.empty()&&std::find(r.aim.damageGroups.begin(),r.aim.damageGroups.end(),hit->focusGroup)==r.aim.damageGroups.end()))continue;
                    SyncRangeVolume volume;if(!syncRangeBuild(&r,value.facing,syncTargetReferenceLimits(value.facing,value.ranges),false,0,volume,false)||!syncRangeContains(volume,value.ranges.playerOrigin))continue;
                    const auto site=motionAdd(value.facing.origin,syncRangeTransform(value.facing,r.start)),delta=subtract(value.ranges.playerOrigin,site);const double d=double(delta.x)*delta.x+double(delta.y)*delta.y+double(delta.z)*delta.z;
                    if(!best||r.priority>best->priority||(r.priority==best->priority&&d<bestDistance)){best=&r;bestDistance=d;}
                }
                requireTarget(best,"every painted context has a real winning native interaction");
                const bool leg=hit->focusGroup.find("leg")!=std::string::npos||hit->focusGroup.find("foot")!=std::string::npos;
                const bool named=!best->aim.damageGroups.empty()&&(hit->focusGroup=="head"||hit->focusGroup.find("arm")!=std::string::npos);
                const bool lower=best->aim.quadrants!=0&&best->aim.quadrants!=15&&best->aim.quadrants!=0x7fffffffu&&!(best->aim.quadrants&3);
                const unsigned expected=leg?3:named?1:lower?3:2;requireTarget(expected==channel+1,"Carcass triangle colour matches the animation that actually wins for that group/quadrant/distance");++triangles;
                if(side==0){atlas<<distance<<'\t'<<channel;for(unsigned j=0;j<3;++j){double px,py;syncAimProject(camera,points[i+j],px,py,w);atlas<<'\t'<<px<<'\t'<<py;}atlas<<'\n';}
            }
        }
        if(side==0&&distance==5)requireTarget(mesh.fill.empty()&&!mesh.quadrantFill.empty()&&!mesh.lowerFill.empty(),"integrated far-front model has blue upper / purple lower and no orange named-part patches");++cases;
    }
    requireTarget(cases==32&&triangles>10000,"all eight views and four distances exercise real winning colours");
    printf("PASS: Carcass nearest-winner colours: %zu views, %zu independently checked original-rule triangles.\n",cases,triangles);
}
