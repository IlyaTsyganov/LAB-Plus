#pragma once

struct SyncSectorMesh {
    bool known=false;
    std::vector<Vec3> fill,outline;
};
enum class SyncSectorTone {Reference,Waiting,Rejected,Confirmed};
// The exclusive XY section still describes the CURRENT foot-height test.
// Its display is extruded between the shot box's bottom/top, as requested.
// Display height does not introduce a new claim about native height limits.
static SyncSectorMesh syncBuildSectorMesh(const SyncExclusiveGuide& guide,const float* bounds) {
    SyncSectorMesh result;
    if(!guide.known||!syncAimBoundsValid(bounds))return result;
    SyncExclusiveBudget budget;
    const SyncExclusivePolygon box{{bounds[0],bounds[1]},{bounds[3],bounds[1]},{bounds[3],bounds[4]},{bounds[0],bounds[4]}};
    std::vector<SyncExclusivePolygon> pieces;
    for(const auto& piece:guide.pieces){syncExclusiveSubtract(piece,box,pieces,budget,4096);if(budget.exhausted||pieces.size()>4096)return result;}
    struct Edge {SyncExclusivePoint a,b;double length;};std::vector<Edge> edges;
    double minX=DBL_MAX,minY=DBL_MAX,maxX=-DBL_MAX,maxY=-DBL_MAX;
    for(const auto& piece:pieces)for(size_t i=0;i<piece.size();++i){const auto a=piece[i],b=piece[(i+1)%piece.size()];const double length=hypot(b.x-a.x,b.y-a.y);
        if(length<1e-8)continue;edges.push_back({a,b,length});minX=std::min(minX,a.x);minY=std::min(minY,a.y);maxX=std::max(maxX,a.x);maxY=std::max(maxY,a.y);}
    if(edges.size()>16384)return result;
    if(edges.empty()){result.known=true;return result;}
    // Pieces partition the area. Cancel shared, oppositely directed edges,
    // including partial edges at T junctions. A spatial index bounds the work;
    // there must be no fan lines or vertical walls on internal partition seams.
    const double cell=std::max(.25,std::max(maxX-minX,maxY-minY)/24),epsilon=1e-7;
    auto key=[](int x,int y){return (uint64_t(uint32_t(x))<<32)|uint32_t(y);};
    auto cells=[&](const Edge& edge){return std::array<int,4>{int(floor((std::min(edge.a.x,edge.b.x)-minX-epsilon)/cell)),int(floor((std::min(edge.a.y,edge.b.y)-minY-epsilon)/cell)),
        int(floor((std::max(edge.a.x,edge.b.x)-minX+epsilon)/cell)),int(floor((std::max(edge.a.y,edge.b.y)-minY+epsilon)/cell))};};
    std::unordered_map<uint64_t,std::vector<size_t>> grid;size_t work=0;
    for(size_t i=0;i<edges.size();++i){const auto c=cells(edges[i]);for(int x=c[0];x<=c[2];++x)for(int y=c[1];y<=c[3];++y){if(++work>4000000)return result;grid[key(x,y)].push_back(i);}}
    std::vector<size_t> seen(edges.size(),size_t(-1));std::vector<Edge> boundary;
    for(size_t i=0;i<edges.size();++i){const auto& a=edges[i];const double dx=(a.b.x-a.a.x)/a.length,dy=(a.b.y-a.a.y)/a.length;
        std::vector<std::pair<double,double>> intervals{{0,a.length}};const auto c=cells(a);
        for(int x=c[0];x<=c[2]&&!intervals.empty();++x)for(int y=c[1];y<=c[3]&&!intervals.empty();++y){const auto found=grid.find(key(x,y));if(found==grid.end())continue;
            for(size_t j:found->second){if(j==i||seen[j]==i)continue;seen[j]=i;if(++work>4000000)return result;const auto& b=edges[j];
                const double bx=b.b.x-b.a.x,by=b.b.y-b.a.y;
                if(dx*bx+dy*by>=0||fabs(dx*by-dy*bx)>epsilon*b.length||fabs(dx*(b.a.y-a.a.y)-dy*(b.a.x-a.a.x))>epsilon)continue;
                const double start=dx*(b.a.x-a.a.x)+dy*(b.a.y-a.a.y),end=dx*(b.b.x-a.a.x)+dy*(b.b.y-a.a.y);
                syncExclusiveRemoveInterval(intervals,std::min(start,end),std::max(start,end));}}
        for(auto interval:intervals)if(interval.second-interval.first>1e-6){const SyncExclusivePoint p{a.a.x+dx*interval.first,a.a.y+dy*interval.first},q{a.a.x+dx*interval.second,a.a.y+dy*interval.second};boundary.push_back({p,q,interval.second-interval.first});}}
    if(boundary.size()>2048)return result;
    auto point=[&](SyncExclusivePoint p,float z){return Vec3{float(p.x),float(p.y),z};};
    const float bottom=bounds[2],top=bounds[5];
    std::vector<int> curveNext(boundary.size(),-1),curvePrevious(boundary.size(),-1);std::vector<double> curveTurn(boundary.size(),0);
    auto triangle=[&](Vec3 a,Vec3 b,Vec3 c){result.fill.push_back(a);result.fill.push_back(b);result.fill.push_back(c);};
    for(const auto& piece:pieces)for(size_t i=1;i+1<piece.size();++i){triangle(point(piece[0],top),point(piece[i],top),point(piece[i+1],top));triangle(point(piece[0],bottom),point(piece[i+1],bottom),point(piece[i],bottom));}
    for(size_t i=0;i<boundary.size();++i){const auto& edge=boundary[i];const auto a=point(edge.a,bottom),b=point(edge.b,bottom),c=point(edge.b,top),d=point(edge.a,top);
        triangle(a,b,c);triangle(a,c,d);result.outline.insert(result.outline.end(),{a,b,d,c});
        // Smooth arc samples receive no vertical stitches. Only genuine
        // corners (over 15 degrees), or open endpoints, get a vertical edge.
        bool corner=true;
        for(size_t j=0;j<boundary.size();++j){if(i==j)continue;if(++work>4000000)return result;
            const auto& previous=boundary[j];if(hypot(previous.b.x-edge.a.x,previous.b.y-edge.a.y)>1e-6)continue;
            const double cosine=((previous.b.x-previous.a.x)*(edge.b.x-edge.a.x)+(previous.b.y-previous.a.y)*(edge.b.y-edge.a.y))/(previous.length*edge.length);
            const double sine=((previous.b.x-previous.a.x)*(edge.b.y-edge.a.y)-(previous.b.y-previous.a.y)*(edge.b.x-edge.a.x))/(previous.length*edge.length);
            if(cosine>=.965925826&&sine>=-1e-7){curveNext[j]=int(i);curvePrevious[i]=int(j);curveTurn[j]=std::max(0.,atan2(sine,cosine));}
            corner=cosine<.965925826;break;}
        if(corner){result.outline.push_back(a);result.outline.push_back(d);}}
    // Two additional uprights divide the outer arc into three equal lengths.
    // Follow smooth convex boundary chains, retaining collinear subdivisions
    // but rejecting straight sides and clockwise hole contours. Prefer the
    // outer chain by its length weighted by distance from the target box.
    std::vector<bool> visited(boundary.size(),false);std::vector<size_t> outerArc;double bestScore=0;
    const double cx=(double(bounds[0])+bounds[3])*.5,cy=(double(bounds[1])+bounds[4])*.5;
    auto consider=[&](size_t start){std::vector<size_t> chain;double turn=0,score=0;int at=int(start);
        while(at>=0&&!visited[size_t(at)]){const size_t i=size_t(at);visited[i]=true;chain.push_back(i);const auto& edge=boundary[i];
            score+=edge.length*hypot((edge.a.x+edge.b.x)*.5-cx,(edge.a.y+edge.b.y)*.5-cy);turn+=curveTurn[i];at=curveNext[i];}
        if(turn>.05&&score>bestScore){bestScore=score;outerArc=std::move(chain);}};
    for(size_t i=0;i<boundary.size();++i)if(curvePrevious[i]<0)consider(i);
    for(size_t i=0;i<boundary.size();++i)if(!visited[i])consider(i);
    double arcLength=0;for(size_t i:outerArc)arcLength+=boundary[i].length;
    for(unsigned divider=1;divider<=2&&!outerArc.empty();++divider){double remaining=arcLength*divider/3;
        for(size_t i:outerArc){const auto& edge=boundary[i];if(remaining>edge.length){remaining-=edge.length;continue;}
            const double t=remaining/edge.length;const SyncExclusivePoint p{edge.a.x+(edge.b.x-edge.a.x)*t,edge.a.y+(edge.b.y-edge.a.y)*t};
            result.outline.push_back(point(p,bottom));result.outline.push_back(point(p,top));break;}}
    result.known=true;return result;
}
static bool syncSectorNearBoundary(const SyncSectorMesh& mesh,Vec3 player,float clearanceRadius) {
    for(size_t i=0;i+1<mesh.outline.size();i+=2){const auto a=mesh.outline[i],b=mesh.outline[i+1];if(a.z!=b.z)continue;
        const double dx=b.x-a.x,dy=b.y-a.y,length=dx*dx+dy*dy;if(length<1e-12)continue;
        const double t=std::clamp(((player.x-a.x)*dx+(player.y-a.y)*dy)/length,0.,1.);
        if(hypot(player.x-a.x-t*dx,player.y-a.y-t*dy)<=clearanceRadius)return true;}return false;
}
static std::array<float,4> syncSectorColor(SyncSectorTone tone,bool lines) {
    const float alpha=lines?.9f:.09f;
    if(tone==SyncSectorTone::Confirmed)return {.3f,1,.5f,alpha};
    if(tone==SyncSectorTone::Rejected)return {1,.25f,.2f,alpha};
    if(tone==SyncSectorTone::Waiting)return {1,.65f,.15f,alpha};
    return {.65f,.7f,.75f,lines?.65f:.06f};
}
static void appendSyncSectorMesh(const SyncSectorMesh& mesh,std::vector<Vertex>& output,bool lines,SyncSectorTone tone) {
    if(!mesh.known)return;const auto color=syncSectorColor(tone,lines);
    for(auto point:lines?mesh.outline:mesh.fill){Vertex vertex{};vertex.position=point;memcpy(vertex.color,color.data(),16);output.push_back(vertex);}
}
