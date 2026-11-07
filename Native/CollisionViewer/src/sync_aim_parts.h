#pragma once

// Screen-facing patches on the wire-box projection, using current shot poses
// and native joint-group membership. No extra anatomical spheres are drawn.
struct SyncAimPartMesh {
    bool required=false,known=false;
    std::vector<Vec3> fill,outline;
    // Quadrant-only rules constrain screen aim, not named anatomical joints.
    std::vector<Vec3> quadrantFill,quadrantOutline;
    std::vector<Vec3> lowerFill,lowerOutline;
};
enum class SyncAimDetail {None,WholeBody,Halves,Anatomy};
struct SyncAimColourRules {
    std::array<std::vector<std::string>,4> namedGroups,lowerGroups;
    uint32_t lowerAny=0;
};
static bool syncBodyGroupIsLeg(const std::string& group) {
    size_t at=0;
    while(at<group.size()){const auto end=group.find('_',at);const auto token=group.substr(at,end==std::string::npos?end:end-at);
        if(token=="leg"||token=="foot")return true;
        if(end==std::string::npos)break;at=end+1;
    }
    return false;
}
static bool syncBodyGroupIsLimb(const std::string& group) {
    size_t at=0;
    while(at<group.size()){const auto end=group.find('_',at);const auto token=group.substr(at,end==std::string::npos?end:end-at);
        for(const char* part:{"head","face","jaw","mouth","eye","brain","horn","mandible","arm","leg","foot","wing","tail"})if(token==part)return true;
        if(end==std::string::npos)break;at=end+1;
    }
    return false;
}
enum class SyncBodyAimKind {None,Specific,AllCurrentGroups,Unknown,Missing};
static SyncBodyAimKind syncBodyAimKind(const SyncAimRequirement* selected,const std::vector<SyncHitSphere>& spheres) {
    if(!selected||selected->damageGroups.empty())return SyncBodyAimKind::None;
    if(!selected->known||spheres.empty())return SyncBodyAimKind::Unknown;
    bool excluded=false,accepted=false;
    for(const auto& s:spheres){if(!s.focusKnown)return SyncBodyAimKind::Unknown;
        // A successful native lookup with no matching group is a known
        // non-target blocker, not a failure of the entire enemy's mapping.
        // It is not an additional anatomical group for all-groups rules.
        if(s.focusGroup.empty())continue;
        const bool matches=std::find(selected->damageGroups.begin(),selected->damageGroups.end(),s.focusGroup)!=selected->damageGroups.end();excluded|=!matches;accepted|=matches;}
    if(!accepted)return SyncBodyAimKind::Missing;
    return excluded?SyncBodyAimKind::Specific:SyncBodyAimKind::AllCurrentGroups;
}
struct SyncAimPartRect {double x0=0,y0=0,x1=0,y1=0;};
static bool syncAimPartRect(const GameCamera& camera,Vec3 low,Vec3 high,SyncAimPartRect& rect,double& front) {
    rect={DBL_MAX,DBL_MAX,-DBL_MAX,-DBL_MAX};front=DBL_MAX;
    std::array<Vec3,8> corners;std::array<double,8> depth{};bool projected=false;
    constexpr double nearDepth=.051;
    auto append=[&](Vec3 p){double x,y,w;if(!syncAimProject(camera,p,x,y,w))return;
        rect.x0=std::min(rect.x0,x);rect.x1=std::max(rect.x1,x);rect.y0=std::min(rect.y0,y);rect.y1=std::max(rect.y1,y);front=std::min(front,w);projected=true;};
    for(unsigned i=0;i<8;++i){const auto p=Vec3{(i&1)?high.x:low.x,(i&2)?high.y:low.y,(i&4)?high.z:low.z};corners[i]=p;
        const auto* m=camera.matrix;depth[i]=double(m[12])*p.x+double(m[13])*p.y+double(m[14])*p.z+m[15];
        if(!std::isfinite(depth[i]))return false;if(depth[i]>=nearDepth)append(p);}
    // Clip AABB edges to the near plane: a large enemy or a close side view
    // can straddle it while the required limb remains visibly in front.
    for(unsigned i=0;i<8;++i)for(unsigned bit:{1u,2u,4u})if(!(i&bit)){
        const auto j=i|bit;if((depth[i]>=nearDepth)==(depth[j]>=nearDepth))continue;
        const double t=(nearDepth-depth[i])/(depth[j]-depth[i]);
        append(motionAdd(corners[i],motionScale(subtract(corners[j],corners[i]),float(t))));}
    return projected;
}
static bool syncAimPartInterval(Vec3 eye,Vec3 ray,const SyncHitSphere& sphere,double& entry,double& leave) {
    const auto delta=subtract(eye,sphere.center);
    const double a=double(ray.x)*ray.x+double(ray.y)*ray.y+double(ray.z)*ray.z,
        b=double(delta.x)*ray.x+double(delta.y)*ray.y+double(delta.z)*ray.z,
        originDistance2=double(delta.x)*delta.x+double(delta.y)*delta.y+double(delta.z)*delta.z,inset=sphere.radius-.002;
    // Being inside the real sphere but outside its inset is not a visible
    // front hit. Treating that artificial inner surface as an entry can paint
    // torso colour over a different part reached before the real exit.
    if(originDistance2<=double(sphere.radius)*sphere.radius)return false;
    const double c=originDistance2-inset*inset,d=b*b-a*c;
    if(inset<=0||a<1e-12||d<=0||c<=0)return false;
    const double root=std::sqrt(d);entry=(-b-root)/a;leave=(-b+root)/a;return entry>0&&leave>entry;
}
struct SyncAimPartPlane {Vec3 normal{};double offset=0;};
static double syncAimPartTriangleDistance2(Vec3 point,Vec3 a,Vec3 b,Vec3 c) {
    using D=std::array<double,3>;
    auto sub=[](Vec3 x,Vec3 y)->D{return {double(x.x)-y.x,double(x.y)-y.y,double(x.z)-y.z};};
    auto dp=[](const D& x,const D& y){return x[0]*y[0]+x[1]*y[1]+x[2]*y[2];};
    const auto u=sub(b,a),v=sub(c,a),w=sub(point,a);
    const D n{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
    const double nn=dp(n,n),uu=dp(u,u),uv=dp(u,v),vv=dp(v,v),det=uu*vv-uv*uv;
    if(nn<1e-30||det<1e-30)return 0; // Degenerate faces cannot prove separation.
    const double height=dp(w,n)/nn;const D projected{w[0]-n[0]*height,w[1]-n[1]*height,w[2]-n[2]*height};
    const double pu=dp(projected,u),pv=dp(projected,v),s=(pu*vv-pv*uv)/det,t=(pv*uu-pu*uv)/det;
    if(s>=0&&t>=0&&s+t<=1)return height*height*nn;
    double best=DBL_MAX;
    for(const auto& edge:{std::make_pair(a,b),std::make_pair(b,c),std::make_pair(c,a)}){
        const auto d=sub(edge.second,edge.first),p=sub(point,edge.first);const double length=dp(d,d);
        const double f=length>1e-30?std::clamp(dp(p,d)/length,0.,1.):0.;
        const D distance{p[0]-f*d[0],p[1]-f*d[1],p[2]-f*d[2]};best=std::min(best,dp(distance,distance));
    }return best;
}
static bool syncAimPartPlane(Vec3 a,Vec3 b,Vec3 c,Vec3 inside,SyncAimPartPlane& plane) {
    const double u[]{double(b.x)-a.x,double(b.y)-a.y,double(b.z)-a.z},v[]{double(c.x)-a.x,double(c.y)-a.y,double(c.z)-a.z};
    const double nx=u[1]*v[2]-u[2]*v[1],ny=u[2]*v[0]-u[0]*v[2],nz=u[0]*v[1]-u[1]*v[0],length=sqrt(nx*nx+ny*ny+nz*nz);
    if(length<1e-12||!std::isfinite(length))return false;Vec3 normal{float(nx/length),float(ny/length),float(nz/length)};
    double offset=double(normal.x)*a.x+double(normal.y)*a.y+double(normal.z)*a.z;
    if(double(normal.x)*inside.x+double(normal.y)*inside.y+double(normal.z)*inside.z>offset){normal=motionScale(normal,-1);offset=-offset;}plane={normal,offset};return true;
}
// All four cell corners enter the SAME inset sphere at a common ray parameter.
// Convexity then covers every ray inside the cell, not just sampled points.
// Every other-group sphere must be separated from the whole eye-to-cell
// pyramid. Face-plane rejection is fast; exact face distance also handles
// spheres outside an edge/corner, without hiding visible legs or colour rims.
static bool syncAimPartCell(Vec3 eye,const Vec3 (&rays)[4],const std::vector<SyncHitSphere>& spheres,
    const std::vector<unsigned char>& wanted,const std::vector<SyncAimPartRect>& rects,const SyncAimPartRect& cell,Vec3 depthNormal) {
    std::array<SyncAimPartPlane,5> planes{};const auto middle=motionScale(motionAdd(motionAdd(rays[0],rays[1]),motionAdd(rays[2],rays[3])),.25f);
    for(unsigned j=0;j<4;++j)if(!syncAimPartPlane(eye,motionAdd(eye,rays[j]),motionAdd(eye,rays[(j+1)%4]),motionAdd(eye,middle),planes[j]))return false;
    auto overlaps=[&](size_t i){return rects[i].x0<=rects[i].x1&&rects[i].y0<=rects[i].y1&&rects[i].x1>=cell.x0&&rects[i].x0<=cell.x1&&rects[i].y1>=cell.y0&&rects[i].y0<=cell.y1;};
    for(size_t i=0;i<spheres.size();++i)if(wanted[i]&&overlaps(i)){
        double enter=0,leave=DBL_MAX;bool covered=true;
        for(auto ray:rays){double a,b;if(!syncAimPartInterval(eye,ray,spheres[i],a,b)){covered=false;break;}enter=std::max(enter,a);leave=std::min(leave,b);}
        if(!covered||enter>=leave)continue;const double t=enter+(leave-enter)*.001;
        // The four rays share a camera-depth slice. Derive its far cap from
        // that known normal instead of a cross product of tiny far triangles.
        double depth=0;for(auto ray:rays)depth=std::max(depth,t*(double(depthNormal.x)*ray.x+double(depthNormal.y)*ray.y+double(depthNormal.z)*ray.z));
        if(depth<=0)continue;planes[4]={depthNormal,double(depthNormal.x)*eye.x+double(depthNormal.y)*eye.y+double(depthNormal.z)*eye.z+depth+.00002};
        bool clear=true;
        std::array<Vec3,5> pyramid{};bool pyramidReady=false;
        for(size_t k=0;k<spheres.size();++k)if(!wanted[k]&&overlaps(k)){
            bool separated=false;for(const auto& plane:planes)if(dot(plane.normal,spheres[k].center)-plane.offset>spheres[k].radius+.003){separated=true;break;}
            if(!separated){bool inside=true;for(const auto& plane:planes)inside&=dot(plane.normal,spheres[k].center)<=plane.offset;
                if(!inside){
                    if(!pyramidReady){pyramidReady=true;for(unsigned j=0;j<4;++j){const double along=dot(depthNormal,rays[j]);
                        if(along<=0){pyramidReady=false;break;}pyramid[j+1]=motionScale(rays[j],float((depth+.00002)/along));}}
                    if(pyramidReady){const auto center=subtract(spheres[k].center,eye);double distance=DBL_MAX;
                        for(unsigned j=0;j<4;++j)distance=std::min(distance,syncAimPartTriangleDistance2(center,pyramid[0],pyramid[j+1],pyramid[(j+1)%4+1]));
                        distance=std::min({distance,syncAimPartTriangleDistance2(center,pyramid[1],pyramid[2],pyramid[3]),syncAimPartTriangleDistance2(center,pyramid[1],pyramid[3],pyramid[4])});
                        separated=distance>double(spheres[k].radius+.003)*(spheres[k].radius+.003);
                    }
                }
            }
            if(!separated){clear=false;break;}}
        if(clear)return true;
    }
    return false;
}
static SyncAimPartMesh syncBuildAimPartMesh(const GameCamera& camera,const float* bounds,const SyncAimFrame& aim,
    const SyncAimRequirement* selected,const std::vector<SyncHitSphere>& spheres,bool concentrate=true,bool bodyQuadrants=false,SyncAimDetail detail=SyncAimDetail::None,
    const SyncAimColourRules* colourRules=nullptr,bool retryColours=true) {
    SyncAimPartMesh result;const auto bodyKind=syncBodyAimKind(selected,spheres);
    const bool colored=detail!=SyncAimDetail::None;
    const bool wholeBody=bodyQuadrants&&selected&&(colored||!syncAimUnrestricted(selected->quadrants))&&
        (bodyKind==SyncBodyAimKind::None||bodyKind==SyncBodyAimKind::AllCurrentGroups);
    if(!wholeBody&&(bodyKind==SyncBodyAimKind::None||bodyKind==SyncBodyAimKind::AllCurrentGroups))return result;
    result.required=!wholeBody;if(bodyKind==SyncBodyAimKind::Unknown)return result;
    if(bodyKind==SyncBodyAimKind::Missing){result.known=true;return result;}
    if(!selected->known||!syncAimMaskKnown(selected->quadrants)||!syncAimBoundsValid(bounds)||spheres.empty()||spheres.size()>128||
       (!syncAimUnrestricted(selected->quadrants)&&(!aim.known||!syncAimBoundsValid(aim.bounds))))return result;
    std::vector<unsigned char> wanted,limbs,torso;std::vector<SyncAimPartRect> rects;SyncAimPartRect box;double front;
    if(!syncAimPartRect(camera,{bounds[0],bounds[1],bounds[2]},{bounds[3],bounds[4],bounds[5]},box,front))return result;
    Vec3 eye{},forward{};if(!inspectorRay(camera,eye,forward))return result;
    Vec3 depthNormal{camera.matrix[12],camera.matrix[13],camera.matrix[14]};const float depthLength=sqrtf(dot(depthNormal,depthNormal));
    if(!std::isfinite(depthLength)||depthLength<1e-6f)return result;depthNormal=motionScale(depthNormal,1/depthLength);
    for(const auto& sphere:spheres){if(!syncRangeFinite(sphere.center)||!std::isfinite(sphere.radius)||sphere.radius<=0)return result;
        const Vec3 extent{sphere.radius,sphere.radius,sphere.radius};SyncAimPartRect rect;double w;
        if(!syncAimPartRect(camera,subtract(sphere.center,extent),motionAdd(sphere.center,extent),rect,w)){
            const auto* m=camera.matrix;const double maximumDepth=double(m[12])*sphere.center.x+double(m[13])*sphere.center.y+double(m[14])*sphere.center.z+m[15]+
                sphere.radius*(fabs(double(m[12]))+fabs(double(m[13]))+fabs(double(m[14])));
            if(!std::isfinite(maximumDepth))return result;
            // Fully behind the eye cannot block a forward ray. A sphere in
            // the near-plane gap remains a conservative full-screen blocker.
            rect=maximumDepth<=0?SyncAimPartRect{1,1,-1,-1}:SyncAimPartRect{-DBL_MAX,-DBL_MAX,DBL_MAX,DBL_MAX};}
        rects.push_back(rect);wanted.push_back((wholeBody&&selected->damageGroups.empty())||
            (sphere.focusKnown&&std::find(selected->damageGroups.begin(),selected->damageGroups.end(),sphere.focusGroup)!=selected->damageGroups.end()));
        if(!sphere.focusKnown&&(!selected->damageGroups.empty()||detail==SyncAimDetail::Anatomy))return result;
        limbs.push_back(wanted.back()&&sphere.focusKnown&&syncBodyGroupIsLimb(sphere.focusGroup));
        torso.push_back(wanted.back()&&sphere.focusKnown&&!sphere.focusGroup.empty()&&!syncBodyGroupIsLimb(sphere.focusGroup));}
    result.known=true;if(std::find(wanted.begin(),wanted.end(),1)==wanted.end())return result;
    box.x0=std::max(-1.,box.x0);box.x1=std::min(1.,box.x1);box.y0=std::max(-1.,box.y0);box.y1=std::min(1.,box.y1);
    if(box.x0>=box.x1||box.y0>=box.y1)return result;
    const double boxWidth=box.x1-box.x0,boxHeight=box.y1-box.y0;SyncAimPartRect wantedBox{DBL_MAX,DBL_MAX,-DBL_MAX,-DBL_MAX};
    if(concentrate){for(size_t i=0;i<wanted.size();++i)if(wanted[i]&&rects[i].x0<=rects[i].x1&&rects[i].y0<=rects[i].y1){
        wantedBox.x0=std::min(wantedBox.x0,rects[i].x0);wantedBox.x1=std::max(wantedBox.x1,rects[i].x1);
        wantedBox.y0=std::min(wantedBox.y0,rects[i].y0);wantedBox.y1=std::max(wantedBox.y1,rects[i].y1);}
    box.x0=std::max(box.x0,wantedBox.x0);box.x1=std::min(box.x1,wantedBox.x1);box.y0=std::max(box.y0,wantedBox.y0);box.y1=std::min(box.y1,wantedBox.y1);
    if(box.x0>=box.x1||box.y0>=box.y1)return result;}
    double centerX=0,centerY=0,w,errorX=1e-6,errorY=1e-6;
    if(!syncAimUnrestricted(selected->quadrants)||detail==SyncAimDetail::Halves||colourRules){const auto* b=aim.bounds;
        if(!aim.known||!syncAimBoundsValid(b)){result.known=false;return result;}
        const Vec3 center{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f};
        if(!syncAimProject(camera,center,centerX,centerY,w)){result.known=false;return result;}
        const auto* m=camera.matrix;
        errorX+=8*FLT_EPSILON*(fabs(double(m[0])*center.x)+fabs(double(m[1])*center.y)+fabs(double(m[2])*center.z)+fabs(m[3]))/w;
        errorY+=8*FLT_EPSILON*(fabs(double(m[4])*center.x)+fabs(double(m[5])*center.y)+fabs(double(m[6])*center.z)+fabs(m[7]))/w;}
    // Spend samples on the required part, including small hands and heads in
    // side views, rather than an entire large demon's empty screen rectangle.
    const double maxColumns=colored?108.:36.,maxRows=colored?144.:48.;
    const unsigned columns=unsigned(std::clamp(ceil(maxColumns*(box.x1-box.x0)/boxWidth),colored?24.:8.,maxColumns)),
        rows=unsigned(std::clamp(ceil(maxRows*(box.y1-box.y0)/boxHeight),colored?24.:8.,maxRows));
    auto divisions=[](double low,double high,unsigned count,double center,double error){std::vector<double> values;for(unsigned i=0;i<=count;++i)values.push_back(low+(high-low)*i/count);
        for(double extra:{center-2*error,center,center+2*error})if(extra>low+1e-8&&extra<high-1e-8)values.push_back(extra);
        std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end(),[](double a,double b){return fabs(a-b)<1e-8;}),values.end());return values;};
    const auto xs=divisions(box.x0,box.x1,columns,centerX,errorX),ys=divisions(box.y0,box.y1,rows,centerY,errorY);const size_t nx=xs.size()-1,ny=ys.size()-1;
    std::vector<unsigned char> cells(nx*ny);std::vector<Vec3> points((nx+1)*(ny+1));front=std::max(.051,front-.01);
    auto point=[&](size_t x,size_t y)->Vec3&{return points[y*(nx+1)+x];};
    for(size_t y=0;y<=ny;++y)for(size_t x=0;x<=nx;++x)if(!syncAimUnproject(camera,xs[x],ys[y],front,point(x,y))){result.known=false;return result;}
    // Keep colour provenance before the availability union. Otherwise a
    // generic lower-screen interaction is lost beneath a blue torso mask.
    std::array<std::array<std::vector<unsigned char>,3>,16> colours;
    std::array<std::array<bool,3>,16> colourAny{};
    if(colourRules){std::array<std::vector<unsigned char>,4> kinds;
        for(unsigned q=0;q<4;++q)for(size_t i=0;i<spheres.size();++i){const auto& s=spheres[i];unsigned char kind=0;
            if(wanted[i]){
                const bool named=s.focusKnown&&std::find(colourRules->namedGroups[q].begin(),colourRules->namedGroups[q].end(),s.focusGroup)!=colourRules->namedGroups[q].end();
                const bool lower=(colourRules->lowerAny&(1u<<q))||(s.focusKnown&&std::find(colourRules->lowerGroups[q].begin(),colourRules->lowerGroups[q].end(),s.focusGroup)!=colourRules->lowerGroups[q].end());
                kind=s.focusKnown&&syncBodyGroupIsLeg(s.focusGroup)?3:named&&syncBodyGroupIsLimb(s.focusGroup)?1:lower?3:2;
            }kinds[q].push_back(kind);
        }
        for(unsigned mask=1;mask<16;++mask)for(unsigned kind=1;kind<=3;++kind)for(size_t i=0;i<spheres.size();++i){bool common=true;
            for(unsigned q=0;q<4;++q)if((mask&(1u<<q))&&kinds[q][i]!=kind)common=false;
            colours[mask][kind-1].push_back(common);
            colourAny[mask][kind-1]|=common;
        }
    }
    auto classify=[&](const SyncAimPartRect& cell,const Vec3 (&rays)[4])->unsigned char {
        const double cx=(cell.x0+cell.x1)*.5,cy=(cell.y0+cell.y1)*.5;
        const unsigned quadrant=(cy>centerY)?(cx<centerX?1u:2u):(cx<centerX?4u:8u);
        uint32_t possible=quadrant;
        if(!syncAimUnrestricted(selected->quadrants)||colourRules){
            if(!syncAimUnrestricted(selected->quadrants)&&!(selected->quadrants&quadrant))return 0;
            possible=0;for(double py:{cell.y0,cell.y1})for(double px:{cell.x0,cell.x1}){
                if(px<=centerX+errorX&&py>=centerY-errorY)possible|=1;if(px>=centerX-errorX&&py>=centerY-errorY)possible|=2;
                if(px<=centerX+errorX&&py<=centerY+errorY)possible|=4;if(px>=centerX-errorX&&py<=centerY+errorY)possible|=8;}
            if(!syncAimUnrestricted(selected->quadrants)&&(possible&~selected->quadrants))return 0;}
        if(colourRules){
            if(!possible)return 0;
            for(unsigned kind:{3u,1u,2u})if(colourAny[possible][kind-1]&&syncAimPartCell(eye,rays,spheres,colours[possible][kind-1],rects,cell,depthNormal))return static_cast<unsigned char>(kind);
        }else if(detail==SyncAimDetail::Anatomy){
            if(syncAimPartCell(eye,rays,spheres,limbs,rects,cell,depthNormal))return 1;
            if(syncAimPartCell(eye,rays,spheres,torso,rects,cell,depthNormal))return 2;
        }else if(syncAimPartCell(eye,rays,spheres,wanted,rects,cell,depthNormal))
            return colored?(detail==SyncAimDetail::Halves&&cy<centerY?3:2):1;
        return 0;
    };
    for(size_t y=0;y<ny;++y)for(size_t x=0;x<nx;++x){
        const Vec3 rays[]{subtract(point(x,y),eye),subtract(point(x+1,y),eye),subtract(point(x+1,y+1),eye),subtract(point(x,y+1),eye)};
        cells[y*nx+x]=classify({xs[x],ys[y],xs[x+1],ys[y+1]},rays);
    }
    for(size_t y=0;y<ny;++y)for(size_t x=0;x<nx;++x)if(cells[y*nx+x]){
        const Vec3 p[]{point(x,y),point(x+1,y),point(x+1,y+1),point(x,y+1)};
        const auto kind=cells[y*nx+x];auto& fill=kind==3?result.lowerFill:kind==2?result.quadrantFill:result.fill;
        auto& outline=kind==3?result.lowerOutline:kind==2?result.quadrantOutline:result.outline;
        for(unsigned j:{0u,1u,2u,0u,2u,3u})fill.push_back(p[j]);
        auto edge=[&](unsigned a,unsigned b){outline.push_back(p[a]);outline.push_back(p[b]);};
        if(y==0||cells[(y-1)*nx+x]!=kind)edge(0,1);if(x+1==nx||cells[y*nx+x+1]!=kind)edge(1,2);
        if(y+1==ny||cells[(y+1)*nx+x]!=kind)edge(2,3);if(x==0||cells[y*nx+x-1]!=kind)edge(3,0);}
    // A differently aligned small-part grid can miss a tiny exposed sliver.
    // Retry the original whole-box grid once, under the identical proof tests.
    if(colourRules&&retryColours){
        // A small exposed foot/head can fall between the whole-body grid
        // lines. Retry a missing colour on its own native groups, under the
        // same quadrant and first-hit proofs; never enlarge the accepted area.
        for(unsigned kind=1;kind<=3;++kind){auto& fill=kind==3?result.lowerFill:kind==2?result.quadrantFill:result.fill;
            auto& outline=kind==3?result.lowerOutline:kind==2?result.quadrantOutline:result.outline;
            if(!fill.empty())continue;
            for(unsigned q=0;q<4;++q){const size_t slot=size_t(1)<<q;if(!syncAimMatches(*selected,1u<<q)||!colourAny[slot][kind-1])continue;
                SyncAimRequirement focused;focused.quadrants=1u<<q;
                for(size_t i=0;i<spheres.size();++i)if(colours[slot][kind-1][i]&&spheres[i].focusKnown&&!spheres[i].focusGroup.empty()&&
                    std::find(focused.damageGroups.begin(),focused.damageGroups.end(),spheres[i].focusGroup)==focused.damageGroups.end())focused.damageGroups.push_back(spheres[i].focusGroup);
                if(focused.damageGroups.empty())continue;
                const auto extra=syncBuildAimPartMesh(camera,bounds,aim,&focused,spheres,true,true,detail,colourRules,false);
                if(!extra.known)continue;const auto& addFill=kind==3?extra.lowerFill:kind==2?extra.quadrantFill:extra.fill;
                const auto& addOutline=kind==3?extra.lowerOutline:kind==2?extra.quadrantOutline:extra.outline;
                fill.insert(fill.end(),addFill.begin(),addFill.end());outline.insert(outline.end(),addOutline.begin(),addOutline.end());
            }
        }
    }
    if(concentrate&&result.fill.empty()&&result.quadrantFill.empty()&&result.lowerFill.empty())return syncBuildAimPartMesh(camera,bounds,aim,selected,spheres,false,bodyQuadrants,detail,colourRules,false);
    if(wholeBody&&!colored){result.quadrantFill=std::move(result.fill);result.quadrantOutline=std::move(result.outline);}
    return result;
}
static void appendSyncAimPartMesh(const SyncAimPartMesh& mesh,std::vector<Vertex>& output,bool lines) {
    if(!mesh.known)return;const std::array<float,4> fill{1,.48f,.08f,.28f},outline{1,.62f,.12f,.95f};
    for(auto point:lines?mesh.outline:mesh.fill){Vertex vertex{};vertex.position=point;memcpy(vertex.color,(lines?outline:fill).data(),16);output.push_back(vertex);}
    const std::array<float,4> quadrantColor{.15f,.45f,1,lines?.9f:.24f};
    for(auto point:lines?mesh.quadrantOutline:mesh.quadrantFill){Vertex vertex{};vertex.position=point;memcpy(vertex.color,quadrantColor.data(),16);output.push_back(vertex);}
    const std::array<float,4> lowerColor{.65f,.22f,1,lines?.9f:.28f};
    for(auto point:lines?mesh.lowerOutline:mesh.lowerFill){Vertex vertex{};vertex.position=point;memcpy(vertex.color,lowerColor.data(),16);output.push_back(vertex);}
}
