#pragma once

struct CapturedLimbEnemy {
    std::string definition,model;bool shape=false,mapped=false,source=false,pose=false,identity=false;
    bool geometry=false,geometryComplete=false;size_t expectedRegions=0;
    SyncTargetFrame frame;std::vector<std::pair<std::string,std::vector<short>>> groups;
};
static std::vector<CapturedLimbEnemy> loadCapturedLimbEnemies(const char* path="Native/CollisionViewer/tests/fixtures/sync-limbs-all-models-11.tsv",bool recenter=true) {
    std::ifstream input(path);requireTarget(bool(input),"native limb snapshot exists");
    std::vector<CapturedLimbEnemy> result;std::string line;bool complete=false;
    while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);char kind;size_t id;fields>>kind>>id;
        if(kind=='C'){size_t shapes,maps;fields>>shapes>>maps>>complete;requireTarget(id==result.size()&&shapes==size_t(std::count_if(result.begin(),result.end(),[](const auto& e){return e.shape;}))&&maps==size_t(std::count_if(result.begin(),result.end(),[](const auto& e){return e.mapped;}))&&complete,"native one-shot inventory completed coherently");continue;}
        if(kind=='E'){requireTarget(id==result.size(),"contiguous captured enemy identities");result.emplace_back();auto& enemy=result.back();unsigned token,generation;
            fields>>token>>generation>>enemy.definition>>enemy.model>>enemy.shape>>enemy.mapped>>enemy.source>>enemy.pose;enemy.frame.entity.id=token;enemy.frame.entity.generation=generation;continue;}
        requireTarget(id<result.size(),"captured limb row has a current enemy");auto& enemy=result[id];auto& frame=enemy.frame;
        if(kind=='B')for(auto& f:frame.shotBounds)fields>>f;
        else if(kind=='Q'){fields>>frame.aim.known;for(auto& f:frame.aim.bounds)fields>>f;}
        else if(kind=='P'){fields>>frame.facing.origin.x>>frame.facing.origin.y>>frame.facing.origin.z;for(auto& f:frame.facing.axis)fields>>f;}
        else if(kind=='H')fields>>enemy.geometry>>enemy.geometryComplete>>enemy.expectedRegions;
        else if(kind=='R'){SyncFacingRegion r;std::string declaration;size_t groups;
            fields>>r.name>>r.incoming.x>>r.incoming.y>>r.incoming.z>>r.start.x>>r.start.y>>r.start.z>>r.halfYaw>>r.minXY>>r.maxXY>>r.minZ>>r.maxZ>>r.checkLunge>>r.flatLunge>>r.aim.quadrants>>r.aim.known>>r.priorityKnown>>r.priority>>declaration>>groups;
            requireTarget(groups<=32,"bounded native geometry groups");while(groups--){std::string group;fields>>group;r.aim.damageGroups.push_back(group);}
            r.audit=syncAuditEnemyRule(declaration,r.name,0,r.aim);requireTarget(r.audit.matched,"live geometry retains actual assigned declaration identity");frame.regions.push_back(std::move(r));}
        else if(kind=='G'){std::string name;size_t count;fields>>name>>count;requireTarget(count<=512,"bounded captured native joint group");std::vector<short> joints(count);for(auto& j:joints)fields>>j;enemy.groups.emplace_back(name,std::move(joints));}
        else if(kind=='S'){SyncHitSphere sphere;fields>>sphere.center.x>>sphere.center.y>>sphere.center.z>>sphere.radius>>sphere.jointIndex>>sphere.focusKnown>>sphere.focusGroup>>sphere.group;
            if(sphere.focusGroup=="-")sphere.focusGroup.clear();frame.spheres.push_back(std::move(sphere));}
        else if(kind=='A'){std::string decl;SyncAnimationRule rule;bool matched;size_t count;fields>>decl>>rule.name>>rule.action>>rule.aim.quadrants>>rule.aim.known>>matched>>count;
            requireTarget(count<=32,"bounded captured assigned aim groups");while(count--){std::string group;fields>>group;rule.aim.damageGroups.push_back(group);}
            rule.audit=syncAuditEnemyRule(decl,rule.name,rule.action,rule.aim);requireTarget(matched&&rule.audit.matched,"every captured enemy retains exact native assigned-rule identity");frame.animationRules.push_back(std::move(rule));}
        else if(kind=='V')fields>>enemy.identity;
        else requireTarget(false,"known all-models snapshot row");requireTarget(!fields.fail(),"captured all-models row parses");
    }
    requireTarget(complete&&!result.empty(),"native inventory has a complete final coherence marker");
    if(std::string(path).find("sync-limbs-all-models-11.tsv")!=std::string::npos)requireTarget(result.size()==29,"all 29 original live actors remain in the frozen inventory");
    for(auto& enemy:result){requireTarget(enemy.identity&&enemy.source,"frozen source/target identity was coherent");
        requireTarget(enemy.frame.regions.size()==enemy.expectedRegions,"all captured native regions were parsed");
        for(const auto& sphere:enemy.frame.spheres){std::string group;for(const auto& row:enemy.groups)if(std::find(row.second.begin(),row.second.end(),sphere.jointIndex)!=row.second.end()){group=row.first;break;}
            requireTarget(sphere.focusKnown&&sphere.focusGroup==group,"every actual hurt sphere agrees with independent first-match native joint-group membership");}
        if(enemy.shape&&recenter){auto& frame=enemy.frame;const Vec3 offset{(frame.shotBounds[0]+frame.shotBounds[3])*.5f,(frame.shotBounds[1]+frame.shotBounds[4])*.5f,(frame.shotBounds[2]+frame.shotBounds[5])*.5f};
            for(auto& sphere:frame.spheres)sphere.center=subtract(sphere.center,offset);frame.facing.origin=subtract(frame.facing.origin,offset);
            for(unsigned i=0;i<6;++i){const float xyz[]{offset.x,offset.y,offset.z};frame.shotBounds[i]-=xyz[i%3];frame.aim.bounds[i]-=xyz[i%3];}}
    }return result;
}
static GameCamera limbOrbitCamera(const SyncTargetFrame& frame,unsigned direction,bool close) {
    const float angle=float(direction)*syncFacingPi/4;Vec3 x{frame.facing.axis[0],frame.facing.axis[1],0};x=motionScale(x,1/sqrtf(dot(x,x)));const Vec3 y{-x.y,x.x,0};
    const float extent=std::max({frame.shotBounds[3]-frame.shotBounds[0],frame.shotBounds[4]-frame.shotBounds[1],frame.shotBounds[5]-frame.shotBounds[2]})*.5f;
    const float distance=std::max(2.f,extent*2.6f)*(close?.35f:1.f);const auto eye=motionScale(motionAdd(motionScale(x,cosf(angle)),motionScale(y,sinf(angle))),distance);
    const auto forward=motionScale(eye,-1/distance),right=cross(forward,{0,0,1}),up=cross(right,forward);
    GameCamera camera{};camera.position[0]=eye.x;camera.position[1]=eye.y;camera.position[2]=eye.z;
    for(const auto& row:{std::make_pair(0u,right),std::make_pair(4u,up),std::make_pair(12u,forward)}){const float xyz[]{row.second.x,row.second.y,row.second.z};for(unsigned j=0;j<3;++j)camera.matrix[row.first+j]=xyz[j];camera.matrix[row.first+3]=-dot(row.second,eye);}
    return camera;
}
static bool limbIndependentRay(const GameCamera& camera,const SyncTargetFrame& frame,const SyncAimRequirement& aim,double px,double py) {
    Vec3 point{};if(!syncAimUnproject(camera,px,py,1,point))return false;const Vec3 eye{camera.position[0],camera.position[1],camera.position[2]};
    const auto ray=subtract(point,eye);double nearest=DBL_MAX;const SyncHitSphere* first=nullptr;
    for(const auto& sphere:frame.spheres){const auto delta=subtract(eye,sphere.center);const double a=dot(ray,ray),b=dot(delta,ray),c=dot(delta,delta)-double(sphere.radius)*sphere.radius;
        const double discriminant=b*b-a*c;if(discriminant<0||a<=0)continue;const double root=sqrt(discriminant),entry=(-b-root)/a,exit=(-b+root)/a,t=entry>=0?entry:exit;
        if(t>=0&&t<nearest){nearest=t;first=&sphere;}}
    if(!first||std::find(aim.damageGroups.begin(),aim.damageGroups.end(),first->focusGroup)==aim.damageGroups.end())return false;
    if(syncAimUnrestricted(aim.quadrants))return true;double cx,cy,w;const auto* b=frame.aim.bounds;
    if(!syncAimProject(camera,{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},cx,cy,w))return false;
    const unsigned quadrant=py>cy?(px<cx?1:2):(px<cx?4:8);return (aim.quadrants&quadrant)!=0;
}
static bool limbVisibleCore(const GameCamera& camera,const SyncTargetFrame& frame,const SyncAimRequirement& aim) {
    const Vec3 eye{camera.position[0],camera.position[1],camera.position[2]};
    for(const auto& sphere:frame.spheres)if(dot(subtract(eye,sphere.center),subtract(eye,sphere.center))<=sphere.radius*sphere.radius)return false;
    SyncAimPartRect rect;double front;if(!syncAimPartRect(camera,{frame.shotBounds[0],frame.shotBounds[1],frame.shotBounds[2]},{frame.shotBounds[3],frame.shotBounds[4],frame.shotBounds[5]},rect,front))return false;
    const double dx=(std::min(1.,rect.x1)-std::max(-1.,rect.x0))/18,dy=(std::min(1.,rect.y1)-std::max(-1.,rect.y0))/24;
    const auto* m=camera.matrix;const Vec3 right{m[0],m[1],m[2]},up{m[4],m[5],m[6]};
    for(const auto& sphere:frame.spheres)if(std::find(aim.damageGroups.begin(),aim.damageGroups.end(),sphere.focusGroup)!=aim.damageGroups.end())for(float x:{-.45f,0.f,.45f})for(float y:{-.45f,0.f,.45f}){
        const auto sample=motionAdd(sphere.center,motionScale(motionAdd(motionScale(right,x),motionScale(up,y)),sphere.radius));double px,py,w;
        if(!syncAimProject(camera,sample,px,py,w)||fabs(px)+dx>=.99||fabs(py)+dy>=.99)continue;
        bool visible=true;for(double a:{-1.,0.,1.})for(double b:{-1.,0.,1.})visible&=limbIndependentRay(camera,frame,aim,px+a*dx,py+b*dy);
        if(visible)return true;
    }return false;
}
static void checkAllCapturedLimbViews() {
    const auto enemies=loadCapturedLimbEnemies();size_t cases=0,missing=0,visible=0;std::unordered_set<std::string> families,models;
    std::ofstream report("build/sync-limbs-v18-multiview.tsv"),vertices("build/sync-limbs-v18-atlas-vertices.tsv");report<<"enemy\tmodel\tstate\taction\tside45\tclose\tkind\tknown\ttriangles\tvisibleCore\n";vertices.precision(8);
    for(size_t id=0;id<enemies.size();++id){const auto& enemy=enemies[id];const auto& frame=enemy.frame;
        for(const auto& rule:frame.animationRules){const auto kind=syncBodyAimKind(&rule.aim,frame.spheres);
            if(kind==SyncBodyAimKind::None||kind==SyncBodyAimKind::AllCurrentGroups){const auto absent=syncBuildAimPartMesh(GameCamera{},frame.shotBounds,frame.aim,&rule.aim,frame.spheres);
                requireTarget(!absent.required&&absent.fill.empty()&&absent.outline.empty(),"every captured orientation-only or whole-body animation avoids invented limb highlighting");continue;}
            requireTarget(enemy.shape&&enemy.mapped&&enemy.pose&&frame.aim.known,"all captured body-specific models have real coherent shot poses, aim bounds and group maps");
            requireTarget(kind==SyncBodyAimKind::Specific,"required native groups must be present on the actual body-specific model");families.insert(rule.audit.family);models.insert(enemy.model);
            for(unsigned side=0;side<8;++side)for(bool close:{false,true}){const auto camera=limbOrbitCamera(frame,side,close);const auto mesh=syncBuildAimPartMesh(camera,frame.shotBounds,frame.aim,&rule.aim,frame.spheres);
                const bool core=limbVisibleCore(camera,frame,rule.aim);visible+=core;missing+=core&&mesh.fill.empty();++cases;
                report<<enemy.definition<<'\t'<<enemy.model<<'\t'<<rule.name<<'\t'<<rule.action<<'\t'<<side<<'\t'<<close<<'\t'<<int(kind)<<'\t'<<mesh.known<<'\t'<<mesh.fill.size()/3<<'\t'<<core<<'\n';
                for(size_t i=0;i<mesh.fill.size();i+=3){double x[3]{},y[3]{},w;
                    for(unsigned j=0;j<3;++j)requireTarget(syncAimProject(camera,mesh.fill[i+j],x[j],y[j],w),"every actual-model patch vertex projects from every tested side");
                    for(const auto& bary:{std::array<double,3>{1./3,1./3,1./3},std::array<double,3>{.7,.15,.15},std::array<double,3>{.15,.7,.15},std::array<double,3>{.15,.15,.7}}){
                        const double px=x[0]*bary[0]+x[1]*bary[1]+x[2]*bary[2],py=y[0]*bary[0]+y[1]*bary[1]+y[2]*bary[2];
                        const bool pass=limbIndependentRay(camera,frame,rule.aim,px,py);
                        if(!pass){std::printf("BAD LIMB %s %s side=%u close=%d tri=%zu p=(%.9f,%.9f)\n",enemy.definition.c_str(),rule.name.c_str(),side,close,i/3,px,py);std::fflush(stdout);}
                        requireTarget(pass,"multi-side patch interiors hit a required native group first and satisfy its native quadrant");}
                    if(rule.action==0&&(rule.name=="kill_front_head"||rule.name=="kill_front_head_01"||rule.name=="kill_front_leftarm"||rule.name=="kill_front_rightarm")){
                        vertices<<id<<'\t'<<side<<'\t'<<close<<'\t'<<rule.name;for(unsigned j=0;j<3;++j)vertices<<'\t'<<x[j]<<'\t'<<y[j];vertices<<'\n';}}
                if(!mesh.known)std::printf("UNKNOWN LIMB %s %s side=%u close=%d\n",enemy.definition.c_str(),rule.name.c_str(),side,close);
                requireTarget(mesh.known,"every captured body-specific model has a known projection on all eight sides at both tested distances");
                if(core&&mesh.fill.empty())std::printf("MISSING LIMB %s %s side=%u close=%d\n",enemy.definition.c_str(),rule.name.c_str(),side,close);
            }
        }
    }
    std::printf("Actual native limb multi-view: %zu cases, %zu visible cores, %zu empty visible cores, %zu families, %zu model variants.\n",cases,visible,missing,families.size(),models.size());
    requireTarget(families.size()==8&&models.size()==12&&cases>=600,"all eight body-guided families and all twelve spawned model variants are tested from eight real-pose directions at two distances");
    requireTarget(missing==0,"no clearly visible required part disappears from an actual native model's mask at any tested side");
}
static void checkLimbUnassignedAndNearPlane() {
    GameCamera camera{};camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    std::vector<SyncHitSphere> spheres={{{0,4,1},.3f,"head",1,true,"head"},{{-.8f,4,0},.3f,"aux",99,true,""},{{0,4,0},.4f,"chest",2,true,"chest"}};
    float bounds[]{-1.1f,3.5f,-.4f,.4f,4.5f,1.3f};SyncAimRequirement rule;rule.damageGroups={"head"};SyncAimFrame aim;
    requireTarget(syncBodyAimKind(&rule,spheres)==SyncBodyAimKind::Specific,"a known unmapped auxiliary joint cannot invalidate the required limb's mapping");
    requireTarget(!syncBuildAimPartMesh(camera,bounds,aim,&rule,spheres).fill.empty(),"a visible head remains highlighted beside a known no-group auxiliary sphere");
    spheres[1].center={0,3,1};spheres[1].radius=.6f;
    requireTarget(syncBuildAimPartMesh(camera,bounds,aim,&rule,spheres).fill.empty(),"known no-group spheres still occlude required parts instead of being dropped as unknown");
    spheres[1].focusKnown=false;requireTarget(!syncBuildAimPartMesh(camera,bounds,aim,&rule,spheres).known,"a genuinely unreadable group map still cannot create invented highlights");
    spheres={{{.15f,.5f,0},.16f,"head",1,true,"head"},{{-.5f,-.2f,0},.1f,"chest",2,true,"chest"}};syncShotBounds(spheres,bounds);
    requireTarget(!syncBuildAimPartMesh(camera,bounds,aim,&rule,spheres).fill.empty(),"a visible required part survives when the box crosses the camera near plane and another sphere is behind the eye");
    rule.damageGroups={"head","chest"};spheres.push_back({{0,2,0},.1f,"aux",-1,true,""});
    requireTarget(syncBodyAimKind(&rule,spheres)==SyncBodyAimKind::AllCurrentGroups,"a known no-group helper sphere does not turn whole-body rules into fake anatomical choices");
}

// Lower-screen rules are legitimate leg-related animations even when they do
// not require a named leg joint. Verify their real physics-centre tiles too.
static void checkCapturedLowerAimViews() {
    const auto enemies=loadCapturedLimbEnemies();size_t cases=0,visible=0,soldierStates=0;
    std::ofstream vertices("build/sync-legs-v19-atlas-vertices.tsv");vertices.precision(9);
    for(size_t id=0;id<enemies.size();++id){const auto& frame=enemies[id].frame;if(!enemies[id].pose||!frame.aim.known)continue;
        for(const auto& rule:frame.animationRules)if(rule.action==0&&rule.aim.quadrants==12&&rule.aim.damageGroups.empty()){
            if(enemies[id].definition.find("soldier")!=std::string::npos)++soldierStates;
            for(unsigned side=0;side<8;++side)for(bool close:{false,true}){++cases;const auto camera=limbOrbitCamera(frame,side,close);std::vector<Vertex> fill;
                appendSyncAimBox(camera,frame.shotBounds,frame.aim,&rule.aim,fill,false);
                double cx,cy,w;const auto* b=frame.aim.bounds;const bool projected=syncAimProject(camera,{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},cx,cy,w);
                SyncAimPartRect rect;double front;const bool hasBox=syncAimPartRect(camera,{frame.shotBounds[0],frame.shotBounds[1],frame.shotBounds[2]},{frame.shotBounds[3],frame.shotBounds[4],frame.shotBounds[5]},rect,front);
                if(projected&&hasBox&&std::max(-1.,rect.y0)<std::min({1.,rect.y1,cy})&&std::max(-1.,rect.x0)<std::min(1.,rect.x1)){
                    ++visible;requireTarget(!fill.empty(),"actual lower-half aim tiles remain visible from all sides and at close range when their allowed half is on screen");}
                for(size_t i=0;i<fill.size();i+=3){vertices<<id<<'\t'<<side<<'\t'<<close<<'\t'<<rule.name;
                    for(unsigned j=0;j<3;++j){double x=0,y=0,depth=0;requireTarget(projected&&syncAimProject(camera,fill[i+j].position,x,y,depth)&&y<=cy+.00001,"lower aim patches stay below the real physics centre instead of guessing anatomical legs");
                        requireTarget(fill[i+j].color[0]<.5f,"quadrant-only leg rules retain the distinct quadrant colour");vertices<<'\t'<<x<<'\t'<<y;}vertices<<'\n';}}}}
    requireTarget(cases>500&&visible>300&&soldierStates==8,"actual capture covers lower-aim GK rules across all available models, including four directions each for Blaster and Shield Soldier");
    requireTarget(syncBodyGroupLabel({"left_upper_leg","right_lower_leg","left_foot"})=="left leg / right leg / left foot","explicit native leg and foot groups receive accurate body labels");
    printf("PASS: actual lower-half aim masks: %zu views, %zu visible cases, Soldier lower states %zu.\n",cases,visible,soldierStates);
}
