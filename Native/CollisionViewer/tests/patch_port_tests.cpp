#include "../src/vulkan_layer.cpp"

// Optional integration audit of user-supplied EXEs. They are mapped as data,
// never loaded/executed and never bundled with the source or test suite.
static void requirePort(bool value,const char* why) { if(!value) throw std::runtime_error(why); }
template<class T> static void portPut(uintptr_t at,T value) { memcpy(reinterpret_cast<void*>(at),&value,sizeof(value)); }
#include "praetor_port_audit.h"
int wmain(int argc,wchar_t** argv) {
    if(argc!=3) { puts("Usage: PatchPortTests retail-exe metadata-rvas.tsv");return 2; }
    unsigned char* image=nullptr;
    try {
        std::ifstream input(std::filesystem::path(argv[1]),std::ios::binary);
        std::vector<unsigned char> file((std::istreambuf_iterator<char>(input)),{});
        requirePort(file.size()>4096,"EXE missing");
        const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
        requirePort(dos->e_magic==IMAGE_DOS_SIGNATURE&&dos->e_lfanew>0&&size_t(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS64)<file.size(),"Invalid PE");
        const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(file.data()+dos->e_lfanew);
        requirePort(nt->Signature==IMAGE_NT_SIGNATURE&&nt->OptionalHeader.SizeOfImage<0x30000000,"Invalid image");
        image=static_cast<unsigned char*>(VirtualAlloc(nullptr,nt->OptionalHeader.SizeOfImage,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        requirePort(image!=nullptr,"Cannot allocate data image");const uintptr_t base=reinterpret_cast<uintptr_t>(image);
        memcpy(image,file.data(),nt->OptionalHeader.SizeOfHeaders);
        auto* section=IMAGE_FIRST_SECTION(nt);
        for(unsigned n=0;n<nt->FileHeader.NumberOfSections;++n) {
            requirePort(uint64_t(section[n].VirtualAddress)+section[n].SizeOfRawData<=nt->OptionalHeader.SizeOfImage&&
                uint64_t(section[n].PointerToRawData)+section[n].SizeOfRawData<=file.size(),"Invalid section bounds");
            memcpy(image+section[n].VirtualAddress,file.data()+section[n].PointerToRawData,section[n].SizeOfRawData);
        }
        const auto& reloc=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        const uint64_t delta=base-nt->OptionalHeader.ImageBase;
        for(uint32_t pos=0;pos<reloc.Size;) {
            auto* block=reinterpret_cast<IMAGE_BASE_RELOCATION*>(image+reloc.VirtualAddress+pos);
            requirePort(block->SizeOfBlock>=8&&block->SizeOfBlock<=reloc.Size-pos,"Invalid relocation");
            auto* values=reinterpret_cast<uint16_t*>(block+1);
            for(unsigned i=0;i<(block->SizeOfBlock-8)/2;++i) if(values[i]>>12==IMAGE_REL_BASED_DIR64) {
                const uint64_t offset=uint64_t(block->VirtualAddress)+(values[i]&4095);
                requirePort(offset+8<=nt->OptionalHeader.SizeOfImage,"Relocation outside image");
                *reinterpret_cast<uint64_t*>(image+offset)+=delta;
            }
            pos+=block->SizeOfBlock;
        }
        WallMemory memory{};const auto* profile=detectGameProfile(memory,base);requirePort(profile,"Profile identity failed");
        const auto* code=observationCodeProfile(*profile);requirePort(code,"Observation profile missing");
        requirePort(auditSyncReleaseRegistration(memory,base,*profile),"Retail release-event registration/signature differs from fixture");
        std::ifstream metadata{std::filesystem::path(argv[2])};std::string line;
        std::unordered_map<std::string,uintptr_t> addresses;
        while(std::getline(metadata,line)) {auto tab=line.find('\t');if(tab!=std::string::npos)addresses[line.substr(0,tab)]=base+std::stoull(line.substr(tab+1),nullptr,0);}
        std::unordered_map<std::string,GraphType> schemas;
        GraphLookup lookup=[&](const std::string& type)->const GraphType& {
            auto found=schemas.find(type);if(found!=schemas.end())return found->second;
            auto address=addresses.find(type);if(address==addresses.end()) {printf("Missing type: %s\n",type.c_str());static const GraphType empty;return empty;}
            return schemas.emplace(type,readGraphType(memory,address->second,type)).first->second;
        };
        auto layout=resolveEnemyTelemetryLayout(lookup);
        if(!layout.valid) {
            printf("Health layout: pending=%d think=%x name=%x health=%x lists=%zu\n",layout.pending,layout.think,layout.name,layout.health,layout.lists.size());
            for(const auto& entry:schemas) {
                printf("Schema %s complete=%d size=%u parent=%s fields=%zu\n",entry.first.c_str(),entry.second.complete,entry.second.size,entry.second.parent.c_str(),entry.second.fields.size());
                if(entry.first=="idAIHealth"||entry.first.find("idShieldHealthT")==0||entry.first.find("idHealthT")==0)
                    for(const auto& field:entry.second.fields) printf("  %s %s %s off=%x size=%u\n",field.name.c_str(),field.type.c_str(),field.ops.c_str(),field.offset,field.size);
            }
        }
        requirePort(layout.valid,"Retail health reflection failed");
        uint32_t pain=~0u;requirePort(resolveEnemyStaggerPointer(memory,base,*profile,layout,pain),"Stagger getters failed");
        const auto* bats=configuredBatsCode(*profile);requirePort(bats&&configuredBatsFingerprint(memory,base,*profile,*bats),"BATS fingerprints/constants failed");
        uint32_t native=0;
        auto desired=techField(lookup,"idDeclWeapon","desiredBringupDurationMS"),ms=techField(lookup,"milliToGameTime_t","milliseconds"),
            mods=techField(lookup,"idWeapon","weaponModOverrides"),scale=techField(lookup,"idWeapon::weaponModOverrides_t","bringUpRateScale");
        requirePort(desired&&ms&&mods&&scale&&memory.get(base+bats->calculation+bats->desiredOperand,native)&&native==desired->offset+ms->offset&&
            memory.get(base+bats->calculation+bats->scaleOperand,native)&&native==mods->offset+scale->offset,"BATS code/reflection mismatch");
        // Mutate ONLY our private data copy. The actual retail file/game is
        // untouched. Dependency corruption must fail closed.
        image[bats->calculation+32]^=1;requirePort(!configuredBatsFingerprint(memory,base,*profile,*bats),"Changed BATS calculation accepted");image[bats->calculation+32]^=1;
        image[code->finish+10]^=4;requirePort(!resolveEnemyStaggerPointer(memory,base,*profile,layout,pain),"Changed stagger displacement accepted");image[code->finish+10]^=4;
        int32_t relative=0;uint32_t manager=0;uintptr_t getter=0;
        memory.get(base+code->clock+3,relative);const auto mapGlobal=base+code->clock+7+relative;
        memory.get(base+code->clock+22,manager);memory.get(base+code->clock+33,relative);getter=base+code->clock+37+relative;
        memory.get(getter+3,relative);const auto flagGlobal=getter+7+relative;
        std::vector<unsigned char> map(manager+32),flag(32),ticks(192);
        portPut(mapGlobal,reinterpret_cast<uintptr_t>(map.data()));portPut(flagGlobal,reinterpret_cast<uintptr_t>(flag.data()));
        portPut(reinterpret_cast<uintptr_t>(map.data())+manager+16,reinterpret_cast<uintptr_t>(ticks.data()));
        portPut(reinterpret_cast<uintptr_t>(ticks.data())+24,int64_t(9999600));portPut(reinterpret_cast<uintptr_t>(ticks.data())+96,int64_t(4999800));
        auto flame=timerField(lookup,"idPlayer","flameBelchNextUseTime");requirePort(timerTimeField(flame),"Flame timer type mismatch");
        LevelIdentity level{reinterpret_cast<uintptr_t>(map.data()),1,"fixture"};
        auto clock=readGameTimerClock(memory,base,*profile,level,flame->offset);requirePort(clock.valid&&clock.ticks==9999600,"Unscaled clock path failed");
        portPut(reinterpret_cast<uintptr_t>(flag.data())+8,int32_t(1));clock=readGameTimerClock(memory,base,*profile,level,flame->offset);
        requirePort(clock.valid&&clock.ticks==4999800,"Scaled clock domain1 path failed");
        requirePort(!readGameTimerClock(memory,base,*profile,level,flame->offset+8).valid,"Foreign flame offset accepted");
        image[code->clock+28]^=1;requirePort(!readGameTimerClock(memory,base,*profile,level,flame->offset).valid,"Wrong clock domain accepted");image[code->clock+28]^=1;
        memory.get(base+code->powerup+3,relative);const auto overrideGlobal=base+code->powerup+7+relative;
        std::array<unsigned char,24> override{};portPut(overrideGlobal,reinterpret_cast<uintptr_t>(override.data()));portPut(reinterpret_cast<uintptr_t>(override.data())+12,30.f);
        double seconds=0;requirePort(readPowerupLifetimeOverride(memory,base,*profile,seconds)&&seconds==30,"Powerup override decoder failed");
        // Check event getter and reflected ownership even when no live event
        // table or sync exists in this offline image.
        auto participant=timerField(lookup,"idSyncEntity","syncPlayer"),working=timerField(lookup,"idSyncEntity","isCurrentlyWorking");
        requirePort(timerInline(participant,"idManagedClassPtr < idPlayer >",lookup)&&timerBoolField(working)&&
            timerPointer(timerField(lookup,participant->type,"ptr"),"idPlayer"),"Sync ownership schema failed");
        auto eventName=timerField(lookup,"idEventDef","name"),eventNumber=timerField(lookup,"idEventDef","eventnum"),
            eventKind=timerField(lookup,"idEventDef","type"),eventArgs=timerField(lookup,"idEventDef","numargs");
        requirePort(eventName&&eventNumber&&eventKind&&eventArgs,"Event definition metadata failed");
        memory.get(base+code->event+18,relative);const auto eventTable=base+code->event+22+relative;
        const auto eventDefinition=base+profile->imageSize-4096;
        memset(reinterpret_cast<void*>(eventTable),0,4096*8);portPut(eventTable,eventDefinition);
        portPut(eventDefinition+eventName->offset,reinterpret_cast<uintptr_t>("ae_releasePlayerFromSync"));
        portPut(eventDefinition+eventNumber->offset,int32_t(0));portPut(eventDefinition+eventKind->offset,int32_t(2));portPut(eventDefinition+eventArgs->offset,int32_t(1));
        portPut(eventDefinition+timerField(lookup,"idEventDef","formatspec")->offset,reinterpret_cast<uintptr_t>("4"));
        portPut(eventDefinition+timerField(lookup,"idEventDef","argTypes")->offset,reinterpret_cast<uintptr_t>("idMD6Anim*;"));
        portPut(eventDefinition+timerField(lookup,"idEventDef","argNames")->offset,reinterpret_cast<uintptr_t>("anim;"));
        requirePort(syncReleaseEventNumber(memory,base,*profile,lookup)==0,"Retail event getter/schema failed");
        portPut(eventDefinition+eventKind->offset,int32_t(0));requirePort(syncReleaseEventNumber(memory,base,*profile,lookup)<0,"Foreign event kind accepted");
        portPut(eventDefinition+eventKind->offset,int32_t(2));
        auditPraetorPort(memory,base,*profile,lookup,0);
        printf("PASS %s: health stride=%u/max=0x%x; stagger; BATS code/dependencies; clock branches/domain guards; powerup; sync ownership\n",profile->name,layout.componentSize,layout.maximum);
        VirtualFree(image,0,MEM_RELEASE);return 0;
    } catch(const std::exception& error) {fprintf(stderr,"FAIL: %s\n",error.what());if(image)VirtualFree(image,0,MEM_RELEASE);return 1;}
}
