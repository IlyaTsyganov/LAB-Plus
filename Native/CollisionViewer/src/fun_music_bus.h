#pragma once

struct FunMusicCall { uintptr_t setter=0; uint32_t idOffset=0; explicit operator bool() const { return setter&&idOffset; } };
// This is the engine's own six-argument Wwise SetRTPCValue call, identified
// specifically by its reference to the resolved s_volumeMusic object.
static FunMusicCall decodeFunMusicCall(const unsigned char* code,uintptr_t at,uintptr_t musicObject) {
    static constexpr unsigned char load[]{0x48,0x8b,0x05};
    static constexpr unsigned char transition[]{0x41,0xb9,0xf4,0x01,0x00,0x00,0x8b,0x8b};
    static constexpr unsigned char arguments[]{0x49,0x83,0xc8,0xff,0xc6,0x44,0x24,0x28,0x00,0xc7,0x44,0x24,0x20,0x04,0x00,0x00,0x00,
        0xf3,0x0f,0x10,0x40,0x0c,0x0f,0x28,0xc8,0xf3,0x0f,0x5d,0xcf,0xf3,0x0f,0x5f,0xce,0x0f,0x28,0xc9,0xe8};
    int32_t global=0,relative=0; uint32_t offset=0;
    if(memcmp(code,load,sizeof(load))||memcmp(code+7,transition,sizeof(transition))||memcmp(code+19,arguments,sizeof(arguments))) return {};
    memcpy(&global,code+3,4); memcpy(&offset,code+15,4); memcpy(&relative,code+56,4);
    if(at+7+global!=musicObject||offset<8||offset>0x400000) return {};
    return {at+60+relative,offset};
}
static bool funRtpcWrapper(const unsigned char* code) {
    static constexpr unsigned char prefix[]{0x48,0x83,0xec,0x48,0x0f,0xb6,0x44,0x24,0x78,0x88,0x44,0x24,0x30,0x8b,0x44,0x24,0x70,
        0x89,0x44,0x24,0x28,0x44,0x89,0x4c,0x24,0x20,0x45,0x33,0xc9,0xe8};
    static constexpr unsigned char suffix[]{0x48,0x83,0xc4,0x48,0xc3};
    return !memcmp(code,prefix,sizeof(prefix))&&!memcmp(code+sizeof(prefix)+4,suffix,sizeof(suffix));
}
class FunMusicBus {
    uintptr_t setter=0,sound=0,vtable=0,idAddress=0;
    uint32_t id=0;
public:
    bool connect(const MeathookHudApi& api,uintptr_t musicObject,uintptr_t base,const GameProfile& profile) {
        setter=0;
        auto tools=inspectorField(api,"engine_t","typeInfoTools"),audio=inspectorField(api,"engine_t","soundSystem");
        if(!tools||tools.type!="idTypeInfoTools*"||tools.size!=8||!audio||audio.type!="idSoundSystem*"||audio.size!=8) return false;
        uintptr_t thunk=reinterpret_cast<uintptr_t>(api.findClass),slot=0; unsigned char bytes[64]{};
        for(unsigned i=0;i<4;++i) {
            if(!WallMemory{}.read(thunk,bytes,32)) return false;
            if(bytes[0]!=0xe9) break;
            int32_t relative=0; memcpy(&relative,bytes+1,4); thunk+=5+relative;
        }
        static constexpr unsigned char prefix[]{0x48,0x89,0xca,0x48,0x8b,0x05};
        static constexpr unsigned char suffix[]{0x48,0x8b,0x08,0x48,0x8b,0x05};
        int32_t relative=0;
        if(memcmp(bytes,prefix,sizeof(prefix))||memcmp(bytes+10,suffix,sizeof(suffix))) return false;
        memcpy(&relative,bytes+6,4);
        if(!WallMemory{}.get(thunk+10+relative,slot)||slot<base+tools.offset||slot-base>=profile.imageSize||
           !WallMemory{}.get(slot-tools.offset+audio.offset,sound)||!WallMemory{}.get(sound,vtable)) return false;
        uintptr_t locator=0; uint32_t rtti[6]{};
        if(vtable<base||vtable-base>=profile.imageSize||!WallMemory{}.get(vtable-8,locator)||locator<base||locator-base>=profile.imageSize||
           !WallMemory{}.read(locator,rtti,sizeof(rtti))||rtti[0]!=1||locator-base!=rtti[5]||rtti[3]>=profile.imageSize||
           readEntityText(WallMemory{},base+rtti[3]+16,128)!=".?AVidSoundSystemLocal@@") return false;
        IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS64 nt{};
        if(!WallMemory{}.get(base,dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<=0||dos.e_lfanew>0x100000||
           !WallMemory{}.get(base+dos.e_lfanew,nt)||nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.NumberOfSections>96) return false;
        auto sections=base+dos.e_lfanew+24+nt.FileHeader.SizeOfOptionalHeader; FunMusicCall selected;
        for(unsigned section=0;section<nt.FileHeader.NumberOfSections;++section) {
            IMAGE_SECTION_HEADER row{}; if(!WallMemory{}.get(sections+section*sizeof(row),row)) return false;
            if(!(row.Characteristics&IMAGE_SCN_MEM_EXECUTE)||row.VirtualAddress>=profile.imageSize||row.Misc.VirtualSize>profile.imageSize-row.VirtualAddress) continue;
            constexpr size_t chunk=1024*1024; std::vector<unsigned char> data;
            for(size_t offset=0;offset<row.Misc.VirtualSize;offset+=chunk) {
                size_t size=std::min(chunk+64,size_t(row.Misc.VirtualSize)-offset); data.resize(size);
                auto address=base+row.VirtualAddress+offset; if(!WallMemory{}.read(address,data.data(),size)) return false;
                for(size_t i=0;i+60<=size&&i<chunk;++i) {
                    auto call=decodeFunMusicCall(data.data()+i,address+i,musicObject); if(!call) continue;
                    if(selected) return false; selected=call;
                }
            }
        }
        if(!selected||selected.setter<base||selected.setter-base>=profile.imageSize||
           !WallMemory{}.read(selected.setter,bytes,39)||!funRtpcWrapper(bytes)) return false;
        idAddress=sound+selected.idOffset;
        if(!WallMemory{}.get(idAddress,id)||!id||id==~0u) return false;
        setter=selected.setter; note("Fun music: validated the engine music RTPC route; other sound buses are untouched."); return true;
    }
    bool set(float volume,unsigned milliseconds=0) const {
        uintptr_t currentVtable=0; uint32_t currentId=0;
        if(!setter||!std::isfinite(volume)||volume<0||volume>1||!WallMemory{}.get(sound,currentVtable)||currentVtable!=vtable||
           !WallMemory{}.get(idAddress,currentId)||currentId!=id) return false;
        using Set=int (*)(uint32_t,float,uint64_t,int,int,bool);
        return reinterpret_cast<Set>(setter)(id,volume,~uint64_t(0),static_cast<int>(milliseconds),4,false)==1;
    }
};
