#pragma once
#include <tuple>
#include "sync_enemy_catalog.h"

static std::string syncAimGroupKey(std::vector<std::string> groups) {
    std::sort(groups.begin(),groups.end());groups.erase(std::unique(groups.begin(),groups.end()),groups.end());
    std::string key;for(const auto& g:groups){if(!key.empty())key+='|';key+=g;}return key;
}
static SyncRuleAudit syncAuditEnemyRule(const std::string& declaration,const std::string& animation,int action,const SyncAimRequirement& aim) {
    SyncRuleAudit audit;audit.checked=true;audit.declaration=declaration;
    for(const auto& rule:syncAuditedEnemyRules)if(action==rule.action&&declaration==rule.declaration&&animation==rule.animation){
        audit.family=rule.family;audit.enemy=rule.label;
        audit.matched=aim.known&&syncAimMaskKnown(aim.quadrants)&&
            (aim.quadrants==rule.quadrants||(syncAimUnrestricted(aim.quadrants)&&syncAimUnrestricted(rule.quadrants)))&&syncAimGroupKey(aim.damageGroups)==rule.groups;break;}
    return audit;
}
static bool syncSameAimRule(const SyncAimRequirement& a,const SyncAimRequirement& b) {
    return a.known==b.known&&a.quadrants==b.quadrants&&syncAimGroupKey(a.damageGroups)==syncAimGroupKey(b.damageGroups);
}
static bool syncSameFacingRule(const SyncFacingRegion& a,const SyncFacingRegion& b) {
    return syncSameAimRule(a.aim,b.aim)&&!memcmp(&a.incoming,&b.incoming,sizeof(Vec3))&&!memcmp(&a.start,&b.start,sizeof(Vec3))&&
        a.halfYaw==b.halfYaw&&a.minXY==b.minXY&&a.maxXY==b.maxXY&&a.minZ==b.minZ&&a.maxZ==b.maxZ&&a.checkLunge==b.checkLunge&&a.flatLunge==b.flatLunge&&
        a.priorityKnown==b.priorityKnown&&(!a.priorityKnown||a.priority==b.priority);
}
static bool syncNormalizeFacingRegions(std::vector<SyncFacingRegion>& regions) {
    std::stable_sort(regions.begin(),regions.end(),[](const auto& a,const auto& b){return a.name<b.name;});
    bool compatible=true;
    for(size_t i=1;i<regions.size();++i)if(regions[i].name==regions[i-1].name&&!syncSameAimRule(regions[i].aim,regions[i-1].aim))compatible=false;
    regions.erase(std::unique(regions.begin(),regions.end(),[](const auto& a,const auto& b){return a.name==b.name&&syncSameFacingRule(a,b);}),regions.end());
    return compatible;
}
static bool syncAimScopesOverlap(const SyncAimRequirement& selected,const SyncAimRequirement& other) {
    if(!selected.known||!other.known||!syncAimMaskKnown(selected.quadrants)||!syncAimMaskKnown(other.quadrants))return true;
    const auto a=syncAimUnrestricted(selected.quadrants)?15u:selected.quadrants,b=syncAimUnrestricted(other.quadrants)?15u:other.quadrants;
    if(!(a&b))return false;
    if(selected.damageGroups.empty()||other.damageGroups.empty())return true;
    for(const auto& group:selected.damageGroups)if(std::find(other.damageGroups.begin(),other.damageGroups.end(),group)!=other.damageGroups.end())return true;
    return false;
}
static bool syncGkCanCompete(const SyncFacingRegion& selected,const SyncFacingRegion& other) {
    // Only audited ordinary priorities may remove a lower-priority fallback.
    // Unknown/dynamic priority always remains a potential competitor.
    return !(selected.priorityKnown&&other.priorityKnown&&other.priority<selected.priority);
}
static void syncMergeAnimationRules(std::vector<SyncAnimationRule>& rules) {
    std::sort(rules.begin(),rules.end(),[](const auto& a,const auto& b){return std::tie(a.action,a.name)<std::tie(b.action,b.name);});
    std::vector<SyncAnimationRule> merged;
    for(auto& rule:rules){if(merged.empty()||merged.back().action!=rule.action||merged.back().name!=rule.name){merged.push_back(std::move(rule));continue;}
        auto& previous=merged.back();const bool compatible=syncSameAimRule(previous.aim,rule.aim)&&previous.audit.matched==rule.audit.matched;
        if(!compatible){previous.aim.known=false;previous.audit.matched=false;}
        else if(previous.audit.family!=rule.audit.family){previous.audit.family.clear();previous.audit.enemy="Target";}}
    rules=std::move(merged);
}
static std::string syncBodyGroupLabel(const std::vector<std::string>& groups) {
    bool head=false,leftArm=false,rightArm=false,leftLeg=false,rightLeg=false,leftFoot=false,rightFoot=false,torso=false,other=false;
    for(const auto& group:groups){if(group=="head")head=true;else if(group.find("left")!=std::string::npos&&(group.find("arm")!=std::string::npos))leftArm=true;
        else if(group.find("right")!=std::string::npos&&group.find("arm")!=std::string::npos)rightArm=true;
        else if(group.find("left")!=std::string::npos&&group.find("leg")!=std::string::npos)leftLeg=true;
        else if(group.find("right")!=std::string::npos&&group.find("leg")!=std::string::npos)rightLeg=true;
        else if(group.find("left")!=std::string::npos&&group.find("foot")!=std::string::npos)leftFoot=true;
        else if(group.find("right")!=std::string::npos&&group.find("foot")!=std::string::npos)rightFoot=true;
        else if(group=="chest"||group=="gut"||group=="stomach"||group=="crotch"||group=="neck"||group.find("spine")!=std::string::npos)torso=true;else other=true;}
    std::string text;auto add=[&](const char* value){if(!text.empty())text+=" / ";text+=value;};
    if(head)add("head");if(leftArm)add("left arm");if(rightArm)add("right arm");if(torso)add("torso");
    if(leftLeg)add("left leg");if(rightLeg)add("right leg");if(leftFoot)add("left foot");if(rightFoot)add("right foot");if(other)add("mixed body area");return text;
}
struct SyncAnimationDisplayHint {std::string animation,enemy,aim;};
