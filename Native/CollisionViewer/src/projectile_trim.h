#pragma once

// Local rendering experiment. Physics contents and the original snapshot are
// untouched. Subtraction uses validated native convex planes, or a closed
// convex decoded-mesh fallback. AABBs only select candidates, never erase geometry.
namespace projectile_trim {
struct Point {
    double x=0,y=0,z=0;
    Point operator+(Point b) const { return {x+b.x,y+b.y,z+b.z}; }
    Point operator-(Point b) const { return {x-b.x,y-b.y,z-b.z}; }
    Point operator*(double s) const { return {x*s,y*s,z*s}; }
};
static Point point(Vec3 v) { return {v.x,v.y,v.z}; }
static Vec3 vertex(Point p) { return {float(p.x),float(p.y),float(p.z)}; }
static double dot(Point a,Point b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static Point cross(Point a,Point b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
struct Plane { Point n; double d; double distance(Point p) const { return dot(n,p)+d; } };
using Polygon=std::vector<Point>;
struct Hull { std::vector<std::array<Vec3,3>> faces; std::vector<Plane> native; Point center{}; };
struct Limits {
    uint64_t work=80000000;
    size_t hullCache=4096;
    std::chrono::milliseconds time{2000};
};
struct Budget {
    Limits limits;
    std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
    std::function<bool()> cancelled;
    uint64_t work=0;
    bool exhausted=false;
    bool spend() {
        if(exhausted) return false;
        if((++work&1023)==0&&(work>limits.work||cancelled()||std::chrono::steady_clock::now()-start>limits.time)) exhausted=true;
        return !exhausted;
    }
};
static Hull closedConvex(const Geometry& geometry,Budget& budget) {
    Hull result;
    if(geometry.convexSolid&&geometry.solidPlanes.size()>=4&&geometry.solidPlanes.size()<=512&&!geometry.triangles.empty()&&geometry.triangles.size()<=60000) {
        Point center{}; double magnitude=1; bool valid=true;
        for(Vec3 p:geometry.triangles) {
            for(float v:{p.x,p.y,p.z}) { if(!std::isfinite(v)) valid=false; magnitude=std::max(magnitude,fabs(double(v))); }
            center=center+point(p);
        }
        center=center*(1.0/geometry.triangles.size());
        const double tolerance=magnitude*2e-6;
        for(const auto& raw:geometry.solidPlanes) {
            Point n{raw[0],raw[1],raw[2]}; double length=sqrt(dot(n,n));
            if(!std::isfinite(length)||fabs(length-1)>.001||!std::isfinite(raw[3])) { valid=false; break; }
            Plane p{n*(1/length),raw[3]/length};
            if(p.distance(center)>=-1e-8) { valid=false; break; }
            for(Vec3 v:geometry.triangles) if(!budget.spend()||p.distance(point(v))>tolerance) { valid=false; break; }
            if(!valid) break;
            result.native.push_back(p);
        }
        if(valid) { result.center=center; return result; }
        result=Hull{};
        if(budget.exhausted) return result;
    }
    if(!geometry.convexSolid||geometry.triangles.size()<12||geometry.triangles.size()>768||geometry.triangles.size()%3) return result;
    std::vector<Vec3> points;
    std::vector<uint32_t> indices;
    for(Vec3 p:geometry.triangles) {
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)) return {};
        auto it=std::find_if(points.begin(),points.end(),[&](Vec3 v) { return v.x==p.x&&v.y==p.y&&v.z==p.z; });
        if(it==points.end()) { indices.push_back(uint32_t(points.size())); points.push_back(p); } else indices.push_back(uint32_t(it-points.begin()));
    }
    if(points.size()<4||points.size()>256) return {};
    Point center{}; double extent=0;
    for(Vec3 p:points) center=center+point(p);
    center=center*(1.0/points.size());
    for(Vec3 p:points) extent=std::max(extent,sqrt(dot(point(p)-center,point(p)-center)));
    const double tolerance=std::max(1e-8,extent*1e-7);
    struct Edge { unsigned count=0; int winding=0; };
    std::unordered_map<uint64_t,Edge> edges;
    for(size_t i=0;i<indices.size();i+=3) {
        Vec3 a=points[indices[i]],b=points[indices[i+1]],c=points[indices[i+2]];
        Point n=cross(point(b)-point(a),point(c)-point(a)); double length=sqrt(dot(n,n));
        if(length<1e-12) return {};
        n=n*(1.0/length); double d=-dot(n,point(a));
        if(dot(n,center)+d>0) { n=n*(-1); d=-d; }
        if(dot(n,center)+d>=-tolerance*4) return {};
        for(Vec3 p:points) if(!budget.spend()||dot(n,point(p))+d>tolerance) return {};
        for(unsigned k=0;k<3;++k) {
            auto x=indices[i+k],y=indices[i+(k+1)%3];
            auto& edge=edges[(uint64_t(std::min(x,y))<<32)|std::max(x,y)];
            ++edge.count; edge.winding+=x<y?1:-1;
        }
        // Coplanar triangles share one clipping plane.
        bool duplicate=false;
        for(const auto& face:result.faces) {
            Point fn=cross(point(face[1])-point(face[0]),point(face[2])-point(face[0])); fn=fn*(1/sqrt(dot(fn,fn)));
            if(dot(fn,center-point(face[0]))>0) fn=fn*(-1);
            if(dot(fn,n)>1-1e-12&&fabs(dot(n,point(face[0]))+d)<tolerance) { duplicate=true; break; }
        }
        if(!duplicate) result.faces.push_back({a,b,c});
    }
    for(const auto& e:edges) if(e.second.count!=2||e.second.winding!=0) return {};
    return result.faces.size()>=4?result:Hull{};
}
static std::vector<Plane> worldPlanes(const Hull& hull,const Mesh& mesh) {
    std::vector<Plane> result;
    if(!hull.native.empty()) {
        const auto* m=mesh.matrix;
        const double a=m[0],b=m[1],c=m[2],d=m[4],e=m[5],f=m[6],g=m[8],h=m[9],i=m[10];
        const double det=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
        if(!std::isfinite(det)||fabs(det)<1e-12) return result;
        const Point center{a*hull.center.x+b*hull.center.y+c*hull.center.z+m[3],d*hull.center.x+e*hull.center.y+f*hull.center.z+m[7],g*hull.center.x+h*hull.center.y+i*hull.center.z+m[11]};
        const double inset=std::max(.0005,std::max({fabs(center.x),fabs(center.y),fabs(center.z)})*2e-6);
        for(const auto& plane:hull.native) {
            const Point n{((e*i-f*h)*plane.n.x+(f*g-d*i)*plane.n.y+(d*h-e*g)*plane.n.z)/det,
                          ((c*h-b*i)*plane.n.x+(a*i-c*g)*plane.n.y+(b*g-a*h)*plane.n.z)/det,
                          ((b*f-c*e)*plane.n.x+(c*d-a*f)*plane.n.y+(a*e-b*d)*plane.n.z)/det};
            const double length=sqrt(dot(n,n)); if(!std::isfinite(length)||length<1e-12) return {};
            Plane transformed{n*(1/length),(plane.d-dot(n,{m[3],m[7],m[11]}))/length};
            if(transformed.distance(center)>=-inset*2) return {};
            transformed.d+=inset; result.push_back(transformed);
        }
        return result;
    }
    Point center{};
    for(const auto& face:hull.faces) for(Vec3 p:face) center=center+point(mesh.world(p));
    if(hull.faces.empty()) return result;
    center=center*(1.0/(hull.faces.size()*3));
    double magnitude=1;
    for(double v:{center.x,center.y,center.z}) magnitude=std::max(magnitude,fabs(v));
    // Inset beyond float transform error. Coincident boundaries deliberately
    // remain visible rather than risk cutting an exterior face by rounding.
    const double inset=std::max(.0005,magnitude*2e-6);
    for(const auto& face:hull.faces) {
        Point a=point(mesh.world(face[0])),b=point(mesh.world(face[1])),c=point(mesh.world(face[2]));
        Point n=cross(b-a,c-a); double length=sqrt(dot(n,n));
        if(!std::isfinite(length)||length<1e-12) return {};
        n=n*(1.0/length); double d=-dot(n,a);
        if(dot(n,center)+d>0) { n=n*(-1); d=-d; }
        if(dot(n,center)+d>=-inset*2) return {};
        result.push_back({n,d+inset});
    }
    return result;
}
static Polygon half(const Polygon& input,const Plane& plane,bool inside,Budget& budget) {
    Polygon result;
    if(input.empty()) return result;
    Point a=input.back(); double da=plane.distance(a);
    for(Point b:input) {
        if(!budget.spend()) return {};
        double db=plane.distance(b); bool keepA=inside?da<=0:da>=0,keepB=inside?db<=0:db>=0;
        if(keepA!=keepB) result.push_back(a+(b-a)*(da/(da-db)));
        if(keepB) result.push_back(b);
        a=b; da=db;
    }
    return result;
}
static bool hasArea(const Polygon& polygon) {
    if(polygon.size()<3) return false;
    Point area{};
    for(size_t i=1;i+1<polygon.size();++i) area=area+cross(polygon[i]-polygon[0],polygon[i+1]-polygon[0]);
    return dot(area,area)>0;
}
static bool subtract(std::vector<Polygon>& polygons,const std::vector<Plane>& planes,Budget& budget) {
    std::vector<Polygon> output;
    for(auto polygon:polygons) {
        // Plane partitions alone can split an entirely disjoint triangle into
        // many pieces. First prove a positive-area intersection with the solid;
        // otherwise retain the original polygon, including its triangulation.
        auto intersection=polygon;
        for(const auto& plane:planes) {
            intersection=half(intersection,plane,true,budget);
            if(budget.exhausted) return false;
            if(!hasArea(intersection)) break;
        }
        if(!hasArea(intersection)) { output.push_back(std::move(polygon)); continue; }
        for(const auto& plane:planes) {
            auto outside=half(polygon,plane,false,budget);
            if(hasArea(outside)) output.push_back(std::move(outside));
            polygon=half(polygon,plane,true,budget);
            if(budget.exhausted||output.size()>128||polygon.size()>256) return false;
            if(polygon.size()<3) break;
        }
    }
    polygons=std::move(output); return true;
}
using Segment=std::array<Point,2>;
static bool subtract(std::vector<Segment>& segments,const std::vector<Plane>& planes,Budget& budget) {
    std::vector<Segment> output;
    for(const auto& segment:segments) {
        double begin=0,end=1; bool intersects=true;
        for(const auto& plane:planes) {
            if(!budget.spend()) return false;
            double a=plane.distance(segment[0]),b=plane.distance(segment[1]);
            if(a>0&&b>0) { intersects=false; break; }
            if(a<=0&&b<=0) continue;
            double t=a/(a-b);
            if(a>0) begin=std::max(begin,t); else end=std::min(end,t);
            if(begin>=end) { intersects=false; break; }
        }
        if(!intersects) output.push_back(segment);
        else {
            Point delta=segment[1]-segment[0];
            if(begin>0) output.push_back({segment[0],segment[0]+delta*begin});
            if(end<1) output.push_back({segment[0]+delta*end,segment[1]});
        }
        if(output.size()>128) return false;
    }
    segments=std::move(output); return true;
}
static bool overlap(const float* a,const float* b) {
    for(int k=0;k<3;++k) if(a[k]>b[k+3]||a[k+3]<b[k]) return false;
    return true;
}
}
struct ProjectileTrim {
    enum Reason : uint8_t { NotProjectile,Pending,NoSolid,CandidateLimit,FragmentLimit,NoReduction,MorePrimitives,Trimmed,Removed,MemoryLimit,WorkLimit };
    // -1 retains the exact original mesh; -2 is fully covered.
    std::vector<int32_t> replacement;
    std::vector<Reason> reasons;
    std::vector<uint8_t> attempts;
    const RuntimeScene* source=nullptr;
    std::vector<Mesh> meshes;
    uint64_t originalTriangles=0,originalLines=0,triangles=0,lines=0,changed=0,removed=0,growthFallback=0;
    uint64_t hulls=0,bytes=0;
    bool limited=false;
    double milliseconds=0;
};
static std::shared_ptr<ProjectileTrim> buildProjectileTrim(const RuntimeScene& scene,const std::function<bool()>& cancelled=[] { return false; },const projectile_trim::Limits& limits={},const GameCamera* focus=nullptr,float focusRadius=40,const ProjectileTrim* previous=nullptr) {
    using namespace projectile_trim;
    const bool resume=previous&&previous->source==&scene&&previous->replacement.size()==scene.meshes.size()&&previous->reasons.size()==scene.meshes.size()&&previous->attempts.size()==scene.meshes.size();
    auto result=resume?std::make_shared<ProjectileTrim>(*previous):std::make_shared<ProjectileTrim>();
    if(!resume) {
        result->source=&scene;
        result->replacement.resize(scene.meshes.size(),-1);
        result->reasons.resize(scene.meshes.size(),ProjectileTrim::NotProjectile);
        result->attempts.resize(scene.meshes.size());
    }
    Budget budget; budget.cancelled=cancelled; budget.limits=limits;
    std::unordered_map<const Geometry*,Hull> hulls;
    struct Target { uint32_t index; float distance; bool visible; };
    std::vector<Target> targets;
    for(uint32_t i=0;i<scene.meshes.size();++i) {
        const auto& mesh=scene.meshes[i]; if(mesh.kind!=3) continue;
        if(!resume) {
            result->reasons[i]=ProjectileTrim::Pending;
            result->originalTriangles+=mesh.geometry->triangles.size()/3; result->originalLines+=mesh.geometry->lines.size()/2;
        } else if(result->reasons[i]!=ProjectileTrim::Pending) continue;
        float distance=0; bool visible=false;
        if(focus) { ViewFilter view(*focus); distance=view.distanceSquared(mesh.bounds); visible=distance<=focusRadius*focusRadius&&view.inView(mesh.bounds); }
        if(resume&&focus&&!visible) continue;
        targets.push_back({i,distance,visible});
    }
    if(focus) std::stable_sort(targets.begin(),targets.end(),[](const Target& a,const Target& b) { return a.visible!=b.visible?a.visible>b.visible:a.distance<b.distance; });
    if(!resume) { result->triangles=result->originalTriangles; result->lines=result->originalLines; }
    for(const auto& target:targets) {
        const size_t index=target.index;
        if(cancelled()) return {};
        if(std::chrono::steady_clock::now()-budget.start>limits.time) budget.exhausted=true;
        if(budget.exhausted) break;
        // A single pathological mesh must not indefinitely starve the rest of
        // the current view. A second interrupted attempt retains its original.
        if(++result->attempts[index]>=2) result->reasons[index]=ProjectileTrim::WorkLimit;
        const auto& mesh=scene.meshes[index]; if(mesh.kind!=3) continue;
        std::vector<std::vector<Plane>> candidates;
        bool tooMany=false;
        auto visit=[&](auto&& self,uint32_t nodeIndex)->void {
            if(tooMany||!budget.spend()) return;
            const auto& node=scene.nodes[nodeIndex]; if(!overlap(mesh.bounds,node.bounds)) return;
            if(node.left!=~0u) { self(self,node.left); self(self,node.right); return; }
            for(uint32_t i=node.begin;i<node.begin+node.count;++i) {
                const auto& solid=scene.meshes[scene.order[i]];
                if(solid.kind!=1||!solid.geometry->convexSolid||!overlap(mesh.bounds,solid.bounds)) continue;
                auto found=hulls.find(solid.geometry.get());
                if(found==hulls.end()) {
                    if(hulls.size()>=limits.hullCache) { budget.exhausted=true; break; }
                    found=hulls.emplace(solid.geometry.get(),closedConvex(*solid.geometry,budget)).first;
                }
                auto planes=worldPlanes(found->second,solid);
                if(!planes.empty()) { candidates.push_back(std::move(planes)); ++result->hulls; }
                if(candidates.size()>64) { tooMany=true; break; }
            }
        };
        if(!scene.nodes.empty()) visit(visit,0);
        if(budget.exhausted) continue;
        if(tooMany) { result->reasons[index]=ProjectileTrim::CandidateLimit; continue; }
        if(candidates.empty()) { result->reasons[index]=ProjectileTrim::NoSolid; continue; }
        auto geometry=std::make_shared<Geometry>(); geometry->kind=3;
        const auto& original=*mesh.geometry; bool triangleOk=true,lineOk=true;
        const size_t originalCount=original.triangles.size()+original.lines.size();
        for(size_t t=0;t+2<original.triangles.size()&&triangleOk;t+=3) {
            std::vector<projectile_trim::Polygon> polygons{{point(mesh.world(original.triangles[t])),point(mesh.world(original.triangles[t+1])),point(mesh.world(original.triangles[t+2]))}};
            for(const auto& planes:candidates) if(!subtract(polygons,planes,budget)) { triangleOk=false; break; }
            for(const auto& polygon:polygons) for(size_t k=1;k+1<polygon.size();++k) {
                geometry->triangles.push_back(vertex(polygon[0])); geometry->triangles.push_back(vertex(polygon[k])); geometry->triangles.push_back(vertex(polygon[k+1]));
            }
            if(geometry->triangles.size()>originalCount*4) triangleOk=false;
        }
        for(size_t l=0;l+1<original.lines.size()&&lineOk&&!budget.exhausted;l+=2) {
            std::vector<Segment> segments{{point(mesh.world(original.lines[l])),point(mesh.world(original.lines[l+1]))}};
            for(const auto& planes:candidates) if(!subtract(segments,planes,budget)) { lineOk=false; break; }
            for(const auto& segment:segments) { geometry->lines.push_back(vertex(segment[0])); geometry->lines.push_back(vertex(segment[1])); }
            if(geometry->lines.size()>originalCount*4) lineOk=false;
        }
        if(budget.exhausted) continue;
        const bool triangleGrowth=geometry->triangles.size()>original.triangles.size(),lineGrowth=geometry->lines.size()>original.lines.size();
        // An expensive fill split must not throw away a useful wire reduction,
        // or vice versa. Neither channel is allowed to grow beyond its original.
        if(!triangleOk||triangleGrowth) { geometry->triangles.clear(); for(Vec3 p:original.triangles) geometry->triangles.push_back(mesh.world(p)); }
        if(!lineOk||lineGrowth) { geometry->lines.clear(); for(Vec3 p:original.lines) geometry->lines.push_back(mesh.world(p)); }
        auto unchanged=[&](const std::vector<Vec3>& a,const std::vector<Vec3>& b) {
            if(a.size()!=b.size()) return false;
            for(size_t i=0;i<a.size();++i) { auto p=mesh.world(b[i]); if(a[i].x!=p.x||a[i].y!=p.y||a[i].z!=p.z) return false; }
            return true;
        };
        if(unchanged(geometry->triangles,original.triangles)&&unchanged(geometry->lines,original.lines)) {
            result->reasons[index]=!triangleOk||!lineOk?ProjectileTrim::FragmentLimit:triangleGrowth||lineGrowth?ProjectileTrim::MorePrimitives:ProjectileTrim::NoReduction;
            ++result->growthFallback; continue;
        }
        const size_t count=geometry->triangles.size()+geometry->lines.size();
        if(result->bytes+count*sizeof(Vec3)>32*1024*1024) { result->reasons[index]=ProjectileTrim::MemoryLimit; continue; }
        result->triangles-=original.triangles.size()/3-geometry->triangles.size()/3;
        result->lines-=original.lines.size()/2-geometry->lines.size()/2;
        ++result->changed; result->bytes+=count*sizeof(Vec3);
        result->reasons[index]=count?ProjectileTrim::Trimmed:ProjectileTrim::Removed;
        if(!count) { result->replacement[index]=-2; ++result->removed; }
        else {
            Mesh clipped=mesh; clipped.geometry=std::move(geometry);
            const float identity[]{1,0,0,0,0,1,0,0,0,0,1,0}; memcpy(clipped.matrix,identity,sizeof(identity));
            result->replacement[index]=int32_t(result->meshes.size()); result->meshes.push_back(std::move(clipped));
        }
    }
    result->limited=budget.exhausted||std::find(result->reasons.begin(),result->reasons.end(),ProjectileTrim::Pending)!=result->reasons.end();
    result->milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-budget.start).count();
    char message[480];
    sprintf_s(message,"Projectile trim experiment: triangles=%llu/%llu edges=%llu/%llu changed=%llu removed=%llu hullVisits=%llu fallback=%llu extraGeometry=%llu bytes build=%.2fms limited=%u",result->triangles,result->originalTriangles,result->lines,result->originalLines,result->changed,result->removed,result->hulls,result->growthFallback,result->bytes,result->milliseconds,unsigned(result->limited)); note(message);
    return result;
}
static bool pendingProjectileTrimInView(const RuntimeScene& scene,const ProjectileTrim& trim,const GameCamera& camera,float distance) {
    if(trim.source!=&scene||trim.reasons.size()!=scene.meshes.size()) return false;
    ViewFilter view(camera);
    auto visit=[&](auto&& self,uint32_t index)->bool {
        const auto& node=scene.nodes[index];
        if(view.distanceSquared(node.bounds)>distance*distance||!view.inView(node.bounds)) return false;
        if(node.left!=~0u) return self(self,node.left)||self(self,node.right);
        for(uint32_t i=node.begin;i<node.begin+node.count;++i) {
            auto target=scene.order[i]; const auto& mesh=scene.meshes[target];
            if(mesh.kind==3&&trim.reasons[target]==ProjectileTrim::Pending&&view.distanceSquared(mesh.bounds)<=distance*distance&&view.inView(mesh.bounds)) return true;
        }
        return false;
    };
    return !scene.nodes.empty()&&visit(visit,0);
}
