#pragma once

// Enemy-anchored position diagram at the player's current foot height and
// current aim scope. Runtime selector sites are explicitly snapshot data;
// authored GK sites are reference poses, never an activation promise.
enum class SyncDistanceTone : unsigned char {Outside,Single,Random,Unknown,Boundary};
struct SyncDistanceRule {
    std::string name;size_t animation=0;std::vector<SyncRangeVolume> volumes;
    bool conditionKnown=true,priorityKnown=false,siteKnown=false,observed=false;
    float priority=0;Vec3 site{};
};
struct SyncDistanceRules {
    bool known=false,selectorKnown=true,authored=false;
    float z=0;double tolerance=0;
    std::vector<std::string> names;std::vector<SyncDistanceRule> rules;
    std::string detail;
};
struct SyncDistanceState {
    uint64_t available=0,possible=0;
    SyncDistanceTone tone=SyncDistanceTone::Outside;
};
struct SyncDistanceMap {
    bool known=false,authored=false;uint64_t signature=0;float z=0;
    double precision=0;std::string detail;std::vector<std::string> names;
    std::vector<Vertex> fill,lines;
};
static bool syncDistanceAim(const SyncAimRequirement& aim,const SyncTargetFrame& value,bool& known) {
    known=aim.known&&syncAimMaskKnown(aim.quadrants);
    if(!known)return true;
    const auto& reference=value.retainedTarget?value.retainedAim:value.aim;
    const auto& focus=value.retainedTarget?value.retainedFocus:value.jointFocus;
    if(!syncAimUnrestricted(aim.quadrants)){
        if(!reference.known||!reference.quadrant)known=false;
        else if(!syncAimMatches(aim,reference.quadrant))return false;
        else if(!syncAimDefinitelyMatches(aim,reference.quadrant))known=false;
    }
    if(!aim.damageGroups.empty()){
        if(!focus.known)known=false;else if(!syncFocusMatches(aim,focus))return false;
    }return true;
}
static SyncDistanceRules syncReadDistanceRules(const SyncTargetFrame& value) {
    SyncDistanceRules out;out.z=value.ranges.playerOrigin.z;out.tolerance=value.selectionTolerance;
    if(!value.rulesCoherent||!value.ranges.known||!syncFacingOrthonormal(value.facing.axis)||!syncRangeFinite(value.ranges.playerOrigin)){
        out.detail="Distance boundaries: waiting for coherent range/pose data";return out;
    }
    const auto limits=syncTargetReferenceLimits(value.facing,value.ranges);
    out.selectorKnown=std::isfinite(out.tolerance)&&out.tolerance>0&&out.tolerance<=10;
    auto append=[&](SyncDistanceRule rule){
        const auto found=std::find(out.names.begin(),out.names.end(),rule.name);
        rule.animation=found==out.names.end()?out.names.size():size_t(found-out.names.begin());
        if(found==out.names.end())out.names.push_back(rule.name);
        if(rule.animation<32)out.rules.push_back(std::move(rule));
    };
    if(value.action==0){
        if(value.regionState!="ready"){out.detail="Distance boundaries: incomplete native GK rules";return out;}
        for(const auto& region:value.regions){bool conditionKnown;
            if(!syncDistanceAim(region.aim,value,conditionKnown))continue;
            SyncRangeVolume volume;if(!syncRangeBuild(&region,value.facing,limits,false,value.incomingYaw,volume,false)){
                out.detail="Distance boundaries: unsupported GK branch";return out;
            }
            SyncDistanceRule rule;rule.name=region.name;rule.conditionKnown=conditionKnown;rule.volumes=syncRangeConvexParts(volume);
            rule.priorityKnown=region.priorityKnown;rule.priority=region.priority;
            rule.site=motionAdd(value.facing.origin,syncRangeTransform(value.facing,region.start));rule.siteKnown=syncRangeFinite(rule.site);
            // A unique prepared interaction can use its actual adjusted site.
            // Same-name branches cannot borrow an arbitrary runtime instance.
            size_t authored=0,native=0;const SyncNativeChoice* choice=nullptr;
            for(const auto& r:value.regions)authored+=r.name==region.name;
            for(const auto& r:value.nativeChoices)if(r.action==0&&r.name==region.name){++native;choice=&r;}
            if(value.selectionKnown&&authored==1&&native==1&&choice&&choice->known){
                rule.priorityKnown=true;rule.priority=choice->priority;rule.site=choice->position;rule.siteKnown=true;rule.observed=true;
            }else out.authored=true;
            append(std::move(rule));
        }
    }else if(value.hasChainsaw){
        SyncRangeVolume volume;if(!syncRangeBuild(nullptr,value.facing,limits,true,value.incomingYaw,volume,false))return out;
        // Keep every native branch, even when several have the same clip name
        // but different aim scopes, priorities or adjusted selector sites.
        for(const auto& r:value.chainsawRules){SyncDistanceRule rule;rule.name=r.name;rule.volumes={volume};
            if(!syncDistanceAim(r.aim,value,rule.conditionKnown))continue;
            rule.priorityKnown=rule.siteKnown=value.selectionKnown&&r.selectionKnown&&r.quick;rule.conditionKnown&=r.quick;
            rule.priority=r.priority;rule.site=r.selectionPosition;rule.observed=rule.siteKnown;append(std::move(rule));}
        for(const auto& name:value.chainsawAnimations)if(std::none_of(value.chainsawRules.begin(),value.chainsawRules.end(),[&](const auto& r){return r.name==name;})&&!syncChainsawAlternativeCovered(value,name)){
            SyncDistanceRule rule;rule.name=name;rule.volumes={volume};rule.conditionKnown=false;append(std::move(rule));
        }
    }
    out.known=out.names.size()<=32;out.detail=out.authored?"Distance map: authored GK reference poses":"Distance map: current native selector snapshot";return out;
}
// -1 outside, 0 crosses a boundary, +1 wholly inside. All bounds enclose
// every point in the cell, rather than trusting corner/centre sampling.
static int syncDistanceVolumeCell(const SyncRangeVolume& v,Vec3 point,double half) {
    bool inside=true;const double cellRadius=half*1.4142135623730951;
    for(const auto& plane:v.planes){const double d=double(plane.n.x)*point.x+double(plane.n.y)*point.y+double(plane.n.z)*point.z+plane.d;
        const double extent=(fabs(plane.n.x)+fabs(plane.n.y))*half;
        if(d-extent>1e-7)return -1;if(d+extent> -1e-7)inside=false;
    }
    for(const auto& ball:v.balls){const auto delta=subtract(point,ball.center);
        const double d=sqrt(double(delta.x)*delta.x+double(delta.y)*delta.y+(ball.horizontal?0:double(delta.z)*delta.z));
        if(d-cellRadius>ball.radius+1e-7)return -1;if(d+cellRadius>ball.radius-1e-7)inside=false;
    }return inside?1:0;
}
static SyncDistanceState syncDistanceState(const SyncDistanceRules& rules,Vec3 point,double half=0) {
    SyncDistanceState out;std::vector<const SyncDistanceRule*> active;uint64_t uncertain=0;bool conditionKnown=true;
    for(const auto& rule:rules.rules){int status=-1;
        for(const auto& volume:rule.volumes){const int v=syncDistanceVolumeCell(volume,point,half);if(v>0){status=1;break;}if(v==0)status=0;}
        const uint64_t bit=uint64_t(1)<<rule.animation;
        if(status>0){out.available|=bit;active.push_back(&rule);conditionKnown&=rule.conditionKnown;}else if(status==0)uncertain|=bit;
    }
    // Another branch of the same clip may carry a different priority/site.
    // Its uncertain eligibility can still change the selector competition.
    if(uncertain){out.tone=SyncDistanceTone::Boundary;return out;}
    if(active.empty())return out;
    if(!conditionKnown){out.tone=SyncDistanceTone::Unknown;return out;}
    if((out.available&(out.available-1))==0){out.possible=out.available;out.tone=SyncDistanceTone::Single;return out;}
    for(const auto* rule:active)if(!rule->priorityKnown||!std::isfinite(rule->priority)){out.tone=SyncDistanceTone::Unknown;return out;}
    float best=-FLT_MAX;for(const auto* rule:active)best=std::max(best,rule->priority);
    uint64_t highest=0;for(const auto* rule:active)if(rule->priority==best)highest|=uint64_t(1)<<rule->animation;
    if((highest&(highest-1))==0){out.possible=highest;out.tone=SyncDistanceTone::Single;return out;}
    if(!rules.selectorKnown){out.tone=SyncDistanceTone::Unknown;return out;}
    for(const auto* rule:active)if(rule->priority==best){
        if(!rule->siteKnown||!syncRangeFinite(rule->site)){out.tone=SyncDistanceTone::Unknown;return out;}
    }
    const double cellRadius=half*1.4142135623730951;uint64_t maybe=0;
    for(const auto* rule:active)if(rule->priority==best){const double di=syncSelectionDistance(point,rule->site);bool all=true,excluded=false;
        for(const auto* other:active)if(other->priority==best&&other!=rule){const double dj=syncSelectionDistance(point,other->site);
            double low=di-dj-2*cellRadius,high=di-dj+2*cellRadius;
            const double denominator=di+dj-2*cellRadius;
            if(denominator>1e-8){
                // Squared-distance difference is affine in XY. Divide its
                // exact interval by an enclosing positive distance sum.
                const double numerator=di*di-dj*dj,spread=2*cellRadius*hypot(double(rule->site.x)-other->site.x,double(rule->site.y)-other->site.y);
                const double upper=di+dj+2*cellRadius;
                low=std::min({(numerator-spread)/denominator,(numerator-spread)/upper,(numerator+spread)/denominator,(numerator+spread)/upper});
                high=std::max({(numerator-spread)/denominator,(numerator-spread)/upper,(numerator+spread)/denominator,(numerator+spread)/upper});
            }
            if(low>=rules.tolerance+1e-6){excluded=true;break;}if(high>=rules.tolerance-1e-6)all=false;
        }
        const uint64_t bit=uint64_t(1)<<rule->animation;if(!excluded){if(all)out.possible|=bit;else maybe|=bit;}
    }
    if(maybe&~out.possible){out.tone=SyncDistanceTone::Boundary;return out;}
    out.tone=out.possible&&(out.possible&(out.possible-1))?SyncDistanceTone::Random:SyncDistanceTone::Single;
    return out;
}
static std::array<float,4> syncDistanceColour(const SyncDistanceState& state,bool lines=false) {
    if(state.tone==SyncDistanceTone::Random)return {1,.12f,.48f,lines?.9f:.12f};
    if(state.tone==SyncDistanceTone::Unknown||state.tone==SyncDistanceTone::Boundary)return {.65f,.7f,.75f,lines?.6f:.035f};
    static constexpr float palette[][3]{{.2f,.85f,.95f},{.3f,1,.55f},{1,.85f,.25f},{.7f,.65f,1},{.95f,.5f,.2f},{.4f,.65f,1}};
    unsigned index=0;uint64_t mask=state.possible;while(mask&&!(mask&1)){++index;mask>>=1;}
    const auto* c=palette[index%6];return {c[0],c[1],c[2],lines?.75f:.055f};
}
static SyncDistanceMap syncBuildDistanceMap(const SyncTargetFrame& value) {
    SyncDistanceMap out;const auto rules=syncReadDistanceRules(value);out.detail=rules.detail;out.names=rules.names;out.authored=rules.authored;out.z=rules.z;
    if(!rules.known)return out;if(rules.rules.empty()){out.known=true;return out;}
    double x0=DBL_MAX,y0=DBL_MAX,x1=-DBL_MAX,y1=-DBL_MAX;
    for(const auto& rule:rules.rules)for(const auto& v:rule.volumes){double lowX=-DBL_MAX,lowY=-DBL_MAX,highX=DBL_MAX,highY=DBL_MAX;
        for(const auto& ball:v.balls){const double dz=ball.horizontal?0:double(rules.z)-ball.center.z,r2=double(ball.radius)*ball.radius-dz*dz;if(r2<=0){lowX=1;highX=0;break;}
            const double sectionRadius=sqrt(r2);lowX=std::max(lowX,ball.center.x-sectionRadius);highX=std::min(highX,ball.center.x+sectionRadius);lowY=std::max(lowY,ball.center.y-sectionRadius);highY=std::min(highY,ball.center.y+sectionRadius);}
        if(lowX<=highX&&lowY<=highY){x0=std::min(x0,lowX);x1=std::max(x1,highX);y0=std::min(y0,lowY);y1=std::max(y1,highY);}
    }
    if(!std::isfinite(x0)||x0>x1||y0>y1){out.known=true;return out;}
    const double width=std::max(x1-x0,y1-y0),cx=(x0+x1)*.5,cy=(y0+y1)*.5;
    if(!std::isfinite(width)||width<=0||width>400)return out;
    constexpr int resolution=256,base=32;const double step=width/resolution;
    x0=cx-width*.5;y0=cy-width*.5;out.precision=step;
    struct Leaf {int x,y,size;SyncDistanceState state;};std::vector<Leaf> leaves;size_t work=0;
    auto visit=[&](auto&& self,int x,int y,int size)->void {
        if(++work>80000)return;
        const Vec3 point{float(x0+(x+size*.5)*step),float(y0+(y+size*.5)*step),rules.z};
        const auto state=syncDistanceState(rules,point,size*step*.5);
        if(state.tone==SyncDistanceTone::Boundary&&size>1){const int h=size/2;for(int dy:{0,h})for(int dx:{0,h})self(self,x+dx,y+dy,h);return;}
        leaves.push_back({x,y,size,state});
    };
    for(int y=0;y<resolution;y+=resolution/base)for(int x=0;x<resolution;x+=resolution/base)visit(visit,x,y,resolution/base);
    if(work>80000){out.detail="Distance map: geometry budget";return out;}
    struct Edge {bool horizontal;int line,low,high,direction;uint64_t key;bool availability;SyncDistanceTone tone;};std::vector<Edge> edges;
    auto vertex=[&](double x,double y,float z,const std::array<float,4>& c){Vertex v{};v.position={float(x),float(y),z};memcpy(v.color,c.data(),16);return v;};
    const float floor=rules.z+.025f;
    auto line=[&](double ax,double ay,double bx,double by,const auto& colour){out.lines.push_back(vertex(ax,ay,floor+.003f,colour));out.lines.push_back(vertex(bx,by,floor+.003f,colour));};
    for(const auto& leaf:leaves){const auto& s=leaf.state;const double ax=x0+leaf.x*step,ay=y0+leaf.y*step,bx=ax+leaf.size*step,by=ay+leaf.size*step;
        const bool body=value.shotBounds[0]<bx&&value.shotBounds[3]>ax&&value.shotBounds[1]<by&&value.shotBounds[4]>ay;
        if(body)continue;
        if(s.available&&s.tone!=SyncDistanceTone::Boundary){const auto colour=syncDistanceColour(s);
            for(const auto& p:{std::make_pair(ax,ay),std::make_pair(bx,ay),std::make_pair(bx,by),std::make_pair(ax,ay),std::make_pair(bx,by),std::make_pair(ax,by)})out.fill.push_back(vertex(p.first,p.second,floor,colour));
            if(s.tone==SyncDistanceTone::Random){const std::array<float,4> hatch{1,.15f,.5f,.7f};const double spacing=std::max(.12,step*2);
                for(double offset=-leaf.size*step;offset<leaf.size*step;offset+=spacing){const double low=std::max(0.,-offset),high=std::min(leaf.size*step,leaf.size*step-offset);
                    if(high>low)line(ax+low,ay+low+offset,ax+high,ay+high+offset,hatch);}
            }
        }
        // Border classes are separate: availability changes are white;
        // selector/random-band boundaries use the magenta snapshot contour.
        const uint64_t selector=s.tone==SyncDistanceTone::Random?uint64_t(1)<<63:s.tone==SyncDistanceTone::Single?s.possible:0;
        for(bool availability:{true,false}){const uint64_t key=availability?s.available:selector;if(!key)continue;
            edges.push_back({true,leaf.y,leaf.x,leaf.x+leaf.size,1,key,availability,s.tone});edges.push_back({true,leaf.y+leaf.size,leaf.x,leaf.x+leaf.size,-1,key,availability,s.tone});
            edges.push_back({false,leaf.x,leaf.y,leaf.y+leaf.size,1,key,availability,s.tone});edges.push_back({false,leaf.x+leaf.size,leaf.y,leaf.y+leaf.size,-1,key,availability,s.tone});}
    }
    std::sort(edges.begin(),edges.end(),[](const auto& a,const auto& b){return std::tie(a.availability,a.horizontal,a.line,a.key)<std::tie(b.availability,b.horizontal,b.line,b.key);});
    for(size_t first=0;first<edges.size();){size_t end=first+1;const auto& e=edges[first];
        while(end<edges.size()&&edges[end].availability==e.availability&&edges[end].horizontal==e.horizontal&&edges[end].line==e.line&&edges[end].key==e.key)++end;
        std::vector<std::pair<int,int>> events;for(size_t i=first;i<end;++i){events.push_back({edges[i].low,edges[i].direction});events.push_back({edges[i].high,-edges[i].direction});}
        std::sort(events.begin(),events.end());int active=0,begin=0;
        for(size_t i=0;i<events.size();){const int at=events[i].first;int delta=0;do{delta+=events[i++].second;}while(i<events.size()&&events[i].first==at);
            const int previous=active;active+=delta;if(!previous&&active)begin=at;
            if(previous&&!active&&at>begin){const std::array<float,4> colour=e.availability?std::array<float,4>{.9f,.95f,1,.75f}:e.key==(uint64_t(1)<<63)?std::array<float,4>{1,.15f,.5f,.9f}:std::array<float,4>{.35f,.85f,1,.6f};
                if(e.horizontal)line(x0+begin*step,y0+e.line*step,x0+at*step,y0+e.line*step,colour);else line(x0+e.line*step,y0+begin*step,x0+e.line*step,y0+at*step,colour);
            }
        }first=end;
    }
    if(out.fill.size()>600000||out.lines.size()>300000){out.fill.clear();out.lines.clear();out.detail="Distance map: vertex budget";return out;}
    out.known=true;return out;
}
static uint64_t syncDistanceSignature(const SyncTargetFrame& value) {
    uint64_t hash=1469598103934665603ull;auto bytes=[&](const void* p,size_t n){const auto* b=static_cast<const unsigned char*>(p);for(size_t i=0;i<n;++i){hash^=b[i];hash*=1099511628211ull;}};
    auto text=[&](const std::string& s){bytes(s.data(),s.size());const char end=0;bytes(&end,1);};
    bytes(&value.action,sizeof(value.action));bytes(&value.entity.address,sizeof(value.entity.address));bytes(&value.entity.id,sizeof(value.entity.id));bytes(&value.entity.generation,sizeof(value.entity.generation));bytes(&value.level.map,sizeof(value.level.map));
    bytes(&value.facing,sizeof(value.facing));bytes(value.shotBounds,24);bytes(&value.ranges.playerOrigin.z,4);
    for(float f:{value.ranges.gloryKillTarget,value.ranges.chainsawTarget,value.ranges.lunge,value.ranges.eyeOffset.z})bytes(&f,4);
    bytes(&value.ranges.viewTargetOffset,sizeof(Vec3));bytes(&value.ranges.ignoreQuickLunge,sizeof(bool));bytes(&value.ranges.known,sizeof(bool));bytes(&value.rulesCoherent,sizeof(bool));text(value.regionState);
    const auto& aim=value.retainedTarget?value.retainedAim:value.aim;const auto& focus=value.retainedTarget?value.retainedFocus:value.jointFocus;
    bytes(&aim.known,sizeof(bool));bytes(&aim.quadrant,4);bytes(&focus.known,sizeof(bool));text(focus.group);bytes(&value.selectionKnown,sizeof(bool));bytes(&value.selectionTolerance,4);
    for(const auto& r:value.regions){text(r.name);bytes(&r.incoming,sizeof(Vec3));bytes(&r.start,sizeof(Vec3));for(float f:{r.halfYaw,r.minXY,r.maxXY,r.minZ,r.maxZ,r.priority})bytes(&f,4);
        bytes(&r.priorityKnown,sizeof(bool));bytes(&r.checkLunge,sizeof(bool));bytes(&r.flatLunge,sizeof(bool));bytes(&r.aim.known,sizeof(bool));bytes(&r.aim.quadrants,4);text(syncAimGroupKey(r.aim.damageGroups));}
    bytes(&value.hasChainsaw,sizeof(bool));for(const auto& name:value.chainsawAnimations)text(name);
    for(const auto& r:value.chainsawRules){text(r.name);bytes(&r.quick,sizeof(bool));bytes(&r.selectionKnown,sizeof(bool));bytes(&r.priority,4);bytes(&r.selectionPosition,sizeof(Vec3));bytes(&r.aim.known,sizeof(bool));bytes(&r.aim.quadrants,4);text(syncAimGroupKey(r.aim.damageGroups));bytes(&r.audit.matched,sizeof(bool));text(r.audit.declaration);}
    for(const auto& r:value.animationRules)if(r.action==1){text(r.name);bytes(&r.audit.matched,sizeof(bool));text(r.audit.declaration);bytes(&r.aim.known,sizeof(bool));bytes(&r.aim.quadrants,4);text(syncAimGroupKey(r.aim.damageGroups));}
    for(const auto& r:value.nativeChoices){text(r.name);bytes(&r.action,sizeof(r.action));bytes(&r.known,sizeof(bool));bytes(&r.priority,4);bytes(&r.position,sizeof(Vec3));}return hash?hash:1;
}
static std::shared_ptr<const SyncDistanceMap> syncDistanceMapCache;
static std::shared_ptr<const SyncDistanceMap> syncDistanceMapFor(const SyncTargetFrame& value) {
    const auto signature=syncDistanceSignature(value);
    const auto cached=std::atomic_load(&syncDistanceMapCache);if(cached&&cached->signature==signature)return cached;
    auto fresh=std::make_shared<SyncDistanceMap>(syncBuildDistanceMap(value));fresh->signature=signature;
    std::shared_ptr<const SyncDistanceMap> immutable=fresh;std::atomic_store(&syncDistanceMapCache,immutable);return immutable;
}
static std::string syncDistanceChoiceText(const SyncTargetFrame& value) {
    // Geometry is cached across XY movement, current status is never cached.
    const auto rules=syncReadDistanceRules(value);if(!rules.known)return rules.detail;
    const auto state=syncDistanceState(rules,value.ranges.playerOrigin);
    if(state.tone==SyncDistanceTone::Outside)return "Distance now: no available animation for this aimed area";
    if(state.tone==SyncDistanceTone::Unknown||state.tone==SyncDistanceTone::Boundary)return "Distance now: choice unresolved | no random-choice claim";
    std::string text=state.tone==SyncDistanceTone::Random?"Distance now: random choice | ":"Distance now: single choice | ";
    unsigned count=0;for(size_t i=0;i<rules.names.size()&&i<32;++i)if(state.possible&(uint64_t(1)<<i)){
        if(count++)text+=" / ";text+=syncZoneLabel(rules.names[i]);if(count==3)break;
    }return text+" (reference)";
}
static void appendSyncDistanceMap(const SyncDistanceMap& map,std::vector<Vertex>& output,bool lines) {
    if(map.known){const auto& vertices=lines?map.lines:map.fill;output.insert(output.end(),vertices.begin(),vertices.end());}
}
