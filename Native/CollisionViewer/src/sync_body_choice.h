#pragma once

// Ordinary player input uses the same priority/nearest selector for GK and
// QUICK_CHAINSAW. Geometry, aim scopes and sites remain per declaration.
struct SyncBodyChoiceRow {
    const SyncFacingRegion* region=nullptr;
    std::string name,declaration;SyncAimRequirement aim;
    Vec3 site{};float priority=0,distanceSquared=0;
    bool priorityKnown=false,siteKnown=false;size_t order=SIZE_MAX;
};
struct SyncBodyChoicePlan {
    bool applies=false;SyncAimDetail detail=SyncAimDetail::WholeBody;
    std::vector<SyncAimRequirement> coverage,sources;SyncAimColourRules colours;
};
static bool syncBodyNearest(const SyncTargetFrame& value) {
    return value.nearestChoiceKnown&&(value.action==0||value.action==1);
}
static size_t syncBodyChoiceOrder(const SyncTargetFrame& value,int action,const std::string& name,const std::string& declaration) {
    size_t order=SIZE_MAX;
    for(const auto& row:value.selectorOrder)if(row.action==action&&row.name==name&&
        (declaration.empty()||row.declaration==declaration))order=std::min(order,row.order);
    // Legacy captured fixtures predate declaration identities.
    if(order==SIZE_MAX&&value.selectorOrder.empty()&&action==0){const auto at=std::find(value.gloryKillOrder.begin(),value.gloryKillOrder.end(),name);
        if(at!=value.gloryKillOrder.end())order=size_t(at-value.gloryKillOrder.begin());}
    return order;
}
static std::vector<SyncBodyChoiceRow> syncBodyChoiceRows(const SyncTargetFrame& value) {
    std::vector<SyncBodyChoiceRow> rows;
    if(!value.rulesCoherent||!value.ranges.known)return rows;
    const auto limits=syncTargetReferenceLimits(value.facing,value.ranges);
    auto add=[&](SyncBodyChoiceRow row){
        const SyncNativeChoice* native=nullptr;
        for(const auto& n:value.aimSelectorSites)if(n.action==value.action&&n.name==row.name&&n.known&&
            (row.declaration.empty()||row.declaration==n.declaration)){
            if(native&&(native->priority!=n.priority||memcmp(&native->position,&n.position,sizeof(Vec3)))){row.siteKnown=false;rows.push_back(std::move(row));return;}
            if(!native||n.order<native->order)native=&n;}
        if(native){row.site=native->position;row.priority=native->priority;row.siteKnown=row.priorityKnown=true;row.order=native->order;}
        if(row.order==SIZE_MAX)row.order=syncBodyChoiceOrder(value,value.action,row.name,row.declaration);
        row.distanceSquared=syncNearestDistanceSquared(value.ranges.playerOrigin,row.site);rows.push_back(std::move(row));
    };
    if(value.action==0){
        if(value.regionState!="ready")return rows;
        for(const auto& region:value.regions){SyncRangeVolume volume;
            if(!syncRangeBuild(&region,value.facing,limits,false,value.incomingYaw,volume,false)||!syncRangeContains(volume,value.ranges.playerOrigin))continue;
            SyncBodyChoiceRow row;row.region=&region;row.name=region.name;row.declaration=region.audit.declaration;row.aim=region.aim;
            row.site=motionAdd(value.facing.origin,syncRangeTransform(value.facing,region.start));row.siteKnown=true;
            row.priorityKnown=region.priorityKnown;row.priority=region.priority;add(std::move(row));
        }
    }else if(value.hasChainsaw){
        SyncRangeVolume volume;
        if(!syncRangeBuild(nullptr,value.facing,limits,true,value.incomingYaw,volume,false)||!syncRangeContains(volume,value.ranges.playerOrigin))return rows;
        for(const auto& rule:value.chainsawRules){
            SyncBodyChoiceRow row;row.name=rule.name;row.declaration=rule.audit.declaration;row.aim=rule.aim;
            row.priorityKnown=row.siteKnown=rule.quick&&rule.selectionKnown;row.priority=rule.priority;row.site=rule.selectionPosition;
            if(!rule.quick)row.aim.known=false;add(std::move(row));
        }
    }
    return rows;
}
static const SyncBodyChoiceRow* syncBodyChoiceWinner(const std::vector<SyncBodyChoiceRow>& rows,const std::string& group,uint32_t quadrant,bool commonScope=false) {
    const SyncBodyChoiceRow* best=nullptr;bool uncertain=false,unknown=false,sameScope=true;
    const SyncBodyChoiceRow* first=nullptr;
    for(const auto& row:rows){const auto& aim=row.aim;
        if(!aim.known||!syncAimMaskKnown(aim.quadrants))return nullptr;
        if(!syncAimMatches(aim,quadrant)||(!aim.damageGroups.empty()&&std::find(aim.damageGroups.begin(),aim.damageGroups.end(),group)==aim.damageGroups.end()))continue;
        if(!first)first=&row;else sameScope&=syncSameAimRule(first->aim,aim);
        if(!row.priorityKnown||!row.siteKnown||!std::isfinite(row.priority)||!syncRangeFinite(row.site)||!std::isfinite(row.distanceSquared)){unknown=true;continue;}
        if(!best||row.priority>best->priority||(row.priority==best->priority&&row.distanceSquared<best->distanceSquared)){
            best=&row;uncertain=false;continue;}
        if(row.priority==best->priority&&row.distanceSquared==best->distanceSquared&&row.name!=best->name){
            if(row.order==SIZE_MAX||best->order==SIZE_MAX)uncertain=true;
            else if(row.order<best->order)best=&row;
        }
    }
    // Missing sites or an exact tie cannot change colours when ALL matching
    // competitors have the identical original scope. This is not a clip claim.
    if(unknown||uncertain)return commonScope&&sameScope?first:nullptr;
    return best;
}
static SyncBodyChoicePlan syncBodyChoicePlan(const SyncTargetFrame& value) {
    SyncBodyChoicePlan plan;if(!syncBodyNearest(value))return plan;
    plan.applies=true;const auto rows=syncBodyChoiceRows(value);
    std::vector<std::string> groups;
    for(const auto& sphere:value.spheres){if(!sphere.focusKnown)return plan;
        if(std::find(groups.begin(),groups.end(),sphere.focusGroup)==groups.end())groups.push_back(sphere.focusGroup);}
    auto add=[](auto& names,const auto& name){if(std::find(names.begin(),names.end(),name)==names.end())names.push_back(name);};
    bool named=false,halves=false;
    for(unsigned q=0;q<4;++q){const uint32_t quadrant=1u<<q;SyncAimRequirement coverage;coverage.quadrants=quadrant;
        for(const auto& group:groups){const auto* winner=syncBodyChoiceWinner(rows,group,quadrant,value.selectedAnimation.empty());if(!winner)continue;
            if(!value.selectedAnimation.empty()&&winner->name!=value.selectedAnimation)continue;
            add(coverage.damageGroups,group);
            const auto& aim=winner->aim;const bool specific=syncBodyAimKind(&aim,value.spheres)==SyncBodyAimKind::Specific;
            const bool lower=!syncAimUnrestricted(aim.quadrants)&&!(aim.quadrants&3);
            if(specific)add(plan.colours.namedGroups[q],group);
            if(lower)add(plan.colours.lowerGroups[q],group);
            named|=specific;halves|=!syncAimUnrestricted(aim.quadrants);
            if(std::none_of(plan.sources.begin(),plan.sources.end(),[&](const auto& source){return syncSameAimRule(source,aim);}))plan.sources.push_back(aim);
        }
        if(!coverage.damageGroups.empty())plan.coverage.push_back(std::move(coverage));
    }
    plan.detail=named?SyncAimDetail::Anatomy:halves?SyncAimDetail::Halves:SyncAimDetail::WholeBody;return plan;
}
