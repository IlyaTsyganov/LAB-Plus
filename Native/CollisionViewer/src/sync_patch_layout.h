#pragma once

// Version-specific immutable field and code descriptors. A scope is local to
// the observing thread; nested fixture/probe reads restore the prior version.
struct SyncPatchMember {const char* owner;uint32_t oldOffset,oldSize,offset,size;};
struct SyncPatchCode {uintptr_t rva;uint32_t size;uint64_t hash;};
struct SyncPatchLayout {
    bool modern=false;
    uintptr_t game=0x6128718,viewManager=0x136140,sphereVtable=0x2f265e8;
    uintptr_t groupCode=0x5d721d,monsterGetter=0xc293f0,monsterSlot=0x1098,monsterField=0x56f0,monsterValue=0x1f0;
    uintptr_t bindSlot=0x620,bindField=0x1e20,physicsSlot=0xe28,aiAxisGetter=0xd41ed0,aiAxisSlot=0x70;
    uintptr_t useHistory=0x61a9db0,semiRandom=0x61a9cb0,historyCount=0x61a9d30,maxDistance=0x61c9630,ignoreChainsawLunge=0x61a99b0;
    uintptr_t emptyEquipment=0;
    const SyncPatchMember* members=nullptr;size_t count=0;
    const SyncPatchCode* code=nullptr;size_t codeCount=0;
};
static const SyncPatchLayout kSyncPatch11{};
static const SyncPatchMember kSyncMembers51[]{
    {"idDeclAnimWeb",0x78,24,0x80,24},
    {"idDeclAnimWeb",0x90,24,0x98,24},
    {"idDeclAnimWeb",0xc0,24,0xc8,24},
    {"idAI2",0x4bb0,0x858,0x4b80,0x8a8},
    {"idAI2",0x58a0,0x8,0x58f8,0x8},
    {"idAI2",0x63a0,0x118,0x64f0,0x140},
    {"idAI2::idAIConstant",0x200,0x628,0x228,0x650},
    {"idActor",0x1c28,0x1c8,0x1c58,0x1c8},
    {"idActor",0x1e20,0x4c0,0x1e50,0x4c8},
    {"idAnimatedEntity",0xbd0,0x8,0xc08,0x8},
    {"idAnimatedEntity",0x1a10,0x70,0x1a40,0x70},
    {"idDeclAIBehavior",0x1f0,0x8,0x1f8,0x8},
    {"idDeclSyncInteraction",0x144,0x4,0x158,0x4},
    {"idDeclSyncInteraction",0x148,0x4,0x15c,0x4},
    {"idDeclSyncInteraction",0x1b8,0x20,0x1d0,0x20},
    {"idDeclSyncInteraction",0x1d8,0xb8,0x1f0,0xb8},
    {"idDeclSyncInteraction",0x2a0,0x78,0x2b8,0x78},
    {"idDeclSyncInteraction",0x318,0x70,0x330,0x70},
    {"idDeclSyncInteraction::playerRequirements_t",0x76,0x1,0x77,0x1},
    {"idDeclTargeting",0xc4,0x4,0xcc,0x4},
    {"idDeclWeapon",0x748,0x8,0x7d0,0x8},
    {"idDeclWeapon",0x1f40,0x8,0x2110,0x8},
    {"idHands",0x2988,0xd78,0x29b0,0xd78},
    {"idHealthT < aiHealthComponent_t , AI_HEALTH_MAX , AI_HEALTH_HITPOINTS >",0xe8,0x1,0x110,0x1},
    {"idJointAnimator",0x0,0x8,0x8,0x8},
    {"idPlayer",0x70e8,0x628,0x7d78,0x650},
    {"idPlayer",0x7768,0x18,0x8420,0x18},
    {"idPlayer",0x7d28,0x4870,0x89e8,0x4870},
    {"idPlayer",0xc5a0,0x9160,0xd260,0x9248},
    {"idPlayer",0x159a0,0x2f0,0x16748,0x2f0},
    {"idPlayer",0x377a4,0x24,0x488a4,0x24},
    {"idPlayer",0x377c8,0xc,0x488c8,0xc},
    {"idPlayer",0x3b000,0x4,0x4c380,0x4},
    {"idPlayer",0x3b078,0x8,0x4c3f8,0x8},
    {"idRenderView",0x213f0,0x40,0x21430,0x40},
    {"idSyncAttack",0x1a8,0x8,0x1c8,0x8},
    {"idSyncAttack",0x1b0,0x18,0x1d0,0x18},
    {"idSyncAttack",0x210,0x8,0x230,0x8},
    {"idSyncAttack",0x608,0x18,0x630,0x18},
    {"idSyncEntity",0x1bf8,0x8,0x1c28,0x8},
    {"idSyncEntity",0x1c18,0x18,0x1c48,0x18},
    {"idSyncEntity",0x1caa,0x1,0x1cda,0x1},
    {"idSyncEntity",0x2bc8,0xc,0x2c08,0xc},
    {"idSyncEntity",0x2c08,0x18,0x2c48,0x18},
    {"idSyncParticipant",0xaa,0x2,0xac,0x2},
    {"idWeapon",0x19d0,0x4,0x19e8,0x4},
    {"idWeapon",0x3310,0x7b0,0x3328,0x7b0},
};
static const SyncPatchCode kSyncCode51[]{
    {0x1a8a43d,13,0xf110f5d9af0d2826ull},
    {0x1fb95f0,11,0xa66ce55f5a9dee1aull},
    {0x1a8ac86,10,0x296d7be5217bd606ull},
    {0x60a6fd,5,0xafeea4a12cebc437ull},
    {0x1a82200,16,0x3a0a6c5e200bddf9ull},
    {0x1a824cc,31,0xbf722dfb01ff79b1ull},
    {0x18557db,5,0x4d5580d2b0a61e91ull},
    {0x18559cb,5,0xfee43b59e9a1263ull},
};
static const SyncPatchLayout kSyncPatch51=[](){auto p=kSyncPatch11;p.modern=true;
    p.emptyEquipment=0x27e0ce0;p.game=0x658a5f0;p.viewManager=0x133210;p.sphereVtable=0x31affb8;p.groupCode=0x60a6fd;
    p.monsterGetter=0xcf94a0;p.monsterSlot=0x10f0;p.monsterField=0x5718;p.monsterValue=0x1f8;
    p.bindSlot=0x650;p.bindField=0x1e50;p.physicsSlot=0xe70;p.aiAxisGetter=0xe275a0;p.aiAxisSlot=0x80;
    p.useHistory=0x6617170;p.semiRandom=0x6617070;p.historyCount=0x66170f0;p.maxDistance=0x6638c90;p.ignoreChainsawLunge=0x6616b70;
    p.members=kSyncMembers51;p.count=sizeof(kSyncMembers51)/sizeof(*kSyncMembers51);p.code=kSyncCode51;p.codeCount=sizeof(kSyncCode51)/sizeof(*kSyncCode51);return p;}();
#include "sync_patch_ports.h"

static const SyncPatchLayout* syncPatchFor(const GameProfile& game) {
    if(game.timestamp==0x5ec7deca&&game.imageSize==0x1d551000)return &kSyncPatch11;
    if(game.timestamp==0x607e69b0&&game.imageSize==0x1b5dd000)return &kSyncPatch51;
    for(const auto& port:kSyncPorts)if(game.timestamp==port.timestamp&&game.imageSize==port.imageSize)return port.layout;
    return nullptr;
}
static thread_local const SyncPatchLayout* syncPatchCurrent=&kSyncPatch11;
static const SyncPatchLayout& syncPatch(){return *syncPatchCurrent;}
struct SyncPatchScope {
    const SyncPatchLayout* previous=syncPatchCurrent;
    explicit SyncPatchScope(const GameProfile& game){const auto* p=syncPatchFor(game);if(p)syncPatchCurrent=p;}
    ~SyncPatchScope(){syncPatchCurrent=previous;}
    SyncPatchScope(const SyncPatchScope&)=delete;SyncPatchScope& operator=(const SyncPatchScope&)=delete;
};
static uint32_t syncPatchOffset(const char* owner,uint32_t offset) {
    const auto& p=syncPatch();const SyncPatchMember* best=nullptr;
    for(size_t i=0;i<p.count;++i){const auto& f=p.members[i];if(!strcmp(owner,f.owner)&&offset>=f.oldOffset&&offset-f.oldOffset<f.oldSize&&(!best||f.oldSize<best->oldSize))best=&f;}
    return best?best->offset+(offset-best->oldOffset):offset;
}
static uint32_t syncPatchSize(const char* owner,uint32_t offset,uint32_t size) {
    const auto& p=syncPatch();for(size_t i=0;i<p.count;++i){const auto& f=p.members[i];if(!strcmp(owner,f.owner)&&offset==f.oldOffset&&size==f.oldSize)return f.size;}return size;
}
static uintptr_t syncPlayerOffset(uint32_t offset){return syncPatchOffset("idPlayer",offset);}
static uintptr_t syncEntityOffset(uint32_t offset) {
    if(offset==0x6488)return syncPatchOffset("idAI2",0x63a0)+syncPatchOffset("idHealthT < aiHealthComponent_t , AI_HEALTH_MAX , AI_HEALTH_HITPOINTS >",0xe8);
    if(offset>=0x1a10&&offset<0x1a80)return syncPatchOffset("idAnimatedEntity",0x1a10)+offset-0x1a10;
    if(offset==0xbd0)return syncPatchOffset("idAnimatedEntity",offset);
    if(offset==0x58a0)return syncPatchOffset("idAI2",offset);
    return offset;
}
static bool syncPatchCodeMatches(const WallMemory& memory,uintptr_t base) {
    const auto& p=syncPatch();if(!p.modern)return true;unsigned char bytes[64]{};
    for(size_t i=0;i<p.codeCount;++i){const auto& check=p.code[i];if(check.size>sizeof(bytes)||!memory.read(base+check.rva,bytes,check.size))return false;
        uint64_t hash=1469598103934665603ull;for(uint32_t k=0;k<check.size;++k){hash^=bytes[k];hash*=1099511628211ull;}if(hash!=check.hash)return false;}
    return p.codeCount>0;
}
