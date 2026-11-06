#pragma once

struct SyncExclusivePoint { double x=0,y=0; };
using SyncExclusivePolygon=std::vector<SyncExclusivePoint>;
struct SyncExclusiveBudget {
    size_t work=0;
    bool exhausted=false;
    bool take(size_t count=1){work+=count;return !(exhausted=exhausted||work>500000);}
};
static double syncExclusiveArea(const SyncExclusivePolygon& polygon) {
    if(polygon.size()<3)return 0;double area=0;const auto origin=polygon.front();
    for(size_t i=1;i+1<polygon.size();++i)area+=(polygon[i].x-origin.x)*(polygon[i+1].y-origin.y)-(polygon[i].y-origin.y)*(polygon[i+1].x-origin.x);
    return area*.5;
}
static SyncExclusivePolygon syncExclusiveClip(const SyncExclusivePolygon& polygon,double nx,double ny,double d,SyncExclusiveBudget& budget) {
    SyncExclusivePolygon output;if(polygon.empty()||!budget.take(polygon.size()))return output;
    auto previous=polygon.back();double before=nx*previous.x+ny*previous.y+d;
    for(auto point:polygon){const double after=nx*point.x+ny*point.y+d;
        if((before<=0)!=(after<=0)){const double t=before/(before-after);output.push_back({previous.x+(point.x-previous.x)*t,previous.y+(point.y-previous.y)*t});}
        if(after<=0)output.push_back(point);previous=point;before=after;}
    if(output.size()>512){budget.exhausted=true;return {};}
    if(syncExclusiveArea(output)<1e-8)return {};return output;
}
static void syncExclusiveEdge(SyncExclusivePoint a,SyncExclusivePoint b,double& nx,double& ny,double& d) {
    nx=b.y-a.y;ny=a.x-b.x;d=-(nx*a.x+ny*a.y);
}
static bool syncExclusiveInside(const SyncExclusivePolygon& polygon,SyncExclusivePoint point,double tolerance=0) {
    if(polygon.size()<3)return false;
    for(size_t i=0;i<polygon.size();++i){double nx,ny,d;syncExclusiveEdge(polygon[i],polygon[(i+1)%polygon.size()],nx,ny,d);
        if(nx*point.x+ny*point.y+d>tolerance*sqrt(nx*nx+ny*ny))return false;}return true;
}
// Selected circles are inscribed; excluded circles are circumscribed and
// slightly expanded. A competitor's approximation must NEVER leave a false
// exclusive sliver near its curved boundary. All native Z/range/facing gates
// participate in these sections, including competitors outside their range.
static SyncExclusivePolygon syncExclusiveSection(const SyncRangeVolume& volume,float z,bool outer,double clearance,SyncExclusiveBudget& budget) {
    if(volume.balls.empty()||!std::isfinite(z))return {};
    const auto& first=volume.balls.front();const double extent=first.radius+clearance;
    SyncExclusivePolygon polygon{{first.center.x-extent,first.center.y-extent},{first.center.x+extent,first.center.y-extent},
        {first.center.x+extent,first.center.y+extent},{first.center.x-extent,first.center.y+extent}};
    for(const auto& plane:volume.planes){const double length=sqrt(double(dot(plane.n,plane.n)));
        polygon=syncExclusiveClip(polygon,plane.n.x,plane.n.y,plane.d+double(plane.n.z)*z+(outer?-clearance:clearance)*length,budget);if(polygon.empty())return {};}
    constexpr unsigned sides=96;
    for(const auto& ball:volume.balls){const double allowedRadius=ball.radius+(outer?clearance:-clearance),dz=ball.horizontal?0:double(z)-ball.center.z;
        const double squared=allowedRadius*allowedRadius-dz*dz;if(allowedRadius<=0||squared<=0)return {};
        const double apothem=sqrt(squared)*(outer?1:cos(double(syncFacingPi)/sides));
        for(unsigned i=0;i<sides;++i){const double a=2*double(syncFacingPi)*(i+.5)/sides,nx=cos(a),ny=sin(a);
            polygon=syncExclusiveClip(polygon,nx,ny,-nx*ball.center.x-ny*ball.center.y-apothem,budget);if(polygon.empty())return {};}}
    return polygon;
}
static bool syncExclusiveBoundsOverlap(const SyncExclusivePolygon& a,const SyncExclusivePolygon& b) {
    auto bounds=[](const auto& polygon){std::array<double,4> result{DBL_MAX,DBL_MAX,-DBL_MAX,-DBL_MAX};
        for(auto p:polygon){result[0]=std::min(result[0],p.x);result[1]=std::min(result[1],p.y);result[2]=std::max(result[2],p.x);result[3]=std::max(result[3],p.y);}return result;};
    const auto x=bounds(a),y=bounds(b);return x[0]<=y[2]&&x[2]>=y[0]&&x[1]<=y[3]&&x[3]>=y[1];
}
static void syncExclusiveSubtract(const SyncExclusivePolygon& source,const SyncExclusivePolygon& excluded,std::vector<SyncExclusivePolygon>& result,SyncExclusiveBudget& budget,size_t maxPieces=256) {
    if(!syncExclusiveBoundsOverlap(source,excluded)){result.push_back(source);return;}
    auto remaining=source;
    for(size_t i=0;i<excluded.size()&&!remaining.empty()&&!budget.exhausted;++i){double nx,ny,d;syncExclusiveEdge(excluded[i],excluded[(i+1)%excluded.size()],nx,ny,d);
        auto outside=syncExclusiveClip(remaining,-nx,-ny,-d,budget);
        if(!outside.empty())result.push_back(std::move(outside));
        remaining=syncExclusiveClip(remaining,nx,ny,d,budget);
        if(result.size()>maxPieces){budget.exhausted=true;return;}}
}
static bool syncExclusiveSegmentInterval(const SyncExclusivePolygon& polygon,SyncExclusivePoint a,SyncExclusivePoint b,double& low,double& high,SyncExclusiveBudget& budget) {
    low=0;high=1;if(polygon.empty()||!budget.take(polygon.size()))return false;
    for(size_t i=0;i<polygon.size();++i){double nx,ny,d;syncExclusiveEdge(polygon[i],polygon[(i+1)%polygon.size()],nx,ny,d);
        const double start=nx*a.x+ny*a.y+d,speed=nx*(b.x-a.x)+ny*(b.y-a.y);
        if(fabs(speed)<1e-12){if(start>1e-9)return false;continue;}
        const double t=-start/speed;if(speed>0)high=std::min(high,t);else low=std::max(low,t);
        if(high<=low)return false;}return high-low>1e-9;
}
static void syncExclusiveRemoveInterval(std::vector<std::pair<double,double>>& intervals,double low,double high) {
    std::vector<std::pair<double,double>> next;
    for(auto interval:intervals){if(high<=interval.first||low>=interval.second){next.push_back(interval);continue;}
        if(low>interval.first)next.push_back({interval.first,low});if(high<interval.second)next.push_back({high,interval.second});}
    intervals=std::move(next);
}
struct SyncExclusiveGuide {
    bool known=false;
    bool requiredAimReference=false;
    std::string detail;
    float z=0;
    double clearance=.002;
    SyncRangeVolume selected;
    std::vector<SyncRangeVolume> competitors;
    std::vector<SyncExclusivePolygon> pieces;
    std::vector<std::pair<SyncExclusivePoint,SyncExclusivePoint>> boundary;
    // Distinct native interactions may resolve to the same animation. Each
    // branch retains its own constraints and priority-based exclusions.
    std::vector<SyncExclusiveGuide> branches;
    bool currentPoses=false;
    Vec3 selectionPosition{};
    std::vector<Vec3> selectionCompetitors;
    double selectionTolerance=0;
};
static double syncSelectionDistance(Vec3 point,Vec3 site){const auto delta=subtract(point,site);return sqrt(double(dot(delta,delta)));}
static bool syncExclusiveNativeContains(const SyncExclusiveGuide& guide,Vec3 point) {
    if(!guide.branches.empty()){for(const auto& branch:guide.branches)if(syncExclusiveNativeContains(branch,point))return true;return false;}
    if(!syncRangeContains(guide.selected,point))return false;
    for(const auto& other:guide.competitors)if(syncRangeContains(other,point))return false;
    if(guide.currentPoses)for(auto site:guide.selectionCompetitors)
        if(syncSelectionDistance(point,site)<=syncSelectionDistance(point,guide.selectionPosition)+guide.selectionTolerance)return false;
    return true;
}
static bool syncExclusiveContains(const SyncExclusiveGuide& guide,Vec3 point) {
    if(!guide.known||fabsf(point.z-guide.z)>.0001f||!syncExclusiveNativeContains(guide,point))return false;
    for(const auto& piece:guide.pieces)if(syncExclusiveInside(piece,{point.x,point.y}))return true;return false;
}
static SyncExclusiveGuide syncExclusiveBuild(SyncRangeVolume selected,std::vector<SyncRangeVolume> competitors,float z) {
    SyncExclusiveGuide result;result.selected=std::move(selected);result.competitors=std::move(competitors);result.z=z;
    if(result.selected.balls.empty()||result.competitors.size()>32){result.detail="Exclusive area unavailable: unsupported constraint count";return result;}
    const auto center=result.selected.balls.front().center;
    result.clearance=.002+8*FLT_EPSILON*std::max({fabs(double(center.x)),fabs(double(center.y)),fabs(double(z))});
    SyncExclusiveBudget budget;const auto original=syncExclusiveSection(result.selected,z,false,result.clearance,budget);
    std::vector<SyncExclusivePolygon> excluded;
    if(!original.empty())result.pieces.push_back(original);
    for(const auto& other:result.competitors){auto polygon=syncExclusiveSection(other,z,true,result.clearance,budget);
        if(polygon.empty())continue;
        std::vector<SyncExclusivePolygon> next;for(const auto& piece:result.pieces){syncExclusiveSubtract(piece,polygon,next,budget);if(next.size()>256)budget.exhausted=true;if(budget.exhausted)break;}
        result.pieces=std::move(next);excluded.push_back(std::move(polygon));if(result.pieces.empty()||budget.exhausted)break;}
    // Boundary comes from the selected and excluded contours, not from the
    // convex partition's internal edges. Do not draw artificial fragment seams.
    if(!result.pieces.empty()&&!budget.exhausted){std::vector<SyncExclusivePolygon> contours{original};contours.insert(contours.end(),excluded.begin(),excluded.end());
        for(size_t owner=0;owner<contours.size()&&!budget.exhausted;++owner){const auto& polygon=contours[owner];
            for(size_t edge=0;edge<polygon.size()&&!budget.exhausted;++edge){auto a=polygon[edge],b=polygon[(edge+1)%polygon.size()];std::vector<std::pair<double,double>> intervals{{0,1}};
                if(owner){double low,high;if(!syncExclusiveSegmentInterval(original,a,b,low,high,budget))continue;intervals={{low,high}};}
                for(size_t j=0;j<excluded.size()&&!intervals.empty();++j){if(owner==j+1)continue;double low,high;
                    if(syncExclusiveSegmentInterval(excluded[j],a,b,low,high,budget)){
                        const double middle=(low+high)*.5;const SyncExclusivePoint p{a.x+(b.x-a.x)*middle,a.y+(b.y-a.y)*middle};
                        if(!owner||owner>j+1||syncExclusiveInside(excluded[j],p,-1e-8))syncExclusiveRemoveInterval(intervals,low,high);}}
                for(auto interval:intervals){if(interval.second-interval.first<1e-8)continue;
                    result.boundary.push_back({{a.x+(b.x-a.x)*interval.first,a.y+(b.y-a.y)*interval.first},{a.x+(b.x-a.x)*interval.second,a.y+(b.y-a.y)*interval.second}});}}}}
    size_t vertices=0;for(const auto& piece:result.pieces)vertices+=piece.size();
    if(budget.exhausted||vertices>4096||result.boundary.size()>4096){result.pieces.clear();result.boundary.clear();result.detail="Exclusive area unavailable: geometry budget";return result;}
    // Conversion to the renderer's float coordinates must still pass the real
    // native predicates, including the boundaries of every excluded volume.
    for(const auto& piece:result.pieces)for(auto p:piece)if(!syncExclusiveNativeContains(result,{float(p.x),float(p.y),z})){
        result.pieces.clear();result.boundary.clear();result.detail="Exclusive area unavailable: boundary precision";return result;}
    for(const auto& edge:result.boundary)for(auto p:{edge.first,edge.second})if(!syncExclusiveNativeContains(result,{float(p.x),float(p.y),z})){
        result.pieces.clear();result.boundary.clear();result.detail="Exclusive area unavailable: boundary precision";return result;}
    result.known=true;result.detail=result.pieces.empty()?"No exclusive area at your foot height":"Exclusive positions | other animation areas removed";return result;
}
// Union without overlapping translucent caps or internal contour walls. Never
// combine the branch predicates into a single, larger convex approximation.
static SyncExclusiveGuide syncExclusiveUnion(std::vector<SyncExclusiveGuide> branches) {
    SyncExclusiveGuide result;if(branches.empty())return result;
    result.selected=branches.front().selected;result.z=branches.front().z;
    SyncExclusiveBudget budget;
    for(const auto& branch:branches){
        if(!branch.known||branch.z!=result.z){result.detail="Same-animation branch unavailable";return result;}
        result.requiredAimReference|=branch.requiredAimReference;
        for(const auto& piece:branch.pieces){std::vector<SyncExclusivePolygon> remaining{piece};
            for(const auto& covered:result.pieces){std::vector<SyncExclusivePolygon> next;
                for(const auto& p:remaining)syncExclusiveSubtract(p,covered,next,budget,4096);
                remaining=std::move(next);if(remaining.empty()||budget.exhausted)break;}
            if(budget.exhausted||result.pieces.size()+remaining.size()>4096){result.pieces.clear();result.detail="Same-animation union geometry budget";return result;}
            result.pieces.insert(result.pieces.end(),remaining.begin(),remaining.end());}}
    result.branches=std::move(branches);result.known=true;
    result.detail=result.pieces.empty()?"No exclusive area at your foot height":"Exclusive positions for all native branches of this animation";
    return result;
}
// For fixed native selector poses, distance(B) >= distance(A)+t is a
// convex second-order-cone constraint. On each triangle, the affine
// interpolation of distance(A) is an UPPER bound (convexity of the norm).
// Clipping against that upper bound gives an inner approximation: no
// triangle can introduce a random-band overlap. Never extrapolate priorities
// or poses as invariant rules of the animation; this is a labelled snapshot.
static void syncExclusiveSelectPoses(SyncExclusiveGuide& guide,Vec3 chosen,const std::vector<Vec3>& others,double tolerance) {
    if(!guide.known||others.empty())return;
    guide.currentPoses=true;guide.selectionPosition=chosen;guide.selectionCompetitors=others;guide.selectionTolerance=tolerance;
    std::vector<SyncExclusivePolygon> triangles;
    for(const auto& polygon:guide.pieces){SyncExclusivePoint center{};for(auto p:polygon){center.x+=p.x/polygon.size();center.y+=p.y/polygon.size();}
        for(size_t i=0;i<polygon.size();++i)triangles.push_back({center,polygon[i],polygon[(i+1)%polygon.size()]});}
    for(unsigned level=0;level<2;++level){std::vector<SyncExclusivePolygon> next;
        for(const auto& tri:triangles){auto a=tri[0],b=tri[1],c=tri[2];SyncExclusivePoint ab{(a.x+b.x)/2,(a.y+b.y)/2},bc{(b.x+c.x)/2,(b.y+c.y)/2},ca{(c.x+a.x)/2,(c.y+a.y)/2};
            next.push_back({a,ab,ca});next.push_back({ab,b,bc});next.push_back({ca,bc,c});next.push_back({ab,bc,ca});}triangles=std::move(next);}
    SyncExclusiveBudget budget;guide.pieces.clear();guide.boundary.clear();const double t=tolerance+2*guide.clearance;
    for(const auto& tri:triangles){const auto a=tri[0],b=tri[1],c=tri[2];const double determinant=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
        if(determinant<1e-12)continue;
        auto norm=[&](SyncExclusivePoint p){return syncSelectionDistance({float(p.x),float(p.y),guide.z},chosen);};
        const double da=norm(a),db=norm(b),dc=norm(c),nx=((db-da)*(c.y-a.y)-(dc-da)*(b.y-a.y))/determinant,
            ny=((b.x-a.x)*(dc-da)-(c.x-a.x)*(db-da))/determinant,d=da-nx*a.x-ny*a.y;
        auto piece=tri;
        for(auto other:others){const double constant=double(other.x)*other.x+double(other.y)*other.y+double(other.z-guide.z)*(other.z-guide.z)-
                double(chosen.x)*chosen.x-double(chosen.y)*chosen.y-double(chosen.z-guide.z)*(chosen.z-guide.z);
            piece=syncExclusiveClip(piece,2*t*nx-2*(double(chosen.x)-other.x),2*t*ny-2*(double(chosen.y)-other.y),2*t*d-constant+t*t,budget);
            if(piece.empty())break;}
        if(!piece.empty())guide.pieces.push_back(std::move(piece));}
    if(budget.exhausted||guide.pieces.size()>2048){guide.pieces.clear();guide.known=false;guide.detail="Current-pose selection area unavailable: geometry budget";return;}
    // Without excluded volumes, this is an intersection of convex balls,
    // halfspaces and ||p-A|| <= affine(p) distance-gap cones. The convex hull
    // of its proven-inside vertices therefore remains inside every constraint.
    // Collapse the construction partition before rendering/clearance difference;
    // never do this for a region with holes or a wide/nonconvex facing gate.
    if(guide.competitors.empty()&&!guide.selected.wideFacing&&guide.pieces.size()>1){std::vector<SyncExclusivePoint> points;
        for(const auto& p:guide.pieces)points.insert(points.end(),p.begin(),p.end());
        std::sort(points.begin(),points.end(),[](auto a,auto b){return a.x<b.x||(a.x==b.x&&a.y<b.y);});
        points.erase(std::unique(points.begin(),points.end(),[](auto a,auto b){return a.x==b.x&&a.y==b.y;}),points.end());
        auto turn=[](auto a,auto b,auto c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);};
        SyncExclusivePolygon hull;
        for(auto p:points){while(hull.size()>=2&&turn(hull[hull.size()-2],hull.back(),p)<=1e-12)hull.pop_back();hull.push_back(p);}
        const auto lower=hull.size();if(points.size()>1)for(size_t i=points.size()-1;i-->0;){const auto p=points[i];while(hull.size()>lower&&turn(hull[hull.size()-2],hull.back(),p)<=1e-12)hull.pop_back();hull.push_back(p);}
        if(hull.size()>1)hull.pop_back();if(hull.size()>=3&&hull.size()<=512)guide.pieces={std::move(hull)};
    }
    for(const auto& polygon:guide.pieces)for(auto p:polygon)if(!syncExclusiveNativeContains(guide,{float(p.x),float(p.y),guide.z})){
        guide.pieces.clear();guide.known=false;guide.detail="Current-pose selection area unavailable: precision";return;}
    guide.detail=guide.pieces.empty()?"No unique distance-choice area for the current native poses":"Distance-choice area for CURRENT native poses | random band removed";
}
static bool syncExclusiveDestination(const SyncExclusiveGuide& guide,Vec3 player,Vec3& destination) {
    if(!guide.known||guide.pieces.empty())return false;double best=DBL_MAX;
    for(const auto& polygon:guide.pieces){SyncExclusivePoint center{};for(auto p:polygon){center.x+=p.x/polygon.size();center.y+=p.y/polygon.size();}
        auto consider=[&](SyncExclusivePoint p){Vec3 point{float(p.x),float(p.y),guide.z};if(!syncExclusiveContains(guide,point))return;
            const auto delta=subtract(point,player);const double distance=dot(delta,delta);if(distance<best){best=distance;destination=point;}};
        consider(center);if(syncExclusiveInside(polygon,{player.x,player.y}))consider({player.x,player.y});
        for(size_t i=0;i<polygon.size();++i){auto a=polygon[i],b=polygon[(i+1)%polygon.size()];const double dx=b.x-a.x,dy=b.y-a.y,length2=dx*dx+dy*dy;
            const double t=length2>1e-16?std::clamp(((player.x-a.x)*dx+(player.y-a.y)*dy)/length2,0.,1.):0;
            SyncExclusivePoint p{a.x+dx*t,a.y+dy*t};const double distance=hypot(center.x-p.x,center.y-p.y),inset=distance>0?std::min(.01,guide.clearance/distance):0;
            consider({p.x+(center.x-p.x)*inset,p.y+(center.y-p.y)*inset});}}
    return best<DBL_MAX;
}
static bool syncExclusiveRayInterval(const SyncRangeVolume& volume,Vec3 origin,Vec3 direction,double margin,double& low,double& high) {
    low=0;high=200;
    for(const auto& plane:volume.planes){const double length=sqrt(double(dot(plane.n,plane.n))),start=double(dot(plane.n,origin))+plane.d+margin*length,speed=dot(plane.n,direction);
        if(fabs(speed)<1e-10){if(start>0)return false;continue;}const double t=-start/speed;if(speed>0)high=std::min(high,t);else low=std::max(low,t);}
    for(const auto& ball:volume.balls){auto delta=subtract(origin,ball.center);if(ball.horizontal)delta.z=0;const double allowedRadius=ball.radius-margin;
        if(allowedRadius<=0)return false;const double b=dot(delta,direction),c=double(dot(delta,delta))-allowedRadius*allowedRadius,discriminant=b*b-c;
        if(discriminant<0)return false;const double root=sqrt(discriminant);low=std::max(low,-b-root);high=std::min(high,-b+root);}
    return std::isfinite(low)&&std::isfinite(high)&&high-low>.0001;
}
static std::vector<std::pair<double,double>> syncExclusiveApproach(const SyncExclusiveGuide& guide,Vec3 player) {
    std::vector<std::pair<double,double>> intervals;if(!guide.known||guide.selected.balls.empty())return intervals;
    const auto center=guide.selected.balls.front().center;const Vec3 origin{center.x,center.y,player.z};auto direction=subtract(player,origin);
    const float length=sqrtf(dot(direction,direction));if(length<.0001f)return intervals;direction=motionScale(direction,1/length);double low,high;
    SyncExclusiveBudget budget;const SyncExclusivePoint end{origin.x+direction.x*200,origin.y+direction.y*200};
    for(const auto& polygon:guide.pieces)if(syncExclusiveSegmentInterval(polygon,{origin.x,origin.y},end,low,high,budget))intervals.push_back({std::max(0.,low)*200,std::min(1.,high)*200});
    std::sort(intervals.begin(),intervals.end());std::vector<std::pair<double,double>> merged;
    for(auto interval:intervals){if(!merged.empty()&&interval.first<=merged.back().second+1e-8)merged.back().second=std::max(merged.back().second,interval.second);else merged.push_back(interval);}intervals=std::move(merged);
    const double dz=double(player.z)-center.z;for(auto& interval:intervals){interval.first=sqrt(interval.first*interval.first+dz*dz);interval.second=sqrt(interval.second*interval.second+dz*dz);}return intervals;
}
static void appendSyncExclusiveGeometry(const SyncExclusiveGuide& guide,Vec3 player,std::vector<Vertex>& output,bool lines,bool confirmed) {
    if(!guide.known||guide.pieces.empty())return;const bool inside=syncExclusiveContains(guide,player);
    const std::array<float,4> color=!inside?std::array<float,4>{.3f,.85f,1,lines?.95f:.1f}:confirmed?std::array<float,4>{.3f,1,.5f,lines?.95f:.1f}:std::array<float,4>{1,.65f,.15f,lines?.95f:.1f};
    auto append=[&](SyncExclusivePoint p,const std::array<float,4>& tint){Vertex vertex{};vertex.position={float(p.x),float(p.y),guide.z};memcpy(vertex.color,tint.data(),16);output.push_back(vertex);};
    if(!lines){for(const auto& piece:guide.pieces)for(size_t i=1;i+1<piece.size();++i){append(piece[0],color);append(piece[i],color);append(piece[i+1],color);}return;}
    for(const auto& edge:guide.boundary){append(edge.first,color);append(edge.second,color);}
    const std::array<float,4> gold{1,.85f,.25f,1};auto marker=[&](Vec3 p){for(auto direction:std::array<Vec3,2>{{{.22f,0,0},{0,.22f,0}}}){
        append({p.x-direction.x,p.y-direction.y},gold);append({p.x+direction.x,p.y+direction.y},gold);}};
    marker(player);Vec3 destination{};if(!inside&&syncExclusiveDestination(guide,player,destination)){append({player.x,player.y},gold);append({destination.x,destination.y},gold);marker(destination);}
}
