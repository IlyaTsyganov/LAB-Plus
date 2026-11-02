#pragma once

// Dense-map values follow the allocated key capacity, rounded to 8 bytes.
// Using the live key count here selects another parameter whenever spare
// capacity crosses a 16-byte boundary (as it does in the shell visor).
static uintptr_t funParmValue(uintptr_t block,uint16_t key) {
    struct Header { uint32_t count,capacity; uintptr_t data; } h{};
    if(!WallMemory{}.get(block,h)) return 0;
    h.count&=0x7fffffff; // growable blocks use bit 31 to mark inline storage
    if(!h.count||h.count>h.capacity||h.capacity>4096||!h.data) return 0;
    std::vector<uint16_t> keys(h.count);
    if(!WallMemory{}.read(h.data,keys.data(),keys.size()*2)) return 0;
    for(size_t i=0;i<keys.size();++i) {
        if(i&&keys[i]<=keys[i-1]) return 0;
    }
    auto found=std::lower_bound(keys.begin(),keys.end(),key);
    if(found==keys.end()||*found!=key) return 0;
    return h.data+((size_t(h.capacity)*2+7)&~size_t(7))+size_t(found-keys.begin())*16;
}
static bool funVisorMaterial(const std::string& name) {
    return name.rfind("models/characters/doomslayer/",0)==0&&name.size()>=13&&
        name.compare(name.size()-13,13,"_helmet_visor")==0;
}
class FunVisor {
    struct Target { uintptr_t material=0,address=0,block=0,surface=0,model=0,slot=0,entity=0; uint64_t identity=0; uint32_t stale=0,id=0,generation=0; bool inserted=false; float saved[4]{},last[4]{}; };
    std::vector<Target> targets;
    InspectorField name,identity,stale,parms,atomic;
    InspectorField surfaceParms,surfaceMaterial,modelSurfaces,renderInfo,modelPointer;
    uint32_t surfaceSize=0;
    uint16_t albedo=0;
    bool valid(const Target& t) const {
        uint64_t id=0;uint32_t generation=0;uintptr_t text=0;
        if(t.surface) {
            struct Slot { uint32_t id,generation;uintptr_t entity,previous,next; } owner{};
            uintptr_t model=0,list=0,material=0;uint32_t count=0;
            if(!WallMemory{}.get(t.slot,owner)||owner.id!=t.id||owner.generation!=t.generation||owner.entity!=t.entity||
                !WallMemory{}.get(t.entity+renderInfo.offset+modelPointer.offset,model)||model!=t.model||
                !WallMemory{}.get(model+modelSurfaces.offset,list)||!WallMemory{}.get(model+modelSurfaces.offset+8,count)||count>256||
                t.surface<list||!surfaceSize||(t.surface-list)%surfaceSize||(t.surface-list)/surfaceSize>=count||
                !WallMemory{}.get(t.surface+surfaceMaterial.offset,material)||material!=t.material) return false;
        }
        return WallMemory{}.get(t.material+identity.offset,id)&&id==t.identity&&
            WallMemory{}.get(t.material+stale.offset,generation)&&generation==t.stale&&
            WallMemory{}.get(t.material+name.offset+atomic.offset,text)&&funVisorMaterial(readEntityText(WallMemory{},text,512))&&
            funParmValue(t.block,albedo)==t.address;
    }
    static bool read(uintptr_t at,float* color) {
        if(!WallMemory{}.read(at,color,16)) return false;
        for(unsigned i=0;i<4;++i) if(!std::isfinite(color[i])||color[i]<0||color[i]>1) return false;
        return true;
    }
    static bool bytes(uintptr_t at,const void* data,size_t size) {
        MEMORY_BASIC_INFORMATION page{};
        if(!VirtualQuery(reinterpret_cast<void*>(at),&page,sizeof(page))||page.State!=MEM_COMMIT||
            (page.Protect&(PAGE_GUARD|PAGE_NOACCESS))||!(page.Protect&(PAGE_READWRITE|PAGE_WRITECOPY))||
            page.RegionSize-(at-reinterpret_cast<uintptr_t>(page.BaseAddress))<size) return false;
        SIZE_T done=0;return WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),data,size,&done)&&done==size;
    }
    static bool write(uintptr_t at,const float* color) { return bytes(at,color,16); }
public:
    void initialize(const MeathookHudApi& api,uintptr_t base,const GameProfile& profile) {
        albedo=0;
        name=inspectorField(api,"idResource","name"); identity=inspectorField(api,"idResource","resourceID");
        stale=inspectorField(api,"idResource","staleCount");parms=inspectorField(api,"idMaterial2","Parms");atomic=inspectorField(api,"idAtomicString","str");
        surfaceParms=inspectorField(api,"idRenderModelSurface","parmBlock");surfaceMaterial=inspectorField(api,"idRenderModelSurface","material");
        modelSurfaces=inspectorField(api,"idRenderModel","surfaces");renderInfo=inspectorField(api,"idBloatedEntity","renderModelInfo");modelPointer=inspectorField(api,"idRenderModelInfo","modelPtr");
        if(auto type=reinterpret_cast<uintptr_t>(api.findClass("idRenderModelSurface"))) WallMemory{}.get(type+16,surfaceSize);
        auto index=inspectorField(api,"idDeclRenderParm","parmIndex"),listField=inspectorField(api,"idResourceList","resourceList"),
            classField=inspectorField(api,"idResourceList","className"),next=inspectorField(api,"idResourceList","next");
        if(!name||name.type!="idAtomicString"||!identity||identity.size!=8||!stale||stale.size!=4||!parms||parms.type!="idParmBlock"||parms.size!=16||
            !atomic||atomic.size!=8||!index||index.size!=4||!listField||listField.offset!=8||!classField||classField.offset!=24||!next||next.offset!=32) return;
        static constexpr unsigned char prefix[]{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x1d};
        static constexpr unsigned char suffix[]{0x48,0x8b,0xf9,0x48,0x85,0xdb,0x74};
        // Read-only registry discovery; no calls to guessed engine functions.
        constexpr size_t chunk=1024*1024; std::vector<unsigned char> bytes;
        std::unordered_set<uintptr_t> lists;
        for(size_t offset=0;offset<profile.typeGetterLimitRva;offset+=chunk) {
            size_t length=std::min(chunk+32,size_t(profile.typeGetterLimitRva)-offset);bytes.resize(length);
            if(!WallMemory{}.read(base+offset,bytes.data(),length)) continue;
            for(size_t i=0;i+24<=length&&i<chunk;++i) {
                if(memcmp(bytes.data()+i,prefix,sizeof(prefix))||memcmp(bytes.data()+i+17,suffix,sizeof(suffix))) continue;
                int32_t relative=0;memcpy(&relative,bytes.data()+i+13,4);uintptr_t slot=base+offset+i+17+relative,head=0;
                if(slot<base||slot-base+8>profile.imageSize||!WallMemory{}.get(slot,head)||!lists.insert(head).second) continue;
                std::unordered_set<uintptr_t> seen;
                while(head&&seen.size()<512&&seen.insert(head).second) {
                    uintptr_t text=0,list=0,array=0;uint32_t count=0,capacity=0;
                    if(!WallMemory{}.get(head+classField.offset,text)) break;
                    if(readEntityText(WallMemory{},text,128)=="idDeclRenderParm"&&WallMemory{}.get(head+listField.offset,list)&&
                        WallMemory{}.get(list+32,array)&&WallMemory{}.get(list+40,count)&&WallMemory{}.get(list+44,capacity)&&count<=capacity&&capacity<=65536) {
                        for(uint32_t n=0;n<count;++n) {
                            uintptr_t resource=0,resourceText=0;int value=0;
                            if(!WallMemory{}.get(array+size_t(n)*8,resource)||!WallMemory{}.get(resource+name.offset+atomic.offset,resourceText)||
                                readEntityText(WallMemory{},resourceText,128)!="glassalbedo"||!WallMemory{}.get(resource+index.offset,value)||value<=0||value>32767) continue;
                            albedo=static_cast<uint16_t>(value);note("Fun visor: resolved glassalbedo by resource name.");return;
                        }
                    }
                    if(!WallMemory{}.get(head+next.offset,head)) break;
                }
            }
        }
    }
    bool available() const { return albedo!=0; }
    bool selected() const { return !targets.empty(); }
    void consider(uintptr_t material) {
        if(!available()||targets.size()>=32||std::any_of(targets.begin(),targets.end(),[&](const auto& t){return t.material==material;})) return;
        uintptr_t text=0;
        if(!WallMemory{}.get(material+name.offset+atomic.offset,text)||!funVisorMaterial(readEntityText(WallMemory{},text,512))) return;
        Target t;t.material=material;t.block=material+parms.offset;t.address=funParmValue(t.block,albedo);
        if(!t.address||!WallMemory{}.get(material+identity.offset,t.identity)||!WallMemory{}.get(material+stale.offset,t.stale)||!read(t.address,t.saved)) return;
        memcpy(t.last,t.saved,16);targets.push_back(t);note("Fun visor: selected Slayer helmet glass; body materials excluded.");
    }
    void considerSurface(uintptr_t material,uintptr_t surface,uintptr_t model,uintptr_t slot,uintptr_t entity,uint32_t id,uint32_t generation) {
        targets.erase(std::remove_if(targets.begin(),targets.end(),[&](const auto& t){return !valid(t);}),targets.end());
        if(!available()||targets.size()>=32||std::any_of(targets.begin(),targets.end(),[&](const auto& t){return t.surface==surface;})) return;
        if(!surfaceParms||surfaceParms.type!="idGrowableParmBlock < 12 >"||surfaceParms.size<16||!surfaceSize||surfaceSize>4096||
            surfaceParms.offset+surfaceParms.size>surfaceSize||!surfaceMaterial||surfaceMaterial.size!=8||!modelSurfaces||modelSurfaces.size!=24||
            !renderInfo||!modelPointer||modelPointer.size!=8) return;
        uintptr_t text=0;
        if(!WallMemory{}.get(material+name.offset+atomic.offset,text)||!funVisorMaterial(readEntityText(WallMemory{},text,512))) return;
        Target t;t.material=material;t.surface=surface;t.model=model;t.slot=slot;t.entity=entity;t.id=id;t.generation=generation;t.block=surface+surfaceParms.offset;
        if(!WallMemory{}.get(material+identity.offset,t.identity)||!WallMemory{}.get(material+stale.offset,t.stale)) return;
        t.address=funParmValue(t.block,albedo);
        if(!t.address) {
            // Add a draw-time override only when the engine's existing inline
            // allocation has room. Never replace engine-owned allocations.
            struct Header { uint32_t count,capacity;uintptr_t data; } h{};
            if(!WallMemory{}.get(t.block,h)||!(h.count&0x80000000u)||h.capacity!=12||h.data!=t.block+16) return;
            auto count=h.count&0x7fffffff;
            if(count>=h.capacity||((h.capacity*2+7)&~7u)+h.capacity*16+16>surfaceParms.size) return;
            std::vector<uint16_t> keys(count);if(count&&!WallMemory{}.read(h.data,keys.data(),count*2)) return;
            // Existing engine surface parameters are sorted; append only.
            if(!std::is_sorted(keys.begin(),keys.end())||(!keys.empty()&&keys.back()>=albedo)) return;
            uintptr_t original=funParmValue(material+parms.offset,albedo);if(!original||!read(original,t.saved)) return;
            t.address=h.data+((h.capacity*2+7)&~7u)+count*16;
            uint32_t newCount=h.count+1;
            if(!write(t.address,t.saved)||!bytes(h.data+count*2,&albedo,2)||!bytes(t.block,&newCount,4)) return;
            t.inserted=true;
        } else if(!read(t.address,t.saved)) return;
        memcpy(t.last,t.saved,16);
        if(!valid(t)) return;
        targets.push_back(t);note("Fun visor: installed a per-surface glass colour override; body parameters are untouched.");
    }
    void update(const std::array<float,4>& hue) {
        for(auto& t:targets) {
            float current[4]{};if(!valid(t)||!read(t.address,current)) continue;
            if(memcmp(current,t.last,16)) memcpy(t.saved,current,16);
            // Preserve transparency. Replacing RGB avoids the original green
            // glass cancelling the red/purple portions of the cycle.
            for(unsigned i=0;i<3;++i) t.last[i]=hue[i]*.6f;
            t.last[3]=t.saved[3];write(t.address,t.last);
        }
    }
    void restore() {
        for(auto& t:targets) {
            float current[4]{};
            if(valid(t)&&read(t.address,current)&&!memcmp(current,t.last,16)) {
                if(t.inserted) {
                    uint32_t count=0,capacity=0;uintptr_t data=0;
                    if(WallMemory{}.get(t.block,count)&&WallMemory{}.get(t.block+4,capacity)&&WallMemory{}.get(t.block+8,data)&&capacity==12) {
                        const auto used=count&0x7fffffff;uint16_t key=0;
                        if(used&&WallMemory{}.get(data+(used-1)*2,key)&&key==albedo) {
                            uint16_t unused=0xffff;uint32_t previous=count-1;
                            bytes(t.block,&previous,4);bytes(data+(used-1)*2,&unused,2);
                        }
                    }
                } else write(t.address,t.saved);
            }
        }
        targets.clear();
    }
};
