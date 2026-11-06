#pragma once

// A horizontal section at the player's physics origin, not a terrain trace.
// Start with a circumscribed square and clip against inscribed circle polygons:
// every displayed point remains within the native convex constraints.
static std::vector<Vec3> syncGuideSection(const SyncRangeVolume& volume,float z) {
    if(volume.wideFacing)return {}; // Nonconvex sectors use syncExclusiveBuild + opposite-wedge subtraction.
    std::vector<Vec3> polygon;
    if(volume.balls.empty()||!std::isfinite(z))return polygon;
    const auto& first=volume.balls.front();const float r=first.radius;
    polygon={{first.center.x-r,first.center.y-r,z},{first.center.x+r,first.center.y-r,z},
             {first.center.x+r,first.center.y+r,z},{first.center.x-r,first.center.y+r,z}};
    auto clip=[&](Vec3 normal,double d) {
        std::vector<Vec3> next;if(polygon.empty())return;
        Vec3 previous=polygon.back();double before=dot(normal,previous)+d;
        for(const auto& point:polygon){const double after=dot(normal,point)+d;
            if((before<=0)!=(after<=0)){const float t=float(before/(before-after));next.push_back(motionAdd(previous,motionScale(subtract(point,previous),t)));}
            if(after<=0)next.push_back(point);previous=point;before=after;}
        polygon=std::move(next);
    };
    for(const auto& plane:volume.planes)clip(plane.n,plane.d);
    constexpr unsigned sides=96;
    for(const auto& ball:volume.balls){const double dz=ball.horizontal?0:double(z)-ball.center.z;
        const double radius2=double(ball.radius)*ball.radius-dz*dz;
        if(radius2<=0)return {};
        const float apothem=float(sqrt(radius2)*cos(syncFacingPi/sides));
        for(unsigned i=0;i<sides&&!polygon.empty();++i){const float a=2*syncFacingPi*(i+.5f)/sides;Vec3 n{cosf(a),sinf(a),0};clip(n,-double(dot(n,ball.center))-apothem);}
    }
    return polygon;
}
struct SyncGuideStatus {
    bool inside=false,direction=false,height=false,cameraRange=false,startRange=false,lunge=false;
    float cameraDistance=0,cameraLimit=0;
};
// Exact interval on the player's current horizontal approach at fixed foot
// height. Express its endpoints in the same camera-target metric as ball 0.
static bool syncGuideApproach(const SyncRangeVolume& volume,Vec3 player,float& nearDistance,float& farDistance) {
    if(volume.wideFacing)return false; // Use the exclusive polygon intervals for nonconvex sectors.
    if(volume.balls.empty())return false;
    const auto center=volume.balls.front().center;const Vec3 origin{center.x,center.y,player.z};auto direction=subtract(player,origin);
    const float length=sqrtf(dot(direction,direction));if(length<.0001f)return false;direction=motionScale(direction,1/length);
    double low=0,high=200;
    for(const auto& plane:volume.planes){const double start=dot(plane.n,origin)+plane.d,speed=dot(plane.n,direction);
        if(fabs(speed)<1e-10){if(start>0)return false;continue;}
        const double end=-start/speed;if(speed>0)high=std::min(high,end);else low=std::max(low,end);}
    for(const auto& ball:volume.balls){auto delta=subtract(origin,ball.center);if(ball.horizontal)delta.z=0;
        const double b=dot(delta,direction),c=double(dot(delta,delta))-double(ball.radius)*ball.radius,discriminant=b*b-c;
        if(discriminant<0)return false;const double root=sqrt(discriminant);low=std::max(low,-b-root);high=std::min(high,-b+root);}
    if(!std::isfinite(low)||!std::isfinite(high)||high-low<.0001)return false;
    const double dz=double(player.z)-center.z;nearDistance=float(sqrt(low*low+dz*dz));farDistance=float(sqrt(high*high+dz*dz));return true;
}
static SyncGuideStatus syncGuideStatus(const SyncRangeVolume& volume,Vec3 player,bool chainsaw) {
    SyncGuideStatus status;status.inside=syncRangeContains(volume,player);
    for(size_t i=0;i<volume.planes.size();++i)if(dot(volume.planes[i].n,player)+volume.planes[i].d>0){if(!chainsaw&&i<volume.facingPlanes)status.direction=true;else status.height=true;}
    if(volume.wideFacing&&!syncFacingContains(volume.facingCenter,volume.facingHalf,atan2f(player.y-volume.facingOrigin.y,player.x-volume.facingOrigin.x)))status.direction=true;
    for(size_t i=0;i<volume.balls.size();++i){const auto& ball=volume.balls[i];auto delta=subtract(player,ball.center);if(ball.horizontal)delta.z=0;
        const float distance=sqrtf(dot(delta,delta));if(i==0){status.cameraDistance=distance;status.cameraLimit=ball.radius;}
        if(distance>ball.radius){if(i==0)status.cameraRange=true;else if(!chainsaw&&i==1)status.startRange=true;else status.lunge=true;}}
    return status;
}
// Dykstra projection finds a nearby feasible point using the same native
// halfspaces and balls. Never display a destination unless it passes them all.
static bool syncGuideDestination(const SyncRangeVolume& volume,Vec3 player,Vec3& destination) {
    const size_t count=volume.planes.size()+volume.balls.size();if(!count||!syncRangeFinite(player))return false;
    std::vector<Vec3> corrections(count);destination=player;
    for(unsigned iteration=0;iteration<96;++iteration){const auto before=destination;
        for(size_t i=0;i<count;++i){const auto input=motionAdd(destination,corrections[i]);auto projected=input;
            if(i<volume.planes.size()){const auto& plane=volume.planes[i];const double excess=dot(plane.n,input)+plane.d;
                const float normal2=dot(plane.n,plane.n);if(excess>0&&normal2>0)projected=subtract(input,motionScale(plane.n,float(excess/normal2)));}
            else {const auto& ball=volume.balls[i-volume.planes.size()];auto delta=subtract(input,ball.center);if(ball.horizontal)delta.z=0;
                const float distance=sqrtf(dot(delta,delta));if(distance>ball.radius)projected=subtract(input,motionScale(delta,1-ball.radius/distance));}
            corrections[i]=subtract(input,projected);destination=projected;}
        const auto change=subtract(destination,before);if(dot(change,change)<1e-10f&&syncRangeContains(volume,destination,-.0001))return true;
    }
    return syncRangeContains(volume,destination,-.0001)&&syncRangeFinite(destination);
}
static void appendSyncPositionGuide(const SyncRangeVolume& volume,Vec3 player,std::vector<Vertex>& output,bool lines,bool selectedOnlyObserved=false) {
    const auto section=syncGuideSection(volume,player.z);const bool inside=syncRangeContains(volume,player);
    const std::array<float,4> color=!inside?std::array<float,4>{.3f,.85f,1,lines?.95f:.1f}:
        selectedOnlyObserved?std::array<float,4>{.3f,1,.5f,lines?.95f:.1f}:std::array<float,4>{1,.65f,.15f,lines?.95f:.1f};
    auto append=[&](Vec3 point,const std::array<float,4>& tint){Vertex vertex{};vertex.position=point;memcpy(vertex.color,tint.data(),16);output.push_back(vertex);};
    if(section.size()>=3){if(lines){for(size_t i=0;i<section.size();++i){append(section[i],color);append(section[(i+1)%section.size()],color);}}
        else for(size_t i=1;i+1<section.size();++i){append(section[0],color);append(section[i],color);append(section[i+1],color);}}
    if(!lines)return;
    const std::array<float,4> marker{1,.85f,.25f,1};
    auto crossAt=[&](Vec3 p){for(Vec3 axis:std::vector<Vec3>{{.22f,0,0},{0,.22f,0},{0,0,.22f}}){append(subtract(p,axis),marker);append(motionAdd(p,axis),marker);}};
    crossAt(player);Vec3 destination{};
    if(!inside&&syncGuideDestination(volume,player,destination)){append(player,marker);append(destination,marker);crossAt(destination);}
}
