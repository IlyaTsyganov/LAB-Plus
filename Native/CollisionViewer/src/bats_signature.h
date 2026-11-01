#pragma once

// Whole audited calculation, with ONLY RIP-relative addresses/call operands
// removed. Member offsets, arithmetic, branches and register use stay exact.
// This can recognize equivalent builds without trusting an arbitrary float.
static uint64_t batsCodeHash(const unsigned char* bytes,size_t size,std::initializer_list<size_t> relocations={}) {
    uint64_t hash=14695981039346656037ULL;
    for(size_t i=0;i<size;++i) {
        bool relocation=false;
        for(auto offset:relocations) if(i>=offset&&i-offset<4) { relocation=true; break; }
        hash=(hash^(relocation?0:bytes[i]))*1099511628211ULL;
    }
    return hash;
}
static bool decodeBatsSignature(const WallMemory& memory,uintptr_t base,const GameProfile& profile,uintptr_t rva,ConfiguredBatsCode& result) {
    unsigned char calculation[529]{},dexterity[44]{},status[143]{},getter[5]{};
    if(rva>=profile.imageSize||sizeof(calculation)>profile.imageSize-rva||!memory.read(base+rva,calculation,sizeof(calculation))) return false;
    const auto hash=batsCodeHash(calculation,sizeof(calculation),{5,12,0x51,0x64,0xc2,0xcf,0x121,0x192,0x1a9,0x20d});
    const bool legacy=hash==0xd9a3484803076231ULL||hash==0xbd1015fbf9a8eb8dULL;
    if(!legacy&&hash!=0x1ec9da5bc647929bULL) return false;
    auto call=[&](size_t operand,uintptr_t& target) {
        int32_t displacement=0; memcpy(&displacement,calculation+operand,4);
        const int64_t address=static_cast<int64_t>(rva+operand+4)+displacement;
        if(address<0||uint64_t(address)>=profile.imageSize) return false;
        target=static_cast<uintptr_t>(address); return true;
    };
    uintptr_t decl=0,secondDecl=0,thirdDecl=0,dex=0,effects=0;
    if(!call(0x51,decl)||!call(0xcf,secondDecl)||!call(0x121,thirdDecl)||decl!=secondDecl||decl!=thirdDecl||
       !call(0x192,dex)||!call(0x1a9,effects)||5>profile.imageSize-decl||44>profile.imageSize-dex||143>profile.imageSize-effects||
       !memory.read(base+decl,getter,sizeof(getter))||!memory.read(base+dex,dexterity,sizeof(dexterity))||!memory.read(base+effects,status,sizeof(status))) return false;
    if(batsCodeHash(getter,5)!=0x71fa801ff8c90d42ULL||
       batsCodeHash(dexterity,44,{0x1b,0x27})!=(legacy?0xf3f2892f96fea768ULL:0xe2115b39d5d0612bULL)||
       batsCodeHash(status,143,{0x18})!=(hash==0xd9a3484803076231ULL?0xcb8f1cd0a336fadeULL:0x2d3d2aec40399026ULL)) return false;
    auto constant=[&](uintptr_t block,const unsigned char* bytes,size_t operand,float expected) {
        int32_t displacement=0;memcpy(&displacement,bytes+operand,4);
        const int64_t address=static_cast<int64_t>(block+operand+4)+displacement;
        float actual=0;
        return address>=0&&uint64_t(address)+4<=profile.imageSize&&memory.get(base+static_cast<uintptr_t>(address),actual)&&actual==expected;
    };
    if(!constant(rva,calculation,5,1.f)||!constant(rva,calculation,0x64,.001f)||!constant(rva,calculation,0xc2,.001f)||
       !constant(dex,dexterity,0x1b,std::numeric_limits<float>::epsilon())||!constant(dex,dexterity,0x27,1.f)||!constant(effects,status,0x18,1.f)) return false;
    result={profile.timestamp,profile.imageSize,rva,dex,decl,effects,batsCodeHash(calculation,529),batsCodeHash(dexterity,44),batsCodeHash(status,143)};
    return true;
}
static bool findBatsSignature(const WallMemory& memory,uintptr_t base,const GameProfile& profile,ConfiguredBatsCode& result) {
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS64 nt{};
    if(!memory.get(base,dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0||uint32_t(dos.e_lfanew)>0x100000||
       !memory.get(base+dos.e_lfanew,nt)||nt.Signature!=IMAGE_NT_SIGNATURE||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||
       nt.OptionalHeader.SizeOfImage!=profile.imageSize||nt.FileHeader.TimeDateStamp!=profile.timestamp||
       !nt.FileHeader.NumberOfSections||nt.FileHeader.NumberOfSections>96||nt.FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64)) return false;
    unsigned matches=0;
    const uintptr_t sections=base+dos.e_lfanew+offsetof(IMAGE_NT_HEADERS64,OptionalHeader)+nt.FileHeader.SizeOfOptionalHeader;
    for(unsigned i=0;i<nt.FileHeader.NumberOfSections;++i) {
        IMAGE_SECTION_HEADER section{};
        if(!memory.get(sections+i*sizeof(section),section)) return false;
        if(!(section.Characteristics&IMAGE_SCN_MEM_EXECUTE)) continue;
        const uint64_t end=uint64_t(section.VirtualAddress)+section.Misc.VirtualSize;
        if(end>profile.imageSize||section.Misc.VirtualSize>0x8000000) return false;
        constexpr size_t chunk=65536; std::vector<unsigned char> bytes(chunk+528);
        for(uint64_t at=section.VirtualAddress;at<end;at+=chunk) {
            const size_t size=static_cast<size_t>(std::min<uint64_t>(bytes.size(),end-at));
            if(!memory.read(base+static_cast<uintptr_t>(at),bytes.data(),size)) return false;
            for(size_t n=0;n<std::min(chunk,size);++n) {
                if(n+12>size||memcmp(bytes.data()+n,"\xf3\x44\x0f\x10\x0d",5)||memcmp(bytes.data()+n+9,"\x48\x8b\x0d",3)) continue;
                ConfiguredBatsCode candidate{};
                if(decodeBatsSignature(memory,base,profile,static_cast<uintptr_t>(at+n),candidate)) { if(++matches>1) return false; result=candidate; }
            }
        }
    }
    return matches==1;
}
static bool resolveBatsSignature(const WallMemory& memory,uintptr_t base,const GameProfile& profile,ConfiguredBatsCode& result) {
    // Cache the bounded scan, then recheck exact fingerprints on every sample.
    static std::mutex mutex; static uintptr_t cachedBase=0; static HANDLE process=nullptr;
    static uint32_t timestamp=0,size=0; static bool found=false; static ConfiguredBatsCode code{};
    std::lock_guard<std::mutex> lock(mutex);
    if(cachedBase!=base||process!=memory.remote||timestamp!=profile.timestamp||size!=profile.imageSize) {
        cachedBase=base;process=memory.remote;timestamp=profile.timestamp;size=profile.imageSize;
        found=findBatsSignature(memory,base,profile,code);
    }
    if(found) result=code;
    return found;
}
