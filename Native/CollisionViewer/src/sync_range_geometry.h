#pragma once

static bool syncRangeFinite(Vec3 p) {return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&fabsf(p.x)<1e6f&&fabsf(p.y)<1e6f&&fabsf(p.z)<1e6f;}

struct SyncRangeLimits {
    bool known=false,ignoreQuickLunge=false;
    float gloryKillTarget=0,chainsawTarget=0,lunge=0,modifier=0;
    Vec3 eyeOffset{},viewTargetOffset{},playerOrigin{},aimOrigin{};
    float modelViewOffset=0;
    std::string detail="Range data unavailable";
};
struct SyncRangePlane {Vec3 n{};double d=0;};
struct SyncRangeBall {Vec3 center{};float radius=0;bool horizontal=false;};
struct SyncRangeVolume {
    std::string name;
    Vec3 interior{};
    std::vector<SyncRangePlane> planes;
    std::vector<SyncRangeBall> balls;
    bool current=false;
    unsigned facingPlanes=0;
    bool wideFacing=false;Vec3 facingOrigin{};float facingCenter=0,facingHalf=0;
};
struct SyncPlayerFootprint {bool known=false;Vec3 minimum{},maximum{};};
static float syncFootprintRadius(const SyncPlayerFootprint& body){return .5f*hypotf(body.maximum.x-body.minimum.x,body.maximum.y-body.minimum.y);}
// A conservative horizontal clearance, not a change to the engine's
// origin-based activation rule. Bounds come from the live physics collider.
static SyncRangeVolume syncFootprintVolume(SyncRangeVolume volume,const SyncPlayerFootprint& body,bool excluded) {
    const Vec3 center{(body.minimum.x+body.maximum.x)*.5f,(body.minimum.y+body.maximum.y)*.5f,0};
    const Vec3 half{(body.maximum.x-body.minimum.x)*.5f,(body.maximum.y-body.minimum.y)*.5f,0};
    for(auto& plane:volume.planes)plane.d+=dot(plane.n,center)+(excluded?-1:1)*(fabs(plane.n.x)*half.x+fabs(plane.n.y)*half.y);
    for(auto& ball:volume.balls){ball.center=subtract(ball.center,center);ball.radius+=(excluded?1:-1)*syncFootprintRadius(body);}
    return volume;
}
// A reference sector is anchored in the enemy's frame, assuming the player's
// body faces the enemy origin. The true current-aim volume is retained for
// validation; never overwrite its sampled aim point/model-to-view offset.
static SyncRangeLimits syncTargetReferenceLimits(const SyncFacingPose& pose,const SyncRangeLimits& limits) {
    auto reference=limits;reference.aimOrigin=pose.origin;reference.modelViewOffset=0;
    reference.eyeOffset.x=0;reference.eyeOffset.y=0;return reference;
}
static Vec3 syncRangeTransform(const SyncFacingPose& pose,Vec3 p) {
    return {pose.axis[0]*p.x+pose.axis[3]*p.y+pose.axis[6]*p.z,
            pose.axis[1]*p.x+pose.axis[4]*p.y+pose.axis[7]*p.z,
            pose.axis[2]*p.x+pose.axis[5]*p.y+pose.axis[8]*p.z};
}
static bool syncRangeContains(const SyncRangeVolume& v,Vec3 p,double margin=0) {
    if(v.wideFacing){const auto d=subtract(p,v.facingOrigin);if(!syncFacingContains(v.facingCenter,v.facingHalf,atan2f(d.y,d.x)))return false;}
    for(const auto& plane:v.planes)if(double(dot(plane.n,p))+plane.d>-margin)return false;
    for(const auto& ball:v.balls) {
        const auto d=subtract(p,ball.center);const double allowedRadius=ball.radius-margin;
        if(!std::isfinite(allowedRadius)||allowedRadius<=0||double(d.x)*d.x+double(d.y)*d.y+(ball.horizontal?0:double(d.z)*d.z)>allowedRadius*allowedRadius)return false;
    }
    return true;
}
static bool syncRangeRay(const SyncRangeVolume& v,Vec3 direction,Vec3& point) {
    double distance=1000;
    for(const auto& plane:v.planes) {
        const double denominator=dot(plane.n,direction);
        if(denominator>1e-10)distance=std::min(distance,-(dot(plane.n,v.interior)+plane.d)/denominator);
    }
    for(const auto& ball:v.balls) {
        const auto delta=subtract(v.interior,ball.center);
        const double a=double(direction.x)*direction.x+double(direction.y)*direction.y+(ball.horizontal?0:double(direction.z)*direction.z);
        if(a<1e-12)continue;
        const double b=double(delta.x)*direction.x+double(delta.y)*direction.y+(ball.horizontal?0:double(delta.z)*direction.z);
        const double c=double(delta.x)*delta.x+double(delta.y)*delta.y+(ball.horizontal?0:double(delta.z)*delta.z)-double(ball.radius)*ball.radius;
        const double discriminant=b*b-a*c;if(discriminant<0)return false;
        distance=std::min(distance,(-b+sqrt(discriminant))/a);
    }
    if(!std::isfinite(distance)||distance<=0||distance>200)return false;
    point=motionAdd(v.interior,motionScale(direction,float(distance)));return syncRangeFinite(point)&&syncRangeContains(v,point,-.0001);
}
static bool syncRangeBuild(const SyncFacingRegion* region,const SyncFacingPose& pose,const SyncRangeLimits& limits,bool chainsaw,
                            float incomingYaw,SyncRangeVolume& result,bool requireInterior=true) {
    result={};if(!limits.known||!syncFacingOrthonormal(pose.axis))return false;
    const auto targetView=motionAdd(pose.origin,syncRangeTransform(pose,limits.viewTargetOffset));
    const auto targetFeet= subtract(targetView,limits.eyeOffset);
    const float targetRadius=chainsaw?limits.chainsawTarget:limits.gloryKillTarget;
    if(!std::isfinite(targetRadius)||targetRadius<=0||targetRadius>200)return false;
    result.balls.push_back({targetFeet,targetRadius,false});
    std::vector<Vec3> seeds;
    if(chainsaw) {
        result.name="Chainsaw range";result.current=true;seeds.push_back(targetFeet);
        if(!limits.ignoreQuickLunge)result.balls.push_back({pose.origin,limits.lunge,false});
    } else {
        if(!region||region->minXY!=0||region->minZ<0||region->minZ>region->maxZ||region->maxZ<=0||region->maxXY<=0||region->halfYaw<=0||region->halfYaw>syncFacingPi)return false;
        result.name=region->name;
        const auto start=motionAdd(pose.origin,syncRangeTransform(pose,region->start));
        result.balls.push_back({start,region->maxXY,true});
        if(region->checkLunge)result.balls.push_back({pose.origin,limits.lunge,region->flatLunge});
        const float nativeCenter=syncFacingYaw(pose,region->incoming),center=nativeCenter-limits.modelViewOffset,a=center-region->halfYaw,b=center+region->halfYaw;
        result.current=syncFacingContains(nativeCenter,region->halfYaw,incomingYaw);
        auto plane=[&](Vec3 n,Vec3 p){result.planes.push_back({n,-double(dot(n,p))});};
        if(region->halfYaw<syncFacingPi-.00001f){
            if(region->halfYaw<=syncFacingPi/2){plane({sinf(a),-cosf(a),0},limits.aimOrigin);plane({-sinf(b),cosf(b),0},limits.aimOrigin);result.facingPlanes=2;}
            else {result.wideFacing=true;result.facingOrigin=limits.aimOrigin;result.facingCenter=center;result.facingHalf=region->halfYaw;}}
        plane({0,0,1},{0,0,pose.origin.z+region->maxZ});plane({0,0,-1},{0,0,pose.origin.z-region->maxZ});
        if(region->minZ>0){if(limits.playerOrigin.z>=pose.origin.z)plane({0,0,-1},{0,0,pose.origin.z+region->minZ});else plane({0,0,1},{0,0,pose.origin.z-region->minZ});}
        const Vec3 forward{cosf(center),sinf(center),0};
        Vec3 middle=motionAdd(limits.aimOrigin,motionScale(forward,std::min(1.f,region->maxXY*.25f)));middle.z=pose.origin.z-std::clamp(limits.eyeOffset.z*.5f,-region->maxZ*.5f,region->maxZ*.5f);
        seeds={middle,motionScale(motionAdd(start,targetFeet),.5f),motionScale(motionAdd(start,pose.origin),.5f),start,targetFeet};
    }
    if(!requireInterior)return true;
    for(Vec3 seed:seeds)if(syncRangeFinite(seed)&&syncRangeContains(result,seed,.001)) {result.interior=seed;return true;}
    return false;
}
static void syncRangeWedge(SyncRangeVolume& volume,Vec3 origin,float center,float half) {
    for(auto n:std::array<Vec3,2>{{{sinf(center-half),-cosf(center-half),0},{-sinf(center+half),cosf(center+half),0}}})volume.planes.push_back({n,-double(dot(n,origin))});
}
static std::vector<SyncRangeVolume> syncRangeConvexParts(const SyncRangeVolume& volume) {
    if(!volume.wideFacing)return {volume};std::vector<SyncRangeVolume> result;
    for(float sign:{-1.f,1.f}){auto part=volume;part.wideFacing=false;syncRangeWedge(part,volume.facingOrigin,volume.facingCenter+sign*volume.facingHalf*.5f,volume.facingHalf*.5f);result.push_back(std::move(part));}return result;
}
static SyncRangeVolume syncRangeOutsideWide(const SyncRangeVolume& volume) {
    auto excluded=volume;excluded.wideFacing=false;excluded.name="outside selected facing";
    syncRangeWedge(excluded,volume.facingOrigin,volume.facingCenter+syncFacingPi,syncFacingPi-volume.facingHalf);return excluded;
}
static std::array<float,4> syncRangeColor(const SyncRangeVolume& v,bool lines,bool chainsaw) {
    static constexpr float palette[][3]{{.3f,.85f,1},{1,.65f,.2f},{.65f,.4f,1},{.3f,1,.5f},{1,.3f,.55f},{1,.95f,.3f}};
    uint32_t hash=2166136261u;for(unsigned char c:v.name)hash=(hash^c)*16777619u;
    std::array<float,4> color{palette[hash%6][0],palette[hash%6][1],palette[hash%6][2],lines?(v.current?.9f:.45f):(v.current?.09f:.035f)};
    if(chainsaw)color={1,.85f,.4f,lines?.8f:.055f};return color;
}
static void appendSyncRangeVolume(const GameCamera& camera,const SyncRangeVolume& volume,std::vector<Vertex>& output,bool lines,bool chainsaw) {
    const auto color=syncRangeColor(volume,lines,chainsaw);
    auto append=[&](Vec3 p){Vertex v{};v.position=p;memcpy(v.color,color.data(),16);output.push_back(v);};
    if(lines) {
        for(unsigned plane=0;plane<3;++plane)for(unsigned i=0;i<64;++i) {
            Vec3 points[2];bool valid=true;
            for(unsigned j=0;j<2;++j){const float a=2*syncFacingPi*(i+j)/64,c=cosf(a),s=sinf(a);valid&=syncRangeRay(volume,plane==0?Vec3{0,c,s}:plane==1?Vec3{s,0,c}:Vec3{c,s,0},points[j]);}
            if(valid){append(points[0]);append(points[1]);}
        }
        return;
    }
    constexpr unsigned longitude=32,latitude=16;
    std::array<Vec3,(longitude+1)*(latitude+1)> points{};
    for(unsigned y=0;y<=latitude;++y)for(unsigned x=0;x<=longitude;++x) {
        const float a=2*syncFacingPi*x/longitude,b=syncFacingPi*y/latitude;
        if(!syncRangeRay(volume,{sinf(b)*cosf(a),sinf(b)*sinf(a),cosf(b)},points[y*(longitude+1)+x]))return;
    }
    Vec3 eye{},direction{};if(!inspectorRay(camera,eye,direction))return;
    const bool inside=syncRangeContains(volume,eye);
    auto triangle=[&](Vec3 a,Vec3 b,Vec3 c){const auto normal=cross(subtract(b,a),subtract(c,a));const float facing=dot(normal,subtract(eye,a));if(inside?facing>0:facing<0)return;append(a);append(b);append(c);};
    for(unsigned y=0;y<latitude;++y)for(unsigned x=0;x<longitude;++x){const auto a=points[y*(longitude+1)+x],b=points[y*(longitude+1)+x+1],c=points[(y+1)*(longitude+1)+x+1],d=points[(y+1)*(longitude+1)+x];if(y)triangle(a,c,b);if(y+1<latitude)triangle(a,d,c);}
}
