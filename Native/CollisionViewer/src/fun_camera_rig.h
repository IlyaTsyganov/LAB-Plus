#pragma once

static bool funJointCameraMatch(const GameCamera& camera,const float* origin,const float* axis,const float* scale,const float* joint) {
    double squared=0;
    for(unsigned a=0;a<3;++a) {
        if(!std::isfinite(camera.position[a])||!std::isfinite(origin[a])) return false;
        double world=origin[a];
        for(unsigned b=0;b<3;++b) {
            float translation=joint[b*4+3],basis=axis[b*3+a];
            if(!std::isfinite(translation)||!std::isfinite(basis)||!std::isfinite(scale[b])||scale[b]<=0||scale[b]>100) return false;
            world+=double(translation)*scale[b]*basis;
        }
        double delta=world-camera.position[a];squared+=delta*delta;
    }
    return squared<=.15*.15;
}
// Read the animated body pose rather than widening the distance threshold
// around the dormant player entity. Cutscene movement can separate the two.
class FunCameraRig {
    InspectorField md6,skeleton,data,numJoints,animator,joints,current,parms,origin,axis,scale;
    InspectorField layoutSize,inverseBind,parents;
    bool ready=false;
public:
    void initialize(const MeathookHudApi& api) {
        md6=inspectorField(api,"idRenderModelSkinned","md6Model");skeleton=inspectorField(api,"idMD6Model","skeleton");
        data=inspectorField(api,"idMD6Skel","data");numJoints=inspectorField(api,"idMD6SkelData","numJoints");
        animator=inspectorField(api,"idRenderModelSkinned","jointAnimator");joints=inspectorField(api,"idJointAnimator","joints");
        current=inspectorField(api,"idJointAnimator","currentDeferred");parms=inspectorField(api,"idRenderModel","g");
        origin=inspectorField(api,"idRenderModelParms","origin");axis=inspectorField(api,"idRenderModelParms","axis");scale=inspectorField(api,"idRenderModelParms","scale");
        layoutSize=inspectorField(api,"idMD6SkelData","size");inverseBind=inspectorField(api,"idMD6SkelData","inverseBasePoseOffset");parents=inspectorField(api,"idMD6SkelData","parentTblOffset");
        ready=md6&&md6.type=="idMD6Model*"&&md6.size==8&&skeleton&&skeleton.type=="idMD6Skel*"&&skeleton.size==8&&
            data&&data.type=="idMD6SkelData*"&&data.size==8&&numJoints&&numJoints.size==2&&animator&&animator.type=="idJointAnimator*"&&animator.size==8&&
            joints&&joints.type=="idJointMat*[4]"&&joints.size==32&&current&&current.size==4&&parms&&parms.type=="idRenderModelParms"&&
            origin&&origin.type=="idVec3"&&origin.size==12&&axis&&axis.type=="idMat3"&&axis.size==36&&scale&&scale.type=="idVec3"&&scale.size==12;
        if(ready) note("Fun camera: animated Slayer pose reader available.");
    }
    bool matches(uintptr_t model,const GameCamera& camera) const {
        if(!ready) return false;
        WallMemory memory;uintptr_t mesh=0,skel=0,layout=0,pose=0,matrices=0;uint16_t count=0;int deferred=-1;
        float position[3]{},rotation[9]{},stretch[3]{};
        if(!memory.get(model+md6.offset,mesh)||!memory.get(mesh+skeleton.offset,skel)||!memory.get(skel+data.offset,layout)||
            !memory.get(layout+numJoints.offset,count)||!count||count>1024||!memory.get(model+animator.offset,pose)||
            !memory.get(pose+current.offset,deferred)||deferred<0||deferred>1||!memory.get(pose+joints.offset+size_t(deferred)*8,matrices)||!matrices||
            !memory.read(model+parms.offset+origin.offset,position,12)||!memory.read(model+parms.offset+axis.offset,rotation,36)||
            !memory.read(model+parms.offset+scale.offset,stretch,12)) return false;
        std::vector<std::array<float,12>> jointData(count);
        if(!memory.read(matrices,jointData.data(),jointData.size()*48)) return false;
        for(const auto& joint:jointData) if(funJointCameraMatch(camera,position,rotation,stretch,joint.data())) return true;
        return false;
    }
    bool headPosition(uintptr_t model,const std::string& path,float (&result)[3]) const {
        if(!ready||!layoutSize||layoutSize.size!=2||!inverseBind||inverseBind.size!=2||!parents||parents.size!=2) return false;
        WallMemory memory;uintptr_t mesh=0,skel=0,layout=0,pose=0,matrices=0;
        uint16_t count=0,bytes=0,bindOffset=0,parentOffset=0;int deferred=-1;
        if(!memory.get(model+md6.offset,mesh)||!memory.get(mesh+skeleton.offset,skel)||!memory.get(skel+data.offset,layout)||
            !memory.get(layout+numJoints.offset,count)||!memory.get(layout+layoutSize.offset,bytes)||
            !memory.get(layout+inverseBind.offset,bindOffset)||!memory.get(layout+parents.offset,parentOffset)) return false;
        // Captured reference rigs: head above neck (not the camera helper).
        // Exact model, joint count, parent and bind-space head bounds guard
        // these indices against another skin/skeleton being substituted.
        int index=-1,expectedParent=-1;
        if((path=="md6def/player/human/base/tp_body.md6"||path=="md6def/player/human/base/tp_body_praetor.md6")&&count==92) { index=91;expectedParent=90; }
        else if(path=="md6def/characters/humans/doomslayer/base/doomslayer_cine.md6"&&count==163) { index=88;expectedParent=87; }
        else if(path=="md6def/player/human/base/helmet.md6"&&count==2) { index=1;expectedParent=0; }
        if(index<0||bindOffset<64||parentOffset<64||size_t(bindOffset)+size_t(count)*48>bytes||size_t(parentOffset)+size_t(count)*2>bytes) return false;
        int16_t parent=-1;float bind[12]{};
        if(!memory.get(layout+parentOffset+size_t(index)*2,parent)||parent!=expectedParent||!memory.read(layout+bindOffset+size_t(index)*48,bind,48)) return false;
        float bindPosition[3]{};
        for(unsigned a=0;a<3;++a) for(unsigned b=0;b<3;++b) bindPosition[a]-=bind[b*4+a]*bind[b*4+3];
        for(float value:bindPosition) if(!std::isfinite(value)) return false;
        if(count>2&&(fabsf(bindPosition[0])>.2f||fabsf(bindPosition[1])>.2f||bindPosition[2]<1.55f||bindPosition[2]>1.95f)) return false;
        if(!memory.get(model+animator.offset,pose)||!memory.get(pose+current.offset,deferred)||deferred<0||deferred>1||
            !memory.get(pose+joints.offset+size_t(deferred)*8,matrices)||!matrices) return false;
        float joint[12]{},position[3]{},rotation[9]{},stretch[3]{};
        if(!memory.read(matrices+size_t(index)*48,joint,48)||!memory.read(model+parms.offset+origin.offset,position,12)||
            !memory.read(model+parms.offset+axis.offset,rotation,36)||!memory.read(model+parms.offset+scale.offset,stretch,12)) return false;
        for(unsigned a=0;a<3;++a) {
            double world=position[a];
            for(unsigned b=0;b<3;++b) {
                if(!std::isfinite(joint[b*4+3])||!std::isfinite(rotation[b*3+a])||!std::isfinite(stretch[b])||stretch[b]<=0||stretch[b]>100) return false;
                world+=double(joint[b*4+3])*stretch[b]*rotation[b*3+a];
            }
            if(!std::isfinite(world)) return false;
            result[a]=static_cast<float>(world);
        }
        return true;
    }
};
