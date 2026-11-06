#pragma once

// 1.1 playerRequirements.focalScreenQuadrants, consumed by 16bb2b2 and
// idPlayer's 16ce6b0 helper. These are aim quadrants in screen space, NOT
// fixed front/back sectors or anatomical damage groups.
static bool syncAimMaskKnown(uint32_t mask){return mask<=15||mask==0x7fffffffu;}
static bool syncAimUnrestricted(uint32_t mask){return mask==0||mask==15||mask==0x7fffffffu;}
static bool syncAimMatches(const SyncAimRequirement& rule,uint32_t quadrant){return rule.known&&syncAimMaskKnown(rule.quadrants)&&(syncAimUnrestricted(rule.quadrants)||(rule.quadrants&quadrant)!=0);}
// The renderer matrix can round differently from the engine's two-matrix
// projection. Require the selected rule to accept every possible side in the
// numerical centre-line band; a competitor needs only one possible side.
static bool syncAimDefinitelyMatches(const SyncAimRequirement& rule,uint32_t quadrant){return rule.known&&syncAimMaskKnown(rule.quadrants)&&(syncAimUnrestricted(rule.quadrants)||(quadrant&&(quadrant&~rule.quadrants)==0));}
static std::string syncAimMaskLabel(uint32_t mask) {
    if(syncAimUnrestricted(mask))return "no aim-quadrant restriction";
    if(!syncAimMaskKnown(mask))return "unsupported aim rule";
    std::string text;for(const auto& entry:std::array<std::pair<uint32_t,const char*>,4>{{{1,"top left"},{2,"top right"},{4,"bottom left"},{8,"bottom right"}}})
        if(mask&entry.first){if(!text.empty())text+=" / ";text+=entry.second;}return text;
}
static bool syncAimBoundsValid(const float* bounds){for(unsigned i=0;i<6;++i)if(!std::isfinite(bounds[i])||fabsf(bounds[i])>1e6f)return false;
    for(unsigned i=0;i<3;++i)if(bounds[i]>=bounds[i+3])return false;return true;}
static bool syncAimProject(const GameCamera& camera,Vec3 point,double& x,double& y,double& w) {
    const auto* m=camera.matrix;w=double(m[12])*point.x+double(m[13])*point.y+double(m[14])*point.z+m[15];
    if(!std::isfinite(w)||w<=.05)return false;
    x=(double(m[0])*point.x+double(m[1])*point.y+double(m[2])*point.z+m[3])/w;
    y=(double(m[4])*point.x+double(m[5])*point.y+double(m[6])*point.z+m[7])/w;return std::isfinite(x)&&std::isfinite(y);
}
static uint32_t syncAimQuadrant(const GameCamera& camera,const float* bounds) {
    if(!syncAimBoundsValid(bounds))return 0;Vec3 center{(bounds[0]+bounds[3])*.5f,(bounds[1]+bounds[4])*.5f,(bounds[2]+bounds[5])*.5f};double x,y,w;
    if(!syncAimProject(camera,center,x,y,w))return 0;
    // Native uses >= / <= on both axes, so equality admits both neighbours.
    const auto* m=camera.matrix;const double errorX=1e-7+8*FLT_EPSILON*(fabs(double(m[0])*center.x)+fabs(double(m[1])*center.y)+fabs(double(m[2])*center.z)+fabs(m[3]))/w;
    const double errorY=1e-7+8*FLT_EPSILON*(fabs(double(m[4])*center.x)+fabs(double(m[5])*center.y)+fabs(double(m[6])*center.z)+fabs(m[7]))/w;
    uint32_t mask=0;if(x>=-errorX&&y>=-errorY)mask|=4;if(x<=errorX&&y>=-errorY)mask|=8;
    if(x>=-errorX&&y<=errorY)mask|=1;if(x<=errorX&&y<=errorY)mask|=2;return mask;
}
static bool syncAimUnproject(const GameCamera& camera,double x,double y,double w,Vec3& point) {
    const auto* m=camera.matrix;const Vec3 a{m[0],m[1],m[2]},b{m[4],m[5],m[6]},c{m[12],m[13],m[14]};
    const auto bc=cross(b,c),ca=cross(c,a),ab=cross(a,b);const double determinant=dot(a,bc);if(!std::isfinite(determinant)||fabs(determinant)<1e-10)return false;
    const double u=x*w-m[3],v=y*w-m[7],t=w-m[15];
    point={float((bc.x*u+ca.x*v+ab.x*t)/determinant),float((bc.y*u+ca.y*v+ab.y*t)/determinant),float((bc.z*u+ca.z*v+ab.z*t)/determinant)};
    return syncRangeFinite(point);
}
// The wire box encloses the actual shot spheres used for target selection.
// The optional highlighted aim tiles use the engine's PHYSICS bounds centre,
// which need not coincide with the centre of that shot-box projection.
static void appendSyncAimBox(const GameCamera& camera,const float* shotBounds,const SyncAimFrame& aim,const SyncAimRequirement* selected,std::vector<Vertex>& output,bool lines) {
    if(!syncAimBoundsValid(shotBounds))return;
    const std::array<float,4> wire{.7f,.9f,1,.85f},crossTint{1,.85f,.25f,.8f},tileTint{.25f,.8f,1,.065f};
    auto append=[&](Vec3 p,const auto& color){Vertex vertex{};vertex.position=p;memcpy(vertex.color,color.data(),16);output.push_back(vertex);};
    Vec3 corners[8];double minX=DBL_MAX,minY=DBL_MAX,maxX=-DBL_MAX,maxY=-DBL_MAX,front=DBL_MAX;
    constexpr double nearW=.051;double depth[8]{};
    auto project=[&](Vec3 p){double x,y,w;if(!syncAimProject(camera,p,x,y,w))return;
        minX=std::min(minX,x);maxX=std::max(maxX,x);minY=std::min(minY,y);maxY=std::max(maxY,y);front=std::min(front,w);};
    for(unsigned i=0;i<8;++i){corners[i]={shotBounds[(i&1)?3:0],shotBounds[(i&2)?4:1],shotBounds[(i&4)?5:2]};
        const auto* m=camera.matrix;depth[i]=double(m[12])*corners[i].x+double(m[13])*corners[i].y+double(m[14])*corners[i].z+m[15];
        if(depth[i]>=nearW)project(corners[i]);}
    for(unsigned i=0;i<8;++i)for(unsigned bit:{1u,2u,4u})if(!(i&bit)&&((depth[i]>=nearW)!=(depth[i|bit]>=nearW))){
        const double t=(nearW-depth[i])/(depth[i|bit]-depth[i]);const auto delta=subtract(corners[i|bit],corners[i]);
        project(motionAdd(corners[i],motionScale(delta,float(t))));}
    if(lines)for(unsigned i=0;i<8;++i)for(unsigned bit:{1u,2u,4u})if(!(i&bit)){append(corners[i],wire);append(corners[i|bit],wire);}
    if(minX==DBL_MAX||!aim.known||!selected||!selected->known||syncAimUnrestricted(selected->quadrants)||!syncAimMaskKnown(selected->quadrants))return;
    const auto* b=aim.bounds;double centerX,centerY,w;if(!syncAimProject(camera,{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},centerX,centerY,w))return;
    minX=std::max(-1.,minX);maxX=std::min(1.,maxX);minY=std::max(-1.,minY);maxY=std::min(1.,maxY);
    if(minX>=maxX||minY>=maxY)return;
    centerX=std::clamp(centerX,minX,maxX);centerY=std::clamp(centerY,minY,maxY);
    front=std::max(.051,front-.01);
    auto point=[&](double x,double y,Vec3& p){return syncAimUnproject(camera,x,y,front,p);};
    if(!selected->damageGroups.empty())return; // Anatomical mask intersects this quadrant instead of filling the whole tile.
    if(lines){Vec3 a,bp;if(point(minX,centerY,a)&&point(maxX,centerY,bp)){append(a,crossTint);append(bp,crossTint);}
        if(point(centerX,minY,a)&&point(centerX,maxY,bp)){append(a,crossTint);append(bp,crossTint);}return;}
    for(unsigned quadrant:{1u,2u,4u,8u})if(selected->quadrants&quadrant){const bool right=(quadrant&10)!=0,top=(quadrant&3)!=0;
        const double left=right?centerX:minX,rightX=right?maxX:centerX,bottom=top?centerY:minY,topY=top?maxY:centerY;Vec3 p[4];
        if(rightX<=left||topY<=bottom)continue;
        if(point(left,bottom,p[0])&&point(rightX,bottom,p[1])&&point(rightX,topY,p[2])&&point(left,topY,p[3]))for(unsigned i:{0u,1u,2u,0u,2u,3u})append(p[i],tileTint);}
}
