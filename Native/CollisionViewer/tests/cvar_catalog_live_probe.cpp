#include "../src/vulkan_layer.cpp"
#include <tlhelp32.h>

// Read the actual engine registry. This probe never injects code, executes
// console commands, changes bindings, writes game memory or changes settings.
int wmain(int argc,wchar_t** argv) {
    if(argc!=2) { fputs("Usage: CvarCatalogLiveProbe <game-pid>\n",stderr); return 2; }
    DWORD pid=static_cast<DWORD>(_wtoi(argv[1]));
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
    for(unsigned retry=0;snapshot==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_BAD_LENGTH&&retry<10;++retry) {
        Sleep(10); snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
    }
    if(snapshot==INVALID_HANDLE_VALUE) { fprintf(stderr,"Cannot enumerate modules for PID %lu: Windows error %lu\n",pid,GetLastError()); return 3; }
    std::vector<MODULEENTRY32W> modules; MODULEENTRY32W module{}; module.dwSize=sizeof(module);
    if(Module32FirstW(snapshot,&module)) do { modules.push_back(module); } while(Module32NextW(snapshot,&module));
    CloseHandle(snapshot);
    HANDLE process=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,pid);
    if(!process) { fprintf(stderr,"Cannot read PID %lu: Windows error %lu\n",pid,GetLastError()); return 4; }
    int result=0;
    try {
        WallMemory memory{process}; uintptr_t base=0;
        for(const auto& entry:modules) if(!_wcsicmp(entry.szModule,L"DOOMEternalx64vk.exe")) base=reinterpret_cast<uintptr_t>(entry.modBaseAddr);
        const auto* profile=detectGameProfile(memory,base);
        if(!profile) throw std::runtime_error("Unsupported game image");
        LabCommandSystem command;
        for(const auto& entry:modules) if(!_wcsicmp(entry.szModule,L"XINPUT1_3.dll")) {
            uintptr_t hook=reinterpret_cast<uintptr_t>(entry.modBaseAddr);
            IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS64 pe{};
            if(!memory.get(hook,dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||!memory.get(hook+dos.e_lfanew,pe)||pe.Signature!=IMAGE_NT_SIGNATURE) continue;
            for(unsigned index=0;index<pe.FileHeader.NumberOfSections;++index) {
                IMAGE_SECTION_HEADER section{};
                uintptr_t at=hook+dos.e_lfanew+offsetof(IMAGE_NT_HEADERS64,OptionalHeader)+pe.FileHeader.SizeOfOptionalHeader+index*sizeof(section);
                if(!memory.get(at,section)||!(section.Characteristics&IMAGE_SCN_MEM_EXECUTE)||section.Misc.VirtualSize>64*1024*1024) continue;
                std::vector<unsigned char> bytes(section.Misc.VirtualSize);
                if(!memory.read(hook+section.VirtualAddress,bytes.data(),bytes.size())) continue;
                const unsigned char prefix[]{0x48,0x89,0xca,0x48,0x8b,0x05};
                for(size_t offset=0;offset+23<=bytes.size();++offset) if(!memcmp(bytes.data()+offset,prefix,sizeof(prefix))) {
                    auto candidate=resolveLabCommandSystem(memory,hook+section.VirtualAddress+offset,base,*profile);
                    if(!candidate) continue;
                    if(command&&command.slot!=candidate.slot) throw std::runtime_error("Ambiguous engine command systems");
                    command=candidate;
                }
            }
        }
        auto system=resolveLabCvarSystem(memory,command,base,*profile);
        if(!resolveLabCvarFactory(memory,system,base,*profile)) throw std::runtime_error("Native cvar ABI validation failed");
        // Audited idCVarSystemLocal registration stores idCVar* in its primary
        // idList at system->implementation (+8), data (+0), count (+8).
        uintptr_t list=0,array=0; int count=0;
        if(!memory.get(system.object+8,list)||!memory.get(list,array)||!memory.get(list+8,count)||count<1||count>100000) throw std::runtime_error("Invalid engine cvar list");
        std::unordered_map<std::string,uintptr_t> registered;
        for(int index=0;index<count;++index) {
            uintptr_t variable=0,data=0,name=0;
            if(!memory.get(array+size_t(index)*8,variable)||!memory.get(variable,data)||!memory.get(data+40,name)) continue;
            auto text=readEntityText(memory,name,128);
            if(text.rfind("labplus_",0)==0&&!registered.emplace(text,variable).second) throw std::runtime_error("Duplicate LAB Plus cvar name");
        }
        printf("Profile: %s\nname\tvalue\tflags\tminimum\tmaximum\n",profile->name);
        unsigned checked=0;
        for(const auto& definition:nativeLabCvarDefinitions()) if(definition.minimum==0&&definition.maximum==1) {
            auto found=registered.find(definition.name);
            if(found==registered.end()) throw std::runtime_error(std::string("Missing native cvar: ")+definition.name);
            int value=0; std::string text; uintptr_t data=0; unsigned flags=0; float minimum=0,maximum=0;
            if(!readLabCvarValue(memory,found->second,definition.name,value,text)||!memory.get(found->second,data)||
               !memory.get(data+64,flags)||!memory.get(data+68,minimum)||!memory.get(data+72,maximum)||
               (flags&3)!=1||!(flags&16)||minimum!=0||maximum!=1||(value!=0&&value!=1))
                throw std::runtime_error(std::string("Invalid boolean cvar metadata: ")+definition.name);
            printf("%s\t%d\t0x%x\t%.0f\t%.0f\n",definition.name,value,flags,minimum,maximum); ++checked;
        }
        printf("PASS: %u native boolean cvar names exist in the live engine registry with BOOL/NOCHEAT flags, 0..1 bounds and binary values. No commands executed or settings changed.\n",checked);
    } catch(const std::exception& error) { fprintf(stderr,"FAIL: %s\n",error.what()); result=1; }
    CloseHandle(process); return result;
}
