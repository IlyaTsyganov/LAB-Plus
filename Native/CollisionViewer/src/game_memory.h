#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>

static bool guardedCopy(uintptr_t address,void* output,size_t bytes) {
    if(address<0x10000||address>0x7fffffffffffULL||bytes>0x1000000) return false;
    // LLVM-MinGW otherwise treats the memcpy intrinsic as non-throwing and
    // can move its memory access outside the structured exception region.
    // An opaque call keeps the existing SEH boundary, without a syscall for
    // each camera/physics field. The MSVC build retains its original path.
#if defined(__MINGW32__)
    using Copy = void*(*)(void*,const void*,size_t);
    static Copy volatile copy = &memcpy;
    __try { copy(output,reinterpret_cast<const void*>(address),bytes); return true; }
#else
    __try { memcpy(output,reinterpret_cast<const void*>(address),bytes); return true; }
#endif
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
struct WallMemory {
    HANDLE remote{};
    bool read(uintptr_t address,void* output,size_t size) const {
        if(!remote) return guardedCopy(address,output,size);
        SIZE_T got=0; return ReadProcessMemory(remote,reinterpret_cast<void*>(address),output,size,&got)&&got==size;
    }
    template<class T> bool get(uintptr_t address,T& value) const { return read(address,&value,sizeof(value)); }
};
