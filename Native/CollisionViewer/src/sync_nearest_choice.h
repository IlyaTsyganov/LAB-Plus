#pragma once
#include "sync_patch_layout.h"

// Retail 1.1 normal player input: 16996c7 -> 18d30c0 -> 1699857.
// This is a different selector from the random-band route at 18d2600.
static bool syncNearestChoiceProfile(const WallMemory& memory,uintptr_t base) {
    if(syncPatch().modern)return syncPatchCodeMatches(memory,base);
    const unsigned char entry[]{0x44,0x89,0x4c,0x24,0x20};
    const unsigned char compare[]{0x4d,0x85,0xe4,0x74,0x2d,0xf3,0x0f,0x10,0x43,0x4c,0x0f,0x2f,0xc6,0x77,0x23,0x0f,0x2e,0xc6,0x7a,0x29,0x75,0x27,0x0f,0x2f,0xfa,0x44,0x0f,0xb6,0x4d,0x7f,0x0f,0x86,0xc4,0xfd,0xff,0xff};
    const unsigned char choose[]{0xe8,0xf4,0x99,0x23,0},execute[]{0xe8,0xd4,0x81,0x23,0};unsigned char code[sizeof(compare)]{};
    return memory.read(base+0x18d30c0,code,sizeof(entry))&&!memcmp(code,entry,sizeof(entry))&&
        memory.read(base+0x18d3415,code,sizeof(compare))&&!memcmp(code,compare,sizeof(compare))&&
        memory.read(base+0x16996c7,code,sizeof(choose))&&!memcmp(code,choose,sizeof(choose))&&
        memory.read(base+0x1699857,code,sizeof(execute))&&!memcmp(code,execute,sizeof(execute));
}
static float syncNearestDistanceSquared(Vec3 origin,Vec3 site) {
    const auto delta=subtract(origin,site);
    // Keep the native single-precision subtraction and addition order.
    const float xy=delta.y*delta.y+delta.x*delta.x;return xy+delta.z*delta.z;
}
