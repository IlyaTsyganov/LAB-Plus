#pragma once

static unsigned expectedNativeBodyColour(const SyncTargetFrame& frame,const std::vector<SyncAimRequirement>& scopes,const std::string& group,unsigned quadrant) {
    bool accepted=false,named=false,lower=false;
    const bool leg=group.find("leg")!=std::string::npos||group.find("foot")!=std::string::npos;
    const bool appendage=group=="head"||group=="face"||group=="eye"||group=="jaw"||group=="mouth"||group.find("arm")!=std::string::npos||group.find("wing")!=std::string::npos||group.find("tail")!=std::string::npos||group.find("horn")!=std::string::npos||group.find("mandible")!=std::string::npos||group.find("brain")!=std::string::npos;
    for(const auto& rule:scopes){const bool all=rule.quadrants==0||rule.quadrants==15||rule.quadrants==0x7fffffffu;
        if(!rule.known||(!all&&!(rule.quadrants&quadrant)))continue;
        if(!rule.damageGroups.empty()&&std::find(rule.damageGroups.begin(),rule.damageGroups.end(),group)==rule.damageGroups.end())continue;
        accepted=true;
        if(!rule.damageGroups.empty())for(const auto& s:frame.spheres)if(!s.focusGroup.empty()&&std::find(rule.damageGroups.begin(),rule.damageGroups.end(),s.focusGroup)==rule.damageGroups.end())named=true;
        lower|=!all&&!(rule.quadrants&3);
    }
    return !accepted?0:leg?3:named&&appendage?1:lower?3:2;
}
static size_t auditNativeBodyColours(const GameCamera& camera,const SyncTargetFrame& frame,const std::vector<SyncAimRequirement>& scopes,const SyncAimPartMesh& mesh) {
    double cx,cy,w;const auto* b=frame.aim.bounds;
    if(!syncAimProject(camera,{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},cx,cy,w)){
        requireTarget(mesh.fill.empty()&&mesh.quadrantFill.empty()&&mesh.lowerFill.empty(),"unknown/behind-camera native aim reference cannot produce invented colour");return 0;
    }
    size_t checked=0;
    for(unsigned channel=0;channel<3;++channel){const auto& points=channel==0?mesh.fill:channel==1?mesh.quadrantFill:mesh.lowerFill;
        for(size_t i=0;i<points.size();i+=3){double x=0,y=0;
            for(unsigned j=0;j<3;++j){double px,py;requireTarget(syncAimProject(camera,points[i+j],px,py,w),"native colour audit vertex projects");x+=px/3;y+=py/3;}
            const auto* hit=bodyColourFirstHit(camera,frame,x,y);requireTarget(hit,"all-zone paint must hit actual body geometry first");
            const unsigned quadrant=y>cy?(x<cx?1:2):(x<cx?4:8);
            const unsigned expected=expectedNativeBodyColour(frame,scopes,hit->focusGroup,quadrant);
            if(expected!=channel+1){fprintf(stderr,"BAD ZONE COLOUR %s focus=%s q=%u expected=%u painted=%u\n",frame.enemyDefinition.c_str(),hit->focusGroup.c_str(),quadrant,expected,channel+1);
                std::ofstream bad("build/bad-zone-v23.tsv");bad.precision(12);bad<<"C";for(auto v:camera.matrix)bad<<'\t'<<v;for(auto v:camera.position)bad<<'\t'<<v;bad<<'\n';
                bad<<"Q";for(auto v:frame.aim.bounds)bad<<'\t'<<v;bad<<'\n';bad<<"P\t"<<x<<'\t'<<y<<'\n';
                for(unsigned j=0;j<3;++j)bad<<"V\t"<<points[i+j].x<<'\t'<<points[i+j].y<<'\t'<<points[i+j].z<<'\n';
                for(const auto& s:frame.spheres)bad<<"S\t"<<s.center.x<<'\t'<<s.center.y<<'\t'<<s.center.z<<'\t'<<s.radius<<'\t'<<s.focusGroup<<'\n';
            }
            requireTarget(expected==channel+1,"all-zone colour must match original native part/quadrant rules, never the merged anatomical label");++checked;
        }
    }return checked;
}
static bool colourCoversPoint(const GameCamera& camera,const std::vector<Vec3>& triangles,double x,double y) {
    for(size_t i=0;i<triangles.size();i+=3){double px[3],py[3],w;bool valid=true;
        for(unsigned j=0;j<3;++j)valid&=syncAimProject(camera,triangles[i+j],px[j],py[j],w);if(!valid)continue;
        double sign[3];for(unsigned j=0;j<3;++j){const auto k=(j+1)%3;sign[j]=(px[k]-px[j])*(y-py[j])-(py[k]-py[j])*(x-px[j]);}
        if((*std::min_element(sign,sign+3)>=-1e-10||*std::max_element(sign,sign+3)<=1e-10)&&fabs((px[1]-px[0])*(py[2]-py[0])-(py[1]-py[0])*(px[2]-px[0]))>1e-14)return true;
    }return false;
}
static void checkCapturedCarcassLowerTorso() {
    const char* path="Native/CollisionViewer/tests/fixtures/sync-body-world-v23.tsv";
    const auto enemies=loadCapturedLimbEnemies(path,false);GameCamera camera{};Vec3 player{};std::ifstream input(path);std::string line;
    while(std::getline(input,line)){std::istringstream fields(line);std::string kind;fields>>kind;
        if(kind=="#CAMERA"){for(auto& f:camera.matrix)fields>>f;for(auto& f:camera.position)fields>>f;}
        else if(kind=="#PLAYER")fields>>player.x>>player.y>>player.z;
    }
    const auto it=std::find_if(enemies.begin(),enemies.end(),[](const auto& e){return e.definition=="ai/fodder/carcass";});
    requireTarget(it!=enemies.end()&&it->geometryComplete,"reported Carcass snapshot includes coherent real rules and geometry");
    auto frame=it->frame;frame.enemyDefinition=it->definition;frame.regionState="ready";frame.ranges.known=true;frame.ranges.gloryKillTarget=10;frame.ranges.lunge=10;frame.ranges.playerOrigin=player;
    const auto* hit=bodyColourFirstHit(camera,frame,0,0);
    requireTarget(hit&&hit->focusGroup=="gut_mech"&&syncAimQuadrant(camera,frame.aim.bounds)==4,"recording reproduces the reported mechanical torso hit in native lower-left aim");
    const auto scopes=syncVisibleAimRequirements(frame);requireTarget(expectedNativeBodyColour(frame,scopes,hit->focusGroup,4)==3,"Carcass lower torso is governed by a real lower-screen animation");
    SyncAimPartMesh old;
    for(const auto& aim:syncDisjointAimRequirements(scopes)){const auto part=syncBuildAimPartMesh(camera,frame.shotBounds,frame.aim,&aim,frame.spheres,true,true,SyncAimDetail::Anatomy);
        old.fill.insert(old.fill.end(),part.fill.begin(),part.fill.end());
        old.quadrantFill.insert(old.quadrantFill.end(),part.quadrantFill.begin(),part.quadrantFill.end());}
    requireTarget(colourCoversPoint(camera,old.quadrantFill,0,0),"v22 anatomical-only interpretation reproduces the incorrect blue torso at the recorded crosshair");
    const auto mesh=syncBuildTargetAimMesh(camera,frame);
    requireTarget(colourCoversPoint(camera,mesh.lowerFill,0,0)&&!colourCoversPoint(camera,mesh.quadrantFill,0,0)&&!colourCoversPoint(camera,mesh.fill,0,0),"reported Carcass crosshair becomes exclusively purple instead of misleading blue");
    const auto checked=auditNativeBodyColours(camera,frame,scopes,mesh);
    std::ofstream atlas("build/carcass-v23-atlas.tsv");atlas.precision(9);
    for(unsigned variant=0;variant<2;++variant){const auto& body=variant?mesh:old;
        for(unsigned channel=0;channel<3;++channel){const auto& points=channel==0?body.fill:channel==1?body.quadrantFill:body.lowerFill;
            for(size_t i=0;i<points.size();i+=3){atlas<<variant<<'\t'<<channel;for(unsigned j=0;j<3;++j){double x,y,w;syncAimProject(camera,points[i+j],x,y,w);atlas<<'\t'<<x<<'\t'<<y;}atlas<<'\n';}
        }
    }
    printf("PASS: live-reported Carcass gut_mech/lower-left regression; %zu triangles checked against all available original rules.\n",checked);
}

struct BodyZoneDeclaration {std::string family,name,state;int action=0;unsigned char position=0,centric[5]{};float rotation=0,ranges[4]{},pitch=0,yaw=0;SyncAimRequirement aim;};
struct BodyZoneMatrix {std::string blueprint,state;float player[12]{},target[12]{};};
static void checkNearestEnemyFamily(SyncTargetFrame frame,const std::string& family);
static void checkAllEnemyBodyZones(bool distanceOnly=false,bool nearestOnly=false) {
    std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-body-templates-v23.tsv");requireTarget(bool(input),"all-enemy native declarations and cached template matrices exist");
    std::vector<BodyZoneDeclaration> declarations;std::vector<BodyZoneMatrix> matrices;std::string line;
    while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream f(line);char kind;f>>kind;
        if(kind=='D'){BodyZoneDeclaration d;unsigned v;size_t count;f>>d.family>>d.name>>d.state>>d.action>>v;d.position=static_cast<unsigned char>(v);
            for(auto& c:d.centric){f>>v;c=static_cast<unsigned char>(v);}f>>d.rotation;for(auto& r:d.ranges)f>>r;f>>d.pitch>>d.yaw>>d.aim.quadrants>>count;
            requireTarget(count<=32,"bounded native declaration groups");while(count--){std::string g;f>>g;d.aim.damageGroups.push_back(g);}declarations.push_back(d);
        }else if(kind=='M'){BodyZoneMatrix m;f>>m.blueprint>>m.state;for(auto& v:m.player)f>>v;for(auto& v:m.target)f>>v;matrices.push_back(m);}else requireTarget(false,"known all-zone fixture record");
        requireTarget(!f.fail(),"all-zone native input parses");
    }
    requireTarget(declarations.size()==224&&matrices.size()>250,"complete reviewed declarations and bounded cached native pose inventory");
    const auto captured=loadCapturedLimbEnemies();std::unordered_set<std::string> families;for(const auto& d:declarations)families.insert(d.family);
    size_t cases=0,triangles=0,nativePoses=0,controlledPoses=0,parts=0,halves=0,coveredRegions=0,emptyRegions=0;
    std::ofstream report(distanceOnly?"build/all-enemy-distance-families-v24.tsv":"build/all-enemy-body-zones-v23.tsv");report<<"family\taction\tside45\theight\trules\tpalette\ttriangles\n";
    for(const auto& family:families){SyncTargetFrame frame;bool realModel=false;
        for(const auto& e:captured)if(e.shape&&e.pose&&e.frame.aim.known&&std::any_of(e.frame.animationRules.begin(),e.frame.animationRules.end(),[&](const auto& r){return r.audit.family==family;})){frame=e.frame;realModel=true;break;}
        if(!realModel){const auto* authored=std::find_if(std::begin(syncAuditedEnemyRules),std::end(syncAuditedEnemyRules),[&](const auto& r){return family==r.family;});
            SyncAimRequirement aim;aim.quadrants=authored->quadrants;std::istringstream names(authored->groups);std::string g;while(std::getline(names,g,'|'))if(!g.empty())aim.damageGroups.push_back(g);frame=displayFixtureFor(*authored,aim);
            frame.facing.axis[0]=frame.facing.axis[4]=frame.facing.axis[8]=1;frame.facing.origin={0,4,0};
        }
        frame.enemyDefinition=family;frame.selectedAnimation.clear();frame.regions.clear();frame.animationRules.clear();frame.chainsawRules.clear();frame.regionState="ready";
        frame.ranges.known=true;frame.ranges.gloryKillTarget=frame.ranges.chainsawTarget=frame.ranges.lunge=12;frame.ranges.ignoreQuickLunge=true;
        size_t sequence=0;
        for(const auto& d:declarations)if(d.family==family){const auto audit=syncAuditEnemyRule(d.name,d.state,d.action,d.aim);requireTarget(audit.matched,"all-zone declaration matches reviewed native identity");
            frame.animationRules.push_back({d.state,d.action,d.aim,audit});if(d.action){frame.hasChainsaw=true;frame.chainsawRules.push_back({d.state,d.aim,true});frame.chainsawRules.back().audit=audit;continue;}
            const auto matrix=std::find_if(matrices.begin(),matrices.end(),[&](const auto& m){return m.blueprint=="syncmelee/"+family&&m.state==d.state;});
            std::vector<unsigned char> decl(0x398),node(0xb0),joints(0x3030);targetPut(decl,0x1d8,d.position);memcpy(decl.data()+0x108,d.centric,5);targetPut(decl,0x144,d.rotation);
            memcpy(decl.data()+0x254,d.ranges,16);targetPut(decl,0x264,d.pitch);targetPut(decl,0x268,d.yaw);targetPut(decl,0x310,d.aim.quadrants);
            // The native bounded reader reads 160 bytes per atomic string.
            // std::string::c_str() alone does not promise that readable span
            // at a heap/page boundary; use fully owned padded native storage.
            std::vector<std::array<char,160>> groupStorage(d.aim.damageGroups.size());std::vector<uintptr_t> strings;
            for(size_t i=0;i<groupStorage.size();++i){const auto& g=d.aim.damageGroups[i];requireTarget(g.size()<160,"native fixture group fits bounded string storage");memcpy(groupStorage[i].data(),g.c_str(),g.size()+1);strings.push_back(reinterpret_cast<uintptr_t>(groupStorage[i].data()));}
            targetPut(decl,0x2a0,SyncPreviewList{reinterpret_cast<uintptr_t>(strings.data()),int(strings.size()),int(strings.size()),0});
            if(matrix!=matrices.end()){memcpy(joints.data(),matrix->player,48);memcpy(joints.data()+48,matrix->target,48);++nativePoses;}
            else { // Controlled orientations cover missing templates; never infer direction from a clip name.
                const float angle=float(sequence++%8)*syncFacingPi/4,c=cosf(angle),s=sinf(angle);const float a[12]{c,-s,0,-2*c,s,c,0,-2*s,0,0,1,0},b[12]{1,0,0,0,0,1,0,0,0,0,1,0};
                memcpy(joints.data(),a,48);memcpy(joints.data()+48,b,48);++controlledPoses;
            }
            // This replay isolates colour partitioning. Uncaptured lunge/filter
            // flags are inactive, and the ordinary native parser still rejects
            // unsupported centric/rotation/pitch/position combinations.
            joints[0x301c]=1;SyncFacingRegion region;
            if(readSyncFacingRegion(WallMemory{},targetAddress(decl),targetAddress(node),targetAddress(joints),0,1,d.state,region)){requireTarget(region.aim.known&&syncSameAimRule(region.aim,d.aim),"all-zone replay retains complete original aim rules");region.audit=audit;frame.regions.push_back(region);}
        }
        if(nearestOnly){checkNearestEnemyFamily(frame,family);continue;}
        checkFamilyDistanceRules(frame);
        if(distanceOnly)continue;
        auto sample=[&](int action,Vec3 player,unsigned side,float height){
            frame.action=action;frame.ranges.playerOrigin=player;
            Vec3 centre{(frame.shotBounds[0]+frame.shotBounds[3])*.5f,(frame.shotBounds[1]+frame.shotBounds[4])*.5f,(frame.shotBounds[2]+frame.shotBounds[5])*.5f};
            Vec3 eye=motionAdd(frame.ranges.playerOrigin,{0,0,1.6f}),forward=subtract(centre,eye);forward=motionScale(forward,1/sqrtf(dot(forward,forward)));
            auto right=cross(forward,{0,0,1});right=motionScale(right,1/sqrtf(dot(right,right)));const auto up=cross(right,forward);GameCamera camera{};
            camera.position[0]=eye.x;camera.position[1]=eye.y;camera.position[2]=eye.z;
            for(const auto& row:{std::make_pair(0u,right),std::make_pair(4u,up),std::make_pair(12u,forward)}){const float xyz[]{row.second.x,row.second.y,row.second.z};for(unsigned j=0;j<3;++j)camera.matrix[row.first+j]=xyz[j];camera.matrix[row.first+3]=-dot(row.second,eye);}
            const auto scopes=syncVisibleAimRequirements(frame);const auto detail=syncTargetAimDetail(frame,scopes);parts+=detail==SyncAimDetail::Anatomy;halves+=detail==SyncAimDetail::Halves;
            const auto mesh=syncBuildTargetAimMesh(camera,frame);const auto checked=auditNativeBodyColours(camera,frame,scopes,mesh);triangles+=checked;++cases;
            report<<family<<'\t'<<action<<'\t'<<side<<'\t'<<height<<'\t'<<scopes.size()<<'\t'<<int(detail)<<'\t'<<checked<<'\n';
        };
        for(int action:{0,1})for(unsigned side=0;side<8;++side)for(float height:{0.f,.5f,2.5f}){
            const float angle=float(side)*syncFacingPi/4;const auto delta=syncRangeTransform(frame.facing,{2.5f*cosf(angle),2.5f*sinf(angle),height});sample(action,motionAdd(frame.facing.origin,delta),side,height);
        }
        for(const auto& region:frame.regions){bool visited=false;const float height=region.minZ>0?std::min(region.maxZ,(region.minZ+region.maxZ)*.5f):0.f;
            frame.ranges.playerOrigin=motionAdd(frame.facing.origin,{0,0,height});SyncRangeVolume volume;
            if(syncRangeBuild(&region,frame.facing,syncTargetReferenceLimits(frame.facing,frame.ranges),false,0,volume,false)){
                const float yaw=atan2f(region.incoming.y,region.incoming.x);
                for(float offset:{0.f,-.3f,.3f}){if(visited)break;for(float distance:{.25f,1.f,2.5f,4.f,6.f}){
                    const auto p=motionAdd(frame.facing.origin,syncRangeTransform(frame.facing,{distance*cosf(yaw+offset*region.halfYaw),distance*sinf(yaw+offset*region.halfYaw),height}));
                    if(syncRangeContains(volume,p)){sample(0,p,99,height);visited=true;break;}
                }}
            }
            if(visited)++coveredRegions;else ++emptyRegions;
        }
    }
    if(nearestOnly){requireTarget(families.size()==25,"nearest audit includes all reviewed families");return;}
    if(distanceOnly){requireTarget(families.size()==25,"focused distance audit includes all 25 reviewed families");return;}
    requireTarget(families.size()==25&&cases>1300&&coveredRegions>100&&nativePoses>100&&controlledPoses>0&&parts>0&&halves>0&&triangles>100000,"all reviewed enemy families cover directional/height/action partitioning and each supported native region");
    printf("PASS: all-enemy body partitions: %zu families, %zu zone/action/height views, %zu cached native pose rows + %zu controlled rows, %zu native regions covered (%zu empty under range cap), %zu original-rule triangle checks.\n",families.size(),cases,nativePoses,controlledPoses,coveredRegions,emptyRegions,triangles);
}
