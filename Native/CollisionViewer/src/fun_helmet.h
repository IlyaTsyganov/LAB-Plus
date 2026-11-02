#pragma once
#include "fun_spatial_audio.h"
#include "fun_visor.h"
#include "fun_camera_rig.h"
#include "fun_camera.h"
#include "fun_shot.h"

// Only explicitly named helmet/visor models qualify. Never tint a complete
// Slayer body or select an object by its screen rectangle/colour.
static bool funHelmetModel(std::string name) {
    std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const bool part=name.find("helmet")!=std::string::npos||name.find("visor")!=std::string::npos;
    const bool slayer=name.find("slayer")!=std::string::npos||name.find("doomguy")!=std::string::npos||name.find("praetor")!=std::string::npos||name.find("player")!=std::string::npos;
    return part&&slayer;
}
static bool funSlayerBody(const std::string& name) {
    return name.find("doomslayer")!=std::string::npos||name.rfind("md6def/player/human/",0)==0;
}
class FunHelmet {
    FunVisor visor;
    FunCameraRig cameraRig;
    struct Target { uintptr_t slot=0,entity=0,model=0,infoColor=0,modelColor=0; uint32_t id=0,generation=0; float savedInfo[4]{},savedModel[4]{},lastInfo[4]{},lastModel[4]{}; bool changed=false,fullCinematic=false; std::string path,name; };
    std::vector<Target> targets;
    std::vector<Target> emitters;
    std::vector<Target> animatedCameras;
    struct SurfaceCandidate { uintptr_t surface=0,material=0;Target owner; };
    std::vector<SurfaceCandidate> surfaceCandidates;
    std::unordered_map<std::string,bool> bloated;
    std::unordered_set<std::string> reportedModels;
    std::unordered_set<std::string> reportedHelmets;
    uintptr_t map=0;
    ULONGLONG scanAt=0;
    bool missingReported=false;
    InspectorField renderInfo,modelPointer,infoColor,modelParms,modelColor,modelName,atomicText;
    InspectorField surfaces,surfaceMaterial,resourceName;
    InspectorField modelOrigin;
    InspectorField committedFrame;
    InspectorField entityName,cameraActive,cameraPosition,cameraClipInfo,cameraCut,clipList,clipAnimation;
    uint32_t clipSize=0;
    uint32_t surfaceSize=0;
    bool ready=false;
    static bool writable(uintptr_t at,size_t size) {
        MEMORY_BASIC_INFORMATION page{};
        return VirtualQuery(reinterpret_cast<void*>(at),&page,sizeof(page))&&page.State==MEM_COMMIT&&
            !(page.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&(page.Protect&(PAGE_READWRITE|PAGE_WRITECOPY))&&
            at>=reinterpret_cast<uintptr_t>(page.BaseAddress)&&size<=page.RegionSize-(at-reinterpret_cast<uintptr_t>(page.BaseAddress));
    }
    static bool write(uintptr_t at,const float* value) {
        if(!writable(at,16)) return false;
        SIZE_T done=0; return WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),value,16,&done)&&done==16;
    }
    static bool color(uintptr_t at,float* into) {
        if(!WallMemory{}.read(at,into,16)) return false;
        for(unsigned i=0;i<4;++i) if(!std::isfinite(into[i])||into[i]<0||into[i]>100) return false;
        return true;
    }
    bool valid(const Target& target) const {
        struct Slot { uint32_t id,generation; uintptr_t entity,previous,next; } slot{};
        uintptr_t model=0;
        return WallMemory{}.get(target.slot,slot)&&slot.id==target.id&&slot.generation==target.generation&&slot.entity==target.entity&&
            (!target.model||(WallMemory{}.get(target.entity+renderInfo.offset+modelPointer.offset,model)&&model==target.model));
    }
    std::string entityText(uintptr_t entity) const {
        uintptr_t text=0;
        return entityName&&entityName.type=="idStr"&&entityName.size==48&&WallMemory{}.get(entity+entityName.offset+8,text)?readEntityText(WallMemory{},text,256):std::string{};
    }
    bool derives(const MeathookHudApi& api,const std::string& type) {
        auto cached=bloated.find(type); if(cached!=bloated.end()) return cached->second;
        std::string current=type; bool result=false;
        for(unsigned i=0;i<32&&!current.empty();++i) {
            if(current=="idBloatedEntity") { result=true; break; }
            uintptr_t info=reinterpret_cast<uintptr_t>(api.findClass(current.c_str())),parent=0;
            if(!WallMemory{}.get(info+8,parent)) break;
            current=readEntityText(WallMemory{},parent,128);
        }
        bloated[type]=result; return result;
    }
    void reportMaterials(uintptr_t model,const std::string& name) {
        if(name.find("slayer")==std::string::npos&&name.find("doomguy")==std::string::npos&&name.find("praetor")==std::string::npos&&name.find("player")==std::string::npos) return;
        if(reportedModels.size()>=32||!reportedModels.insert(name).second) return;
        note(("Fun visible Slayer model: "+name).c_str());
        if(!surfaces||surfaces.size!=24||surfaces.type.find("idRenderModelSurface")==std::string::npos||!surfaceMaterial||surfaceMaterial.type!="idMaterial2*"||surfaceMaterial.size!=8||
           !resourceName||resourceName.type!="idAtomicString"||!surfaceSize||surfaceSize>4096||surfaceMaterial.offset+8>surfaceSize) return;
        uintptr_t list=0; uint32_t count=0;
        if(!WallMemory{}.get(model+surfaces.offset,list)||!WallMemory{}.get(model+surfaces.offset+8,count)||count>256) return;
        for(uint32_t i=0;i<count;++i) {
            uintptr_t material=0,text=0;
            if(!WallMemory{}.get(list+size_t(i)*surfaceSize+surfaceMaterial.offset,material)||!WallMemory{}.get(material+resourceName.offset+atomicText.offset,text)) continue;
            auto path=readEntityText(WallMemory{},text,512); if(!path.empty()) note(("Fun Slayer surface material: "+path).c_str());
        }
    }
public:
    void initialize(const MeathookHudApi& api,uintptr_t base=0,const GameProfile* profile=nullptr) {
        cameraRig.initialize(api);
        if(base&&profile) visor.initialize(api,base,*profile);
        renderInfo=inspectorField(api,"idBloatedEntity","renderModelInfo");
        modelPointer=inspectorField(api,"idRenderModelInfo","modelPtr");
        infoColor=inspectorField(api,"idRenderModelInfo","color");
        modelParms=inspectorField(api,"idRenderModel","g"); modelColor=inspectorField(api,"idRenderModelParms","color");
        modelOrigin=inspectorField(api,"idRenderModelParms","origin");
        committedFrame=inspectorField(api,"idRenderModel","surfacesCommittedFrameCount");
        entityName=inspectorField(api,"idEntity","name");cameraActive=inspectorField(api,"idCamera","isActive");
        cameraPosition=inspectorField(api,"idAnimCamera","camPos");cameraClipInfo=inspectorField(api,"idAnimCamera","midnightAnimInfo");
        cameraCut=inspectorField(api,"idAnimCamera","lastCutValue");
        clipList=inspectorField(api,"idMidnightAnimMdntInfo","clipList");clipAnimation=inspectorField(api,"idMidnightAnimClipInfo","anim");
        if(auto type=reinterpret_cast<uintptr_t>(api.findClass("idMidnightAnimClipInfo"))) WallMemory{}.get(type+16,clipSize);
        modelName=inspectorField(api,"idRenderModel","name"); atomicText=inspectorField(api,"idAtomicString","str");
        surfaces=inspectorField(api,"idRenderModel","surfaces"); surfaceMaterial=inspectorField(api,"idRenderModelSurface","material"); resourceName=inspectorField(api,"idResource","name");
        const auto surfaceType=reinterpret_cast<uintptr_t>(api.findClass("idRenderModelSurface")); if(surfaceType) WallMemory{}.get(surfaceType+16,surfaceSize);
        ready=renderInfo&&renderInfo.type=="idRenderModelInfo"&&modelPointer&&modelPointer.type=="idRenderModel*"&&modelPointer.size==8&&
            infoColor&&infoColor.type=="idColor"&&infoColor.size==16&&infoColor.offset+16<=renderInfo.size&&
            modelParms&&modelParms.type=="idRenderModelParms"&&modelColor&&modelColor.type=="idVec4"&&modelColor.size==16&&modelColor.offset+16<=modelParms.size&&
            modelName&&modelName.type=="idAtomicString"&&atomicText&&atomicText.type=="char*"&&atomicText.size==8&&atomicText.offset+8<=modelName.size;
    }
    void clearColors(uintptr_t currentMap) {
        visor.restore();
        if(currentMap==map) for(auto& target:targets) if(target.changed&&valid(target)) {
            float current[4]{};
            if(color(target.infoColor,current)&&!memcmp(current,target.lastInfo,16)) write(target.infoColor,target.savedInfo);
            if(color(target.modelColor,current)&&!memcmp(current,target.lastModel,16)) write(target.modelColor,target.savedModel);
        }
        targets.clear();
    }
    void restore(uintptr_t currentMap) {
        clearColors(currentMap);emitters.clear();animatedCameras.clear();surfaceCandidates.clear();map=currentMap;scanAt=0;
    }
    void update(const MeathookHudApi& api,uintptr_t base,const GameProfile& profile,uintptr_t currentMap,const std::array<float,4>& hue,bool applyColor=true) {
        if(!ready) return;
        if(map!=currentMap) restore(currentMap);
        const auto now=GetTickCount64();
        if(now>=scanAt) {
            scanAt=now+500; uintptr_t list=0; uint32_t count=0;
            emitters.clear();animatedCameras.clear();surfaceCandidates.clear();
            if(WallMemory{}.get(map+profile.mapEntitiesOffset,list)&&WallMemory{}.get(map+profile.mapEntitiesOffset+8,count)&&count<=32768) {
                for(uint32_t i=0;i<count&&targets.size()<16;++i) {
                    struct Slot { uint32_t id,generation; uintptr_t entity,previous,next; } slot{};
                    const auto address=list+size_t(i)*32;
                    if(!WallMemory{}.get(address,slot)||!slot.entity) continue;
                    uintptr_t vtable=0,model=0,name=0;
                    if(!WallMemory{}.get(slot.entity,vtable)) continue;
                    auto entityClass=readEntityClass(WallMemory{},vtable,base,profile);
                    Target target; target.slot=address; target.entity=slot.entity; target.id=slot.id; target.generation=slot.generation;
                    if(entityClass=="idAnimCamera") { target.name=entityText(slot.entity);if(animatedCameras.size()<64) animatedCameras.push_back(target);continue; }
                    if(!derives(api,entityClass)||
                       !WallMemory{}.get(slot.entity+renderInfo.offset+modelPointer.offset,model)||!model||
                       !WallMemory{}.get(model+modelName.offset+atomicText.offset,name)) continue;
                    auto text=readEntityText(WallMemory{},name,512); reportMaterials(model,text);
                    target.model=model;target.path=text;
                    target.fullCinematic=text.rfind("md6def/characters/",0)==0&&text.find("doomslayer")!=std::string::npos;
                    if(visor.available()&&funSlayerBody(text)&&surfaces&&surfaces.size==24&&surfaceMaterial&&surfaceMaterial.type=="idMaterial2*"&&
                        surfaceMaterial.size==8&&surfaceSize>=8&&surfaceSize<=4096&&surfaceMaterial.offset+8<=surfaceSize&&resourceName&&resourceName.type=="idAtomicString") {
                        uintptr_t list=0;uint32_t count=0;
                        if(WallMemory{}.get(model+surfaces.offset,list)&&WallMemory{}.get(model+surfaces.offset+8,count)&&count<=256) for(uint32_t k=0;k<count;++k) {
                            auto surface=list+size_t(k)*surfaceSize;uintptr_t material=0;
                            uintptr_t materialName=0;
                            if(surfaceCandidates.size()<128&&WallMemory{}.get(surface+surfaceMaterial.offset,material)&&
                                WallMemory{}.get(material+resourceName.offset+atomicText.offset,materialName)&&funVisorMaterial(readEntityText(WallMemory{},materialName,512)))
                                surfaceCandidates.push_back({surface,material,target});
                        }
                    }
                    const bool playerModel=text.rfind("md6def/player/human/",0)==0&&(text.find("helmet")!=std::string::npos||text.find("tp_body")!=std::string::npos);
                    const bool cinematicSlayer=text.rfind("md6def/characters/",0)==0&&text.find("doomslayer")!=std::string::npos;
                    if((playerModel||cinematicSlayer)&&emitters.size()<64) {target.name=entityText(slot.entity);emitters.push_back(target);}
                    if(!funHelmetModel(text)||std::any_of(targets.begin(),targets.end(),[&](const auto& t) { return t.slot==address&&t.generation==slot.generation&&t.entity==slot.entity; })) continue;
                    target.infoColor=slot.entity+renderInfo.offset+infoColor.offset; target.modelColor=model+modelParms.offset+modelColor.offset;
                    if(!color(target.infoColor,target.savedInfo)||!color(target.modelColor,target.savedModel)) continue;
                    targets.push_back(target); if(reportedHelmets.insert(text).second) note(("Fun helmet candidate (live validation required): "+text).c_str());
                }
            }
            if(targets.empty()&&surfaceCandidates.empty()&&!visor.selected()&&!missingReported) { missingReported=true; note("Fun: no supported Slayer visor or separate helmet found in the current scene."); }
        }
        if(!applyColor) return;
        for(const auto& candidate:surfaceCandidates) if(valid(candidate.owner)) {
            const auto& t=candidate.owner;visor.considerSurface(candidate.material,candidate.surface,t.model,t.slot,t.entity,t.id,t.generation);
        }
        visor.update(hue);
        for(auto& target:targets) {
            if(!valid(target)) continue;
            float currentInfo[4]{},currentModel[4]{};
            if(!color(target.infoColor,currentInfo)||!color(target.modelColor,currentModel)) continue;
            if(target.changed) {
                if(memcmp(currentInfo,target.lastInfo,16)) memcpy(target.savedInfo,currentInfo,16);
                if(memcmp(currentModel,target.lastModel,16)) memcpy(target.savedModel,currentModel,16);
            }
            for(unsigned k=0;k<4;++k) {
                const float multiplier=k==3?1.f:(.15f+.85f*hue[k]);
                target.lastInfo[k]=target.savedInfo[k]*multiplier; target.lastModel[k]=target.savedModel[k]*multiplier;
            }
            target.changed=write(target.infoColor,target.lastInfo)||target.changed;
            target.changed=write(target.modelColor,target.lastModel)||target.changed;
        }
    }
    FunShotView directedCamera(const GameCamera& camera) const {
        FunShotView result;
        if(!cameraActive||cameraActive.type!="bool"||cameraActive.size!=1||!cameraPosition||cameraPosition.size!=12||
            !cameraClipInfo||cameraClipInfo.size!=8||!clipList||clipList.size!=24||!clipAnimation||clipAnimation.type!="idAtomicString"||
            clipAnimation.size!=8||clipSize<24||clipSize>4096||clipAnimation.offset+8>clipSize) return result;
        unsigned activeCount=0;FunShotView single;double nearest=std::numeric_limits<double>::infinity();
        for(const auto& target:animatedCameras) {
            if(!valid(target)) continue;
            unsigned char active=0;float position[3]{};uintptr_t info=0,list=0;uint32_t count=0;
            if(!WallMemory{}.get(target.entity+cameraActive.offset,active)||active!=1||!WallMemory{}.read(target.entity+cameraPosition.offset,position,12)) continue;
            double squared=0;for(unsigned a=0;a<3;++a) { const double d=double(position[a])-camera.position[a];squared+=d*d; }
            if(!std::isfinite(squared)) continue;
            FunShotView candidate;
            if(target.name.size()>7&&target.name.compare(target.name.size()-7,7,"_camera")==0) candidate.actorPrefix=target.name.substr(0,target.name.size()-7)+"_";
            if(WallMemory{}.get(target.entity+cameraClipInfo.offset,info)&&info&&WallMemory{}.get(info+clipList.offset,list)&&
                WallMemory{}.get(info+clipList.offset+8,count)&&count==1) {
                uintptr_t text=0;if(WallMemory{}.get(list+clipAnimation.offset,text)) {
                    const auto animation=readEntityText(WallMemory{},text,512);int cut=-1;
                    if(cameraCut&&cameraCut.type=="int"&&cameraCut.size==4) WallMemory{}.get(target.entity+cameraCut.offset,cut);
                    candidate.view=funDirectedShot(animation,cut);
                    // Generic/func camera names need not share the actor's
                    // prefix. Prefer the authored clip's scene directory when
                    // a validated Slayer entity belongs to that scene.
                    const auto end=animation.rfind('/');
                    if(end!=std::string::npos&&end>0) {
                        const auto begin=animation.rfind('/',end-1);
                        if(begin!=std::string::npos) {
                            auto prefix=animation.substr(begin+1,end-begin-1)+"_";
                            if(std::any_of(emitters.begin(),emitters.end(),[&](const auto& actor) {return actor.name.rfind(prefix,0)==0&&valid(actor);})) candidate.actorPrefix=prefix;
                        }
                    }
                }
            }
            ++activeCount;single=candidate;
            if(squared<nearest) {nearest=squared;result=candidate;}
        }
        // Engine animation and renderer origins may belong to adjacent
        // frames during a cut. A unique active camera needs no distance guess.
        return activeCount==1?single:nearest<=.15*.15?result:FunShotView{};
    }
    FunSpatialMix spatial(const GameCamera& camera,const std::string& actorPrefix={}) const {
        struct Source { float position[3];uint32_t frame;bool full; };
        std::vector<Source> sources;uint32_t newest=~0u;
        for(const auto& target:emitters) {
            if(!valid(target)||(!actorPrefix.empty()&&target.name.rfind(actorPrefix,0)!=0)) continue;
            Source source{};source.full=target.fullCinematic;
            if(!committedFrame||committedFrame.size!=4||!WallMemory{}.get(target.model+committedFrame.offset,source.frame)||source.frame==~0u||
                !cameraRig.headPosition(target.model,target.path,source.position)) continue;
            if(newest==~0u||uint32_t(source.frame-newest)<0x80000000u) newest=source.frame;
            sources.push_back(source);
        }
        float best=-std::numeric_limits<float>::infinity();FunSpatialMix selected;selected.gain=0;
        for(const auto& source:sources) {
            if(!funActorCommittedWithRig(newest,source.frame)) continue;
            auto mix=funSpatialMix(camera,source.position);if(!mix.located) continue;
            const float score=(source.full?100.f:0.f)+mix.gain;
            if(score>best) {best=score;selected=mix;}
        }
        return selected;
    }
    FunAnimatedView cameraAtAnimatedBody(const GameCamera& camera) const {
        if(!modelOrigin||modelOrigin.type!="idVec3"||modelOrigin.size!=12) return FunAnimatedView::Unknown;
        auto decision=FunAnimatedView::Unknown;
        for(const auto& target:emitters) {
            if(target.fullCinematic||!valid(target)||!cameraRig.matches(target.model,camera)) continue;
            float rigOrigin[3]{};if(!WallMemory{}.read(target.model+modelParms.offset+modelOrigin.offset,rigOrigin,12)) continue;
            bool renderedActor=false,actorPose=false;
            for(const auto& actor:emitters) {
                if(!actor.fullCinematic||!valid(actor)) continue;
                float actorOrigin[3]{};if(!WallMemory{}.read(actor.model+modelParms.offset+modelOrigin.offset,actorOrigin,12)) continue;
                float separation=0;
                for(unsigned i=0;i<3;++i) { const float delta=actorOrigin[i]-rigOrigin[i];separation+=delta*delta; }
                if(!std::isfinite(separation)||separation>.25f) continue;
                // Compare render commit generations, not broad entity-visible
                // flags (the latter remain true even on inactive scene actors).
                if(committedFrame&&committedFrame.size==4) {
                    uint32_t rigFrame=0,actorFrame=0;
                    if(!WallMemory{}.get(target.model+committedFrame.offset,rigFrame)||!WallMemory{}.get(actor.model+committedFrame.offset,actorFrame)||
                        !funActorCommittedWithRig(rigFrame,actorFrame)) continue;
                }
                renderedActor=true;actorPose=actorPose||cameraRig.matches(actor.model,camera);
            }
            const auto candidate=funAnimatedView(true,renderedActor,actorPose);
            if(candidate==FunAnimatedView::External) return candidate;
            decision=candidate;
        }
        return decision;
    }
};
