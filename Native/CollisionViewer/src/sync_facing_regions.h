#pragma once

// A diagram of the native facing test, not a collision shape or a promise
// that an interaction can execute. Its box is fitted to the aimed actor.
static constexpr float syncFacingPi=3.14159265358979323846f;
struct SyncAimRequirement {
    uint32_t quadrants=0;
    bool known=true;
    std::vector<std::string> damageGroups;
};
struct SyncRuleAudit {bool checked=false,matched=false;std::string declaration,family,enemy;};
struct SyncAnimationRule {std::string name;int action=0;SyncAimRequirement aim;SyncRuleAudit audit;};
struct SyncChainsawRule {
    std::string name;SyncAimRequirement aim;bool quick=true;
    // Runtime selector data, never retained in the authoring cache.
    bool selectionKnown=false;float priority=0;Vec3 selectionPosition{};
    SyncRuleAudit audit;
};
struct SyncAimFrame {bool known=false;float bounds[6]{};uint32_t quadrant=0;};
struct SyncJointFocus {bool known=false;std::string group;};
// Material labels describe shot shapes. Focus labels come from the native
// MD6 type-0 first-match joint lookup; the two must not be interchanged.
struct SyncHitSphere {
    Vec3 center{};float radius=0;std::string group;
    short jointIndex=-1;bool focusKnown=false;std::string focusGroup;
};
struct SyncNativeChoice {std::string name;int action=0;float priority=0;Vec3 position{};bool known=false;};
struct SyncFacingRegion {
    std::string name;
    Vec3 incoming{}; // Enemy-local direction opposite the authored player forward.
    Vec3 start{};
    float halfYaw=0,minXY=0,maxXY=0,minZ=0,maxZ=0;
    bool checkLunge=false,flatLunge=false;
    SyncAimRequirement aim;
    SyncRuleAudit audit;
    bool priorityKnown=false;float priority=0;
};
struct SyncFacingPose { float axis[9]{};Vec3 origin{}; };
static float syncFacingYaw(const SyncFacingPose& pose,Vec3 local) {
    return atan2f(pose.axis[1]*local.x+pose.axis[4]*local.y+pose.axis[7]*local.z,
                  pose.axis[0]*local.x+pose.axis[3]*local.y+pose.axis[6]*local.z);
}
static bool syncFacingContains(float center,float halfYaw,float yaw) {
    return fabsf(remainderf(yaw-center,2*syncFacingPi))<=halfYaw;
}
static bool syncFacingOrthonormal(const float* axis) {
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j) {
        double d=0;for(unsigned k=0;k<3;++k)d+=double(axis[i*3+k])*axis[j*3+k];
        if(!std::isfinite(d)||fabs(d-(i==j?1:0))>.004)return false;
    }
    return true;
}
static bool syncFacingIncoming(const float* instigator,const float* target,Vec3& incoming) {
    // MD6 joint matrices are row-major 3x4; idMat3 stores basis vectors.
    float a[9],b[9];for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){a[i*3+j]=instigator[i*4+j];b[i*3+j]=target[i*4+j];}
    if(!syncFacingOrthonormal(a)||!syncFacingOrthonormal(b))return false;
    const Vec3 forward{a[0],a[3],a[6]};
    incoming={-(b[0]*forward.x+b[3]*forward.y+b[6]*forward.z),
              -(b[1]*forward.x+b[4]*forward.y+b[7]*forward.z),
              -(b[2]*forward.x+b[5]*forward.y+b[8]*forward.z)};
    return std::isfinite(incoming.x)&&std::isfinite(incoming.y)&&std::isfinite(incoming.z)&&
        incoming.x*incoming.x+incoming.y*incoming.y>.99f;
}
