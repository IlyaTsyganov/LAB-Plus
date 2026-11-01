#include "../src/vulkan_layer.cpp"
#include "../src/speedrun_techs.h"
static void expect(bool valid,const char* message) { if(!valid) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
template<class T> static void set(void* data,size_t offset,T value) { memcpy(static_cast<unsigned char*>(data)+offset,&value,sizeof(value)); }
// Exact bytes from independently audited retail1.0/1.1 EXEs. These are read-only
// fixture data, never executed, and include the real calculation/helper blocks.
static const unsigned char bats10calc[]{
    0xf3,0x44,0x0f,0x10,0x0d,0x14,0x6d,0xf6,0x00,0x48,0x8b,0x0d,0xf5,0xaa,0xb8,0x04,0x41,0x0f,0x28,0xf9,
    0x0f,0x29,0xb4,0x24,0xa0,0x00,0x00,0x00,0x45,0x0f,0x28,0xc1,0x4c,0x8b,0xa4,0x24,0xb8,0x00,0x00,0x00,
    0x48,0x8b,0xbc,0x24,0xc0,0x00,0x00,0x00,0x8b,0x51,0x08,0x85,0xd2,0x0f,0x84,0x83,0x00,0x00,0x00,0x83,
    0xfa,0x01,0x0f,0x85,0x2a,0x01,0x00,0x00,0x48,0x85,0xf6,0x0f,0x84,0x21,0x01,0x00,0x00,0x48,0x8b,0xce,
    0xe8,0x50,0xc7,0xe2,0xff,0x66,0x0f,0x6e,0x98,0x00,0x0b,0x00,0x00,0x0f,0x5b,0xdb,0xf3,0x0f,0x59,0x1d,
    0x59,0x0e,0xf7,0x00,0x41,0x0f,0x2f,0xda,0x76,0x30,0x66,0x0f,0x6e,0x7d,0xdb,0x66,0x0f,0x6e,0x45,0xdf,
    0x66,0x0f,0x6e,0x55,0xcb,0x66,0x0f,0x6e,0x4d,0xcf,0x0f,0x5b,0xff,0x0f,0x5b,0xc0,0x0f,0x5b,0xd2,0xf3,
    0x0f,0x5e,0xf8,0x0f,0x5b,0xc9,0xf3,0x0f,0x5e,0xd1,0xf3,0x0f,0x58,0xfa,0xf3,0x0f,0x5e,0xfb,0x48,0x63,
    0x86,0xd0,0x19,0x00,0x00,0x44,0x0f,0x28,0xc7,0x48,0x69,0xc8,0xd8,0x03,0x00,0x00,0xf3,0x0f,0x59,0xbc,
    0x31,0xac,0x34,0x00,0x00,0xe9,0xa6,0x00,0x00,0x00,0xf3,0x0f,0x10,0x35,0xfb,0x0d,0xf7,0x00,0x4d,0x85,
    0xff,0x74,0x4d,0x49,0x8b,0xcf,0xe8,0xd2,0xc6,0xe2,0xff,0x66,0x0f,0x6e,0x88,0x10,0x0b,0x00,0x00,0x0f,
    0x5b,0xc9,0xf3,0x0f,0x59,0xce,0x41,0x0f,0x2f,0xca,0x76,0x18,0x66,0x0f,0x6e,0x7d,0xcb,0x66,0x0f,0x6e,
    0x45,0xcf,0x0f,0x5b,0xff,0x0f,0x5b,0xc0,0xf3,0x0f,0x5e,0xf8,0xf3,0x0f,0x5e,0xf9,0x49,0x63,0x87,0xd0,
    0x19,0x00,0x00,0x48,0x69,0xc8,0xd8,0x03,0x00,0x00,0xf3,0x42,0x0f,0x59,0xbc,0x39,0xac,0x34,0x00,0x00,
    0x48,0x85,0xf6,0x74,0x51,0x48,0x8b,0xce,0xe8,0x80,0xc6,0xe2,0xff,0x66,0x0f,0x6e,0x88,0x00,0x0b,0x00,
    0x00,0x0f,0x5b,0xc9,0xf3,0x0f,0x59,0xce,0x41,0x0f,0x2f,0xca,0x76,0x1c,0x66,0x44,0x0f,0x6e,0x45,0xdb,
    0x66,0x0f,0x6e,0x45,0xdf,0x45,0x0f,0x5b,0xc0,0x0f,0x5b,0xc0,0xf3,0x44,0x0f,0x5e,0xc0,0xf3,0x44,0x0f,
    0x5e,0xc1,0x48,0x63,0x86,0xd0,0x19,0x00,0x00,0x48,0x69,0xc8,0xd8,0x03,0x00,0x00,0xf3,0x44,0x0f,0x59,
    0x84,0x31,0xa8,0x34,0x00,0x00,0x49,0x8b,0x9e,0x38,0x03,0x00,0x00,0x4c,0x8b,0xbc,0x24,0xb0,0x00,0x00,
    0x00,0x48,0x8b,0xb4,0x24,0xf8,0x00,0x00,0x00,0x48,0x85,0xdb,0x74,0x4f,0x48,0x8d,0x8b,0x98,0x56,0x02,
    0x00,0xe8,0x4f,0x11,0x2a,0x00,0x48,0x8b,0x03,0x48,0x8b,0xcb,0x0f,0x28,0xf0,0xff,0x90,0x30,0x05,0x00,
    0x00,0x48,0x8b,0xc8,0xe8,0x68,0xb2,0x1f,0x00,0x48,0x8b,0x83,0x60,0xd5,0x02,0x00,0xf3,0x0f,0x59,0xf0,
    0xf3,0x0f,0x10,0x88,0x0c,0x09,0x00,0x00,0xf3,0x0f,0x5f,0xce,0x41,0x0f,0x2f,0xca,0x76,0x0f,0xf3,0x44,
    0x0f,0x5e,0xc9,0xf3,0x45,0x0f,0x59,0xc1,0xf3,0x41,0x0f,0x59,0xf9,0x45,0x0f,0x2e,0x86,0xcc,0x18,0x00,
    0x00,0x48,0x8b,0x9c,0x24,0xf0,0x00,0x00,0x00,0x44,0x0f,0x28,0x4c,0x24,0x70,0x0f,0x28,0xb4,0x24,0xa0,
    0x00,0x00,0x00,0x7a,0x02,0x74,0x16,0x45,0x0f,0x2f,0xc2,0x76,0x10,0x49,0x8d,0x8e,0xa0,0x0f,0x00,0x00,
    0x41,0x0f,0x28,0xc8,0xe8,0x24,0xa7,0xf1,0xff,
};
static const unsigned char bats10dexterity[]{
    0x48,0x8b,0x81,0xe0,0x00,0x00,0x00,0x48,0x85,0xc0,0x74,0x17,0xf3,0x0f,0x10,0x80,0xf0,0x00,0x00,0x00,
    0x0f,0x28,0xd0,0xf3,0x0f,0x5f,0x15,0x89,0x6b,0xcd,0x00,0x0f,0x28,0xc2,0xc3,0xf3,0x0f,0x10,0x05,0x0d,
    0x5a,0xcc,0x00,0xc3,
};
static const unsigned char bats10declGetter[]{
    0x48,0x8b,0x41,0x38,0xc3,
};
static const unsigned char bats10status[]{
    0x48,0x89,0x6c,0x24,0x18,0x57,0x48,0x83,0xec,0x30,0x33,0xff,0x0f,0x29,0x74,0x24,0x20,0x48,0x8b,0xe9,
    0xf3,0x0f,0x10,0x35,0xec,0xb8,0xd6,0x00,0x39,0x79,0x28,0x7e,0x5b,0x48,0x89,0x5c,0x24,0x40,0x48,0x89,
    0x74,0x24,0x48,0x8b,0xf7,0x0f,0x1f,0x00,0x48,0x8b,0x45,0x20,0x48,0x8b,0x1c,0x06,0x48,0x85,0xdb,0x74,
    0x2a,0x48,0x8b,0x03,0x48,0x8b,0xcb,0xff,0x50,0x60,0x84,0xc0,0x75,0x1d,0x48,0x8b,0x03,0x48,0x8b,0xcb,
    0xff,0x50,0x68,0x84,0xc0,0x74,0x10,0x48,0x8b,0x03,0x48,0x8b,0xcb,0xff,0x90,0xd8,0x00,0x00,0x00,0xf3,
    0x0f,0x59,0xf0,0xff,0xc7,0x48,0x83,0xc6,0x08,0x3b,0x7d,0x28,0x7c,0xbe,0x48,0x8b,0x74,0x24,0x48,0x48,
    0x8b,0x5c,0x24,0x40,0x48,0x8b,0x6c,0x24,0x50,0x0f,0x28,0xc6,0x0f,0x28,0x74,0x24,0x20,0x48,0x83,0xc4,
    0x30,0x5f,0xc3,
};
static const unsigned char bats11calc[]{
    0xf3,0x44,0x0f,0x10,0x0d,0x14,0xe1,0xf7,0x00,0x48,0x8b,0x0d,0x05,0x15,0xb4,0x04,0x41,0x0f,0x28,0xf9,
    0x0f,0x29,0xb4,0x24,0xa0,0x00,0x00,0x00,0x45,0x0f,0x28,0xc1,0x4c,0x8b,0xa4,0x24,0xb8,0x00,0x00,0x00,
    0x48,0x8b,0xbc,0x24,0xc0,0x00,0x00,0x00,0x8b,0x51,0x08,0x85,0xd2,0x0f,0x84,0x83,0x00,0x00,0x00,0x83,
    0xfa,0x01,0x0f,0x85,0x2a,0x01,0x00,0x00,0x48,0x85,0xf6,0x0f,0x84,0x21,0x01,0x00,0x00,0x48,0x8b,0xce,
    0xe8,0xf0,0x91,0x62,0x00,0x66,0x0f,0x6e,0x98,0x00,0x0b,0x00,0x00,0x0f,0x5b,0xdb,0xf3,0x0f,0x59,0x1d,
    0x69,0x82,0xf8,0x00,0x41,0x0f,0x2f,0xda,0x76,0x30,0x66,0x0f,0x6e,0x7d,0xdb,0x66,0x0f,0x6e,0x45,0xdf,
    0x66,0x0f,0x6e,0x55,0xcb,0x66,0x0f,0x6e,0x4d,0xcf,0x0f,0x5b,0xff,0x0f,0x5b,0xc0,0x0f,0x5b,0xd2,0xf3,
    0x0f,0x5e,0xf8,0x0f,0x5b,0xc9,0xf3,0x0f,0x5e,0xd1,0xf3,0x0f,0x58,0xfa,0xf3,0x0f,0x5e,0xfb,0x48,0x63,
    0x86,0xd0,0x19,0x00,0x00,0x44,0x0f,0x28,0xc7,0x48,0x69,0xc8,0xd8,0x03,0x00,0x00,0xf3,0x0f,0x59,0xbc,
    0x31,0xac,0x34,0x00,0x00,0xe9,0xa6,0x00,0x00,0x00,0xf3,0x0f,0x10,0x35,0x0b,0x82,0xf8,0x00,0x4d,0x85,
    0xff,0x74,0x4d,0x49,0x8b,0xcf,0xe8,0x72,0x91,0x62,0x00,0x66,0x0f,0x6e,0x88,0x10,0x0b,0x00,0x00,0x0f,
    0x5b,0xc9,0xf3,0x0f,0x59,0xce,0x41,0x0f,0x2f,0xca,0x76,0x18,0x66,0x0f,0x6e,0x7d,0xcb,0x66,0x0f,0x6e,
    0x45,0xcf,0x0f,0x5b,0xff,0x0f,0x5b,0xc0,0xf3,0x0f,0x5e,0xf8,0xf3,0x0f,0x5e,0xf9,0x49,0x63,0x87,0xd0,
    0x19,0x00,0x00,0x48,0x69,0xc8,0xd8,0x03,0x00,0x00,0xf3,0x42,0x0f,0x59,0xbc,0x39,0xac,0x34,0x00,0x00,
    0x48,0x85,0xf6,0x74,0x51,0x48,0x8b,0xce,0xe8,0x20,0x91,0x62,0x00,0x66,0x0f,0x6e,0x88,0x00,0x0b,0x00,
    0x00,0x0f,0x5b,0xc9,0xf3,0x0f,0x59,0xce,0x41,0x0f,0x2f,0xca,0x76,0x1c,0x66,0x44,0x0f,0x6e,0x45,0xdb,
    0x66,0x0f,0x6e,0x45,0xdf,0x45,0x0f,0x5b,0xc0,0x0f,0x5b,0xc0,0xf3,0x44,0x0f,0x5e,0xc0,0xf3,0x44,0x0f,
    0x5e,0xc1,0x48,0x63,0x86,0xd0,0x19,0x00,0x00,0x48,0x69,0xc8,0xd8,0x03,0x00,0x00,0xf3,0x44,0x0f,0x59,
    0x84,0x31,0xa8,0x34,0x00,0x00,0x49,0x8b,0x9e,0x38,0x03,0x00,0x00,0x4c,0x8b,0xbc,0x24,0xb0,0x00,0x00,
    0x00,0x48,0x8b,0xb4,0x24,0xf8,0x00,0x00,0x00,0x48,0x85,0xdb,0x74,0x4f,0x48,0x8d,0x8b,0x38,0x54,0x02,
    0x00,0xe8,0x8f,0x75,0x2b,0x00,0x48,0x8b,0x03,0x48,0x8b,0xcb,0x0f,0x28,0xf0,0xff,0x90,0x30,0x05,0x00,
    0x00,0x48,0x8b,0xc8,0xe8,0x88,0xe1,0x20,0x00,0x48,0x8b,0x83,0x08,0xd3,0x02,0x00,0xf3,0x0f,0x59,0xf0,
    0xf3,0x0f,0x10,0x88,0x0c,0x09,0x00,0x00,0xf3,0x0f,0x5f,0xce,0x41,0x0f,0x2f,0xca,0x76,0x0f,0xf3,0x44,
    0x0f,0x5e,0xc9,0xf3,0x45,0x0f,0x59,0xc1,0xf3,0x41,0x0f,0x59,0xf9,0x45,0x0f,0x2e,0x86,0xd4,0x18,0x00,
    0x00,0x48,0x8b,0x9c,0x24,0xf0,0x00,0x00,0x00,0x44,0x0f,0x28,0x4c,0x24,0x70,0x0f,0x28,0xb4,0x24,0xa0,
    0x00,0x00,0x00,0x7a,0x02,0x74,0x16,0x45,0x0f,0x2f,0xc2,0x76,0x10,0x49,0x8d,0x8e,0xa8,0x0f,0x00,0x00,
    0x41,0x0f,0x28,0xc8,0xe8,0xb4,0x8d,0xf1,0xff,
};
static const unsigned char bats11dexterity[]{
    0x48,0x8b,0x81,0xe0,0x00,0x00,0x00,0x48,0x85,0xc0,0x74,0x17,0xf3,0x0f,0x10,0x80,0xf0,0x00,0x00,0x00,
    0x0f,0x28,0xd0,0xf3,0x0f,0x5f,0x15,0x59,0x7b,0xcd,0x00,0x0f,0x28,0xc2,0xc3,0xf3,0x0f,0x10,0x05,0xcd,
    0x69,0xcc,0x00,0xc3,
};
static const unsigned char bats11declGetter[]{
    0x48,0x8b,0x41,0x38,0xc3,
};
static const unsigned char bats11status[]{
    0x48,0x89,0x6c,0x24,0x18,0x57,0x48,0x83,0xec,0x30,0x33,0xff,0x0f,0x29,0x74,0x24,0x20,0x48,0x8b,0xe9,
    0xf3,0x0f,0x10,0x35,0xcc,0xfd,0xd6,0x00,0x39,0x79,0x28,0x7e,0x5b,0x48,0x89,0x5c,0x24,0x40,0x48,0x89,
    0x74,0x24,0x48,0x8b,0xf7,0x0f,0x1f,0x00,0x48,0x8b,0x45,0x20,0x48,0x8b,0x1c,0x06,0x48,0x85,0xdb,0x74,
    0x2a,0x48,0x8b,0x03,0x48,0x8b,0xcb,0xff,0x50,0x60,0x84,0xc0,0x75,0x1d,0x48,0x8b,0x03,0x48,0x8b,0xcb,
    0xff,0x50,0x68,0x84,0xc0,0x74,0x10,0x48,0x8b,0x03,0x48,0x8b,0xcb,0xff,0x90,0xe0,0x00,0x00,0x00,0xf3,
    0x0f,0x59,0xf0,0xff,0xc7,0x48,0x83,0xc6,0x08,0x3b,0x7d,0x28,0x7c,0xbe,0x48,0x8b,0x74,0x24,0x48,0x48,
    0x8b,0x5c,0x24,0x40,0x48,0x8b,0x6c,0x24,0x50,0x0f,0x28,0xc6,0x0f,0x28,0x74,0x24,0x20,0x48,0x83,0xc4,
    0x30,0x5f,0xc3,
};
int main() {
    expect(fastDestroyerBladeArmed(999960,1999920),"shorter next-charge override is armed");
    for(int64_t once:{-1LL,0LL,1999920LL,2999880LL}) expect(!fastDestroyerBladeArmed(once,1999920),"consumed, zero and ordinary/long overrides are not fast");
    expect(configuredBatsState(5,1,.6f,.3f)=="active","configured raise speed identifies the demonstrated fast state");
    expect(configuredBatsState(1.6666666269302368f,1,.6f,.3f)=="inactive","dexterity is included in normal raise speed");
    expect(configuredBatsState(1,1,1,.3f)=="inactive"&&configuredBatsState(2,2,1,.3f)=="inactive","unupgraded weapons and legitimate mod-rate scales remain normal");
    expect(configuredBatsState(.1f,1,1,.3f)=="unavailable"&&configuredBatsState(NAN,1,1,.3f)=="unavailable","unsettled and nonfinite animation rates remain unknown");
    const unsigned char flagCode[]{0x0f,0xb6,0x41,0x05,0xc1,0xe8,0x07,0xc3};
    auto bit=decodeTechBoolGetter(flagCode,sizeof(flagCode),7);expect(bit&&bit.byte==5&&bit.mask==128,"reflected mod-change getter identifies its actual packed bit");
    expect(!decodeTechBoolGetter(flagCode,sizeof(flagCode),5),"getter cannot read beyond the declared flags object");
    unsigned char unsupported[8]{};memcpy(unsupported,flagCode,8);unsupported[0]=0xe9;expect(!decodeTechBoolGetter(unsupported,8,7),"unknown or hooked getter code is never executed or guessed");
    FastBladeTracker tracker;FastBladeObservation sample;sample.identity={1,2,3,4,5};sample.module="weapon/player/gauss_rifle_ballista";sample.once=sample.afterHit=999960;sample.nominal=-1;sample.secondaryShot=10;sample.chargeState=2;sample.held=true;sample.flagKnown=true;
    expect(tracker.observe(sample).state=="inactive","ordinary mastered Arbalest recharge is not Fast DBlade despite its one-second override");
    sample.secondaryShot=11;sample.interruptedModChange=true;expect(tracker.observe(sample).state=="active","actual interrupted mod-change flag plus hit override activates immediately");
    sample.held=false;sample.interruptedModChange=false;sample.chargeState=0;expect(tracker.observe(sample).state=="active","prepared state persists when another weapon is held");
    sample.module="weapon/player/gauss_rifle_destroyer";sample.nominal=1999920;expect(tracker.observe(sample).state=="active","selecting Destroyer Blade is not the activation trigger");
    sample.once=-1;expect(tracker.observe(sample).state=="inactive","consuming the engine one-shot override clears Fast DBlade");
    sample.once=999960;sample.module="weapon/player/gauss_rifle_ballista";sample.nominal=-1;sample.held=true;sample.secondaryShot=12;sample.chargeState=2;sample.interruptedModChange=false;expect(tracker.observe(sample).state=="inactive","a complete ordinary mod switch followed by a mastery hit does not rearm the technique");
    sample.interruptedModChange=true;expect(tracker.observe(sample).state=="active","a new prepared hit can rearm the technique");
    sample.secondaryShot++;sample.interruptedModChange=false;expect(tracker.observe(sample).state=="inactive","a later ordinary shot cannot inherit an earlier hit's preparation proof");
    sample.interruptedModChange=true;expect(tracker.observe(sample).state=="active","the later shot can independently provide engine preparation proof");
    sample.interruptedModChange=false;sample.held=false;sample.identity.map++;expect(tracker.observe(sample).state=="unavailable","map changes discard prepared-hit provenance");
    sample.held=true;sample.interruptedModChange=true;tracker.observe(sample);sample.held=false;sample.interruptedModChange=false;sample.identity.token++;expect(tracker.observe(sample).state=="unavailable","reused inventory references cannot inherit an old armed state");
    sample.held=true;sample.interruptedModChange=true;tracker.observe(sample);sample.held=false;sample.interruptedModChange=false;sample.identity.player++;expect(tracker.observe(sample).state=="unavailable","a different player cannot inherit preparation");
    std::unordered_map<std::string,GraphType> schemas;
    const std::string managed="idManagedClassPtr < idInventoryItem >",list="idList < idManagedClassPtr < idInventoryItem > , TAG_IDLIST , false >",mod="idWeapon::weaponModOverrides_t",time="idTypesafeTime < long long , gameTimeUnique_t , 999960 >";
    schemas["idActor"]={65536,"",{{"inventory","idInventoryCollection","",20480,24},{"statusEffectComponent","idStatusEffectComponent","",16384,136}},true};
    schemas["idInventoryCollection"]={24,"",{{"inventory",list,"",0,24}},true};
    schemas[list]={24,"",{{"list",managed,"*",0,8},{"num","int","",8,4}},true};
    schemas[managed]={32,"",{{"spawnId","idSpawnId","",0,4},{"ptr","idInventoryItem","*",8,8}},true};
    schemas["idInventoryItem"]={272,"",{{"decl","idDeclInventory","*",56,8},{"quickSlot","int","",72,4}},true};
    schemas["idWeapon"]={15216,"idInventoryItem",{{"weaponModOverrides",mod,"[2]",13072,1968},{"fireMode","weaponFireMode_t","",6608,4},{"lastFireTime","idArray < idTypesafeTime < long long , gameTimeUnique_t , 999960 > , 2 >","",528,16},{"chargeParms","idWeapon::chargeParms_t","",600,128}},true};
    schemas[mod]={984,"",{{"weaponDecl","idDeclWeapon","*",8,8},{"chargeGameTimeOnce",time,"",376,8},{"chargeGameTime",time,"",192,8},{"bringUpRateScale","float","",408,4},{"chargeGameTimeAfterDirectHit",time,"",296,8}},true};
    schemas["idResource"]={8,"",{{"name","idAtomicString","",0,8}},true};
    schemas["idAtomicString"]={8,"",{{"str","char","*",0,8}},true};
    schemas["idPlayer"]={65536,"idActor",{{"hands","idHands","",1024,2048},{"playerProps","idDeclPlayerProps","*",32768,8},{"dashboard","idPlayerDashboard","",33792,512}},true};
    schemas["idHands"]={2048,"",{{"rightItem","idHandsItem","",64,128},{"webAnimator","idAnimWebHands","",256,64},{"nextWeaponRight","idWeapon","*",512,8},{"queuedNextWeaponRight","idWeapon","*",520,8},{"handsFlags","idHands::handsFlags_t","",600,7},{"meleeD5PrevWeapon","idWeapon","*",528,8}},true};
    schemas["idWeapon::chargeParms_t"]={128,"",{{"chargeState","idWeapon::chargeState_t","",0,4}},true};
    schemas["idHands::handsFlags_t"]={7,"",{},true};
    schemas["idHandsItem"]={128,"",{{"item",managed,"",48,32}},true};
    schemas["idAnimWebHands"]={64,"",{{"bringUpAnimRate","float","",0,4},{"bringDownAnimRate","float","",4,4}},true};
    schemas["idStatusEffectComponent"]={136,"",{{"activeStatusEffects","idStaticList < idStatusEffect * , 10 , false , TAG_IDLIST >","",32,104}},true};
    schemas["idList < idStatusEffect * , TAG_IDLIST , false >"]={24,"",{{"num","int","",8,4}},true};
    schemas["idDeclWeapon"]={8504,"",{{"desiredBringupDurationMS","milliToGameTime_t","",2808,16}},true};
    schemas["milliToGameTime_t"]={16,"",{{"gameTime",time,"",0,8},{"milliseconds","idTypesafeTime < int , millisecondUnique_t , 1000 >","",8,4}},true};
    schemas["idDeclPlayerProps"]={64,"",{{"playerSpeedClamping","idDeclPlayerProps::playerSpeedClamping_t","",32,16}},true};
    schemas["idDeclPlayerProps::playerSpeedClamping_t"]={16,"",{{"weaponSwitchClampScale","float","",4,4}},true};
    schemas["idPlayerDashboard"]={512,"",{{"abilityUpgrades","idAbility_UpgradeHandler","",0,256}},true};
    schemas["idAbility_UpgradeHandler"]={256,"",{{"dexterityWeaponSwitching","idUpgradeMod_Abilities","*",224,8}},true};
    schemas["idUpgradeMod_Abilities"]={512,"",{{"dexterityMods","idUpgradeMod_Abilities::dexterityMods_t","",232,16}},true};
    schemas["idUpgradeMod_Abilities::dexterityMods_t"]={16,"",{{"weaponChangeSpeed","float","",8,4}},true};
    auto lookup=[&](const std::string& type)->const GraphType& { auto found=schemas.find(type); static const GraphType empty; return found==schemas.end()?empty:found->second; };
    const auto profile=kProfile10;
    auto* base=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x7000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));expect(base!=nullptr,"fixture image allocation");
    uintptr_t image=reinterpret_cast<uintptr_t>(base);
    std::vector<unsigned char> system(96),map(profile.mapEntitiesOffset+16),player(65536),weapon(15216),definition(8504),module(16),inventory(32),props(64),upgrade(512),cvar(16),punch(4096),punchDefinition(64),foreignWeapon(4096);
    set(base,profile.gameSystemRva,reinterpret_cast<uintptr_t>(system.data()));set(system.data(),0x50,reinterpret_cast<uintptr_t>(map.data()));set(map.data(),profile.mapPlayerOffset,reinterpret_cast<uintptr_t>(player.data()));
    set(player.data(),0,image+profile.playerVtableRva);set(player.data(),20480,reinterpret_cast<uintptr_t>(inventory.data()));set(player.data(),20488,int32_t(1));
    set(inventory.data(),0,uint32_t(123));set(inventory.data(),8,reinterpret_cast<uintptr_t>(weapon.data()));
    const char* weaponName="weapon/player/gauss_rifle",*moduleName="weapon/player/gauss_rifle_destroyer",*typeName="idWeapon";
    set(definition.data(),0,reinterpret_cast<uintptr_t>(weaponName));set(module.data(),0,reinterpret_cast<uintptr_t>(moduleName));set(weapon.data(),56,reinterpret_cast<uintptr_t>(definition.data()));
    const unsigned char getter[]{0x48,0x8d,0x05,0,0,0,0,0xc3};memcpy(base+0x210000,getter,8);set(base,0x210003,int32_t(0x220000-0x210000-7));set(base,0x200010,image+0x210000);set(base,0x220000,reinterpret_cast<uintptr_t>(typeName));set(weapon.data(),0,image+0x200000);
    set(player.data(),32768,reinterpret_cast<uintptr_t>(props.data()));set(props.data(),36,.3f);set(player.data(),33792+224,reinterpret_cast<uintptr_t>(upgrade.data()));set(upgrade.data(),240,.6f);
    memcpy(base+0x161dbbb,bats10calc,sizeof(bats10calc));memcpy(base+0x18beea0,bats10dexterity,sizeof(bats10dexterity));memcpy(base+0x144a360,bats10declGetter,sizeof(bats10declGetter));memcpy(base+0x1818fd0,bats10status,sizeof(bats10status));
    set(base,0x25848d8,1.f);set(base,0x258ea7c,.001f);set(base,0x2595a48,std::numeric_limits<float>::epsilon());
    set(base,0x61a86c0,reinterpret_cast<uintptr_t>(cvar.data()));set(weapon.data(),13072+408,1.f);set(weapon.data(),13072+984+408,1.f);
    memcpy(base+0x230000,flagCode,sizeof(flagCode));schemas["idHands::handsFlags_t"].properties.push_back({"modChangeAnimPlaying","bool","",image+0x230000});
    const unsigned char meleeCode[]{0x0f,0xb6,0x41,0x02,0xc1,0xe8,0x04,0x83,0xe0,0x01,0xc3};
    memcpy(base+0x240000,meleeCode,sizeof(meleeCode));schemas["idHands::handsFlags_t"].properties.push_back({"isMeleeAnimPlaying","bool","",image+0x240000});
    const char* punchName="weapon/player/fists_doom5melee";set(punchDefinition.data(),0,reinterpret_cast<uintptr_t>(punchName));set(punch.data(),56,reinterpret_cast<uintptr_t>(punchDefinition.data()));set(punch.data(),0,image+0x200000);set(punch.data(),72,int32_t(-1));
    const size_t secondary=13072+984;set(weapon.data(),secondary+8,reinterpret_cast<uintptr_t>(module.data()));set(weapon.data(),secondary+192,int64_t(1999920));set(weapon.data(),secondary+376,int64_t(999960));set(weapon.data(),secondary+296,int64_t(999960));set(weapon.data(),536,int64_t(10));
    set(player.data(),1024+64+48+8,reinterpret_cast<uintptr_t>(weapon.data()));set(player.data(),1024+256,5.f);set(player.data(),1024+260,1.6666666269302368f);
    LevelIdentity level{reinterpret_cast<uintptr_t>(map.data()),1,"fixture"};
    auto capture=[&]() { return captureSpeedrunTechs(WallMemory{},image,profile,level,lookup); };
    auto state=capture();expect(state.rows.size()==2&&state.rows[0].state=="active"&&state.rows[1].state=="active","production reader sees confirmed BATS and next-charge override from exact fields");
    base[0x161dbbb+0x34]^=1;state=capture();expect(state.rows[0].state=="unavailable","a changed normal-rate algorithm cannot inherit an audited profile");base[0x161dbbb+0x34]^=1;
    set(player.data(),1024+64+48+8,reinterpret_cast<uintptr_t>(punch.data()));set(player.data(),1024+528,reinterpret_cast<uintptr_t>(weapon.data()));player[1024+600+2]=16;
    set(player.data(),1024+512,reinterpret_cast<uintptr_t>(weapon.data()));
    state=capture();expect(state.rows[0].state=="active","ordinary punch retains live BATS by using the engine's preceding inventory weapon");
    state=capture();expect(state.rows[0].state=="active","repeated melee observations keep evaluating the surviving fast rate");
    set(player.data(),1024+64+48+8,reinterpret_cast<uintptr_t>(weapon.data()));set(player.data(),1024+512,reinterpret_cast<uintptr_t>(punch.data()));
    state=capture();expect(state.rows[0].state=="active","the melee handoff toward the temporary punch item uses the same real gun configuration");
    set(player.data(),1024+64+48+8,reinterpret_cast<uintptr_t>(punch.data()));set(player.data(),1024+512,reinterpret_cast<uintptr_t>(weapon.data()));
    set(player.data(),1024+256,1.6666666269302368f);state=capture();expect(state.rows[0].state=="inactive","a real rate reset during melee clears BATS immediately instead of retaining an active label");
    set(player.data(),1024+256,5.f);set(player.data(),1024+520,reinterpret_cast<uintptr_t>(foreignWeapon.data()));state=capture();expect(state.rows[0].state=="unavailable","a queued genuine weapon switch cannot borrow melee continuity");set(player.data(),1024+520,uintptr_t(0));
    set(player.data(),1024+528,reinterpret_cast<uintptr_t>(foreignWeapon.data()));state=capture();expect(state.rows[0].state=="unavailable","stale or foreign previous-weapon pointers cannot inherit BATS");set(player.data(),1024+528,reinterpret_cast<uintptr_t>(weapon.data()));
    player[1024+600+2]=0;state=capture();expect(state.rows[0].state=="unavailable","a punch item without the engine's actual melee state never inherits the preceding gun's status");player[1024+600+2]=16;
    const char* unsupportedPunch="weapon/player/fists_berserk";set(punchDefinition.data(),0,reinterpret_cast<uintptr_t>(unsupportedPunch));state=capture();expect(state.rows[0].state=="unavailable","unverified melee and powerup weapons cannot use the ordinary punch exception");set(punchDefinition.data(),0,reinterpret_cast<uintptr_t>(punchName));
    base[0x240000]=0xe9;state=capture();expect(state.rows[0].state=="unavailable","unrecognized melee getter instructions cannot preserve BATS");memcpy(base+0x240000,meleeCode,sizeof(meleeCode));
    schemas["idHands"].fields[5].size=4;state=capture();expect(state.rows[0].state=="unavailable","a malformed preceding-weapon pointer cannot escape reflected-field bounds");schemas["idHands"].fields[5].size=8;
    set(player.data(),1024+64+48+8,reinterpret_cast<uintptr_t>(weapon.data()));player[1024+600+2]=0;set(player.data(),1024+512,uintptr_t(0));set(player.data(),1024+256,1.6666666269302368f);
    state=capture();expect(state.rows[0].state=="inactive","returning to the ordinary gun after an actual reset cannot leave BATS stuck active");
    set(player.data(),1024+64+48+8,reinterpret_cast<uintptr_t>(punch.data()));player[1024+600+2]=16;state=capture();expect(state.rows[0].state=="inactive","an ordinary punch after reset remains inactive despite previously observed BATS");
    set(player.data(),1024+64+48+8,reinterpret_cast<uintptr_t>(weapon.data()));player[1024+600+2]=0;set(player.data(),1024+256,5.f);
    set(weapon.data(),secondary+376,int64_t(-1));state=capture();expect(state.rows[1].state=="inactive","first charge consumes Fast DBlade");
    const char* arbalestName="weapon/player/gauss_rifle_ballista";set(module.data(),0,reinterpret_cast<uintptr_t>(arbalestName));set(weapon.data(),secondary+192,int64_t(-1));set(weapon.data(),secondary+376,int64_t(999960));set(weapon.data(),600,int32_t(2));set(weapon.data(),536,int64_t(20));
    state=capture();expect(state.rows[1].state=="inactive","production reader rejects ordinary Arbalest mastery recharge");
    player[1024+600+5]=128;state=capture();expect(state.rows[1].state=="active","production reader activates at the prepared Arbalest hit while Arbalest remains selected");
    player[1024+600+5]=0;set(player.data(),1024+64+48+8,uintptr_t(0));state=capture();expect(state.rows[1].state=="active","production reader retains engine-proven preparation when Ballista is holstered");
    set(weapon.data(),secondary+376,int64_t(-1));state=capture();expect(state.rows[1].state=="inactive","production reader observes consumption independently of the held weapon");set(player.data(),1024+64+48+8,reinterpret_cast<uintptr_t>(weapon.data()));
    set(module.data(),0,reinterpret_cast<uintptr_t>(moduleName));set(weapon.data(),secondary+192,int64_t(1999920));set(weapon.data(),600,int32_t(0));
    set(player.data(),1024+256,1.6666666269302368f);set(player.data(),1024+260,2.222222328186035f);state=capture();expect(state.rows[0].state=="inactive","plain switching resets BATS");
    set(player.data(),16384+32+8,int32_t(1));state=capture();expect(state.rows[0].state=="unavailable","temporary status effects are outside initial BATS calibration");set(player.data(),16384+32+8,int32_t(0));
    set(player.data(),1024+520,reinterpret_cast<uintptr_t>(weapon.data()));state=capture();expect(state.rows[0].state=="unavailable","queued weapon changes are not stable BATS observations");set(player.data(),1024+520,uintptr_t(0));
    const char* shotgunName="weapon/player/shotgun";set(definition.data(),0,reinterpret_cast<uintptr_t>(shotgunName));state=capture();expect(state.rows[0].state=="inactive","BATS normal-rate comparison also works without Ballista in inventory");
    set(player.data(),1024+256,5.f);state=capture();expect(state.rows[0].state=="active","BATS detection uses current weapon configuration instead of a Ballista name gate");
    set(player.data(),33792+224,uintptr_t(0));set(player.data(),1024+256,1.f);state=capture();expect(state.rows[0].state=="inactive","fresh campaign without dexterity has its own correct normal rate");set(player.data(),33792+224,reinterpret_cast<uintptr_t>(upgrade.data()));
    set(definition.data(),2816,int32_t(400));state=capture();expect(state.rows[0].state=="unavailable","nonzero desired clip duration is explicit until its animation length is known");set(definition.data(),2816,int32_t(0));
    set(definition.data(),0,reinterpret_cast<uintptr_t>(weaponName));set(player.data(),1024+256,1.6666666269302368f);
    auto otherProfile=profile;otherProfile.timestamp++;state=captureSpeedrunTechs(WallMemory{},image,otherProfile,level,lookup);expect(state.rows[0].state=="unavailable","unverified builds never inherit demonstrated BATS constants");
    set(definition.data(),0,uintptr_t(0));state=capture();expect(state.rows[1].state=="unavailable","unreadable resource names cannot prove Ballista absent");set(definition.data(),0,reinterpret_cast<uintptr_t>(weaponName));
    set(weapon.data(),0,uintptr_t(0));state=capture();expect(state.rows[1].state=="unavailable","unreadable inventory classes cannot prove Ballista absent");set(weapon.data(),0,image+0x200000);
    schemas["idPlayer"].fields[0].size=16;state=capture();expect(state.rows[0].state=="unavailable","undersized inline hands cannot borrow another type's offsets");schemas["idPlayer"].fields[0].size=2048;
    schemas["idHands"].fields[0].size=16;state=capture();expect(state.rows[0].state=="unavailable","undersized inline hand item cannot expose an external weapon pointer");schemas["idHands"].fields[0].size=128;
    schemas["idActor"].fields[1].size=64;state=capture();expect(state.rows[0].state=="unavailable","undersized status component cannot borrow list offsets");schemas["idActor"].fields[1].size=136;
    memcpy(base+0x1659efb,bats11calc,sizeof(bats11calc));memcpy(base+0x1911620,bats11dexterity,sizeof(bats11dexterity));memcpy(base+0x1c83140,bats11declGetter,sizeof(bats11declGetter));memcpy(base+0x1868230,bats11status,sizeof(bats11status));set(base,0x619b410,reinterpret_cast<uintptr_t>(cvar.data()));
    set(base,0x25d8018,1.f);set(base,0x25e21cc,.001f);set(base,0x25e9198,std::numeric_limits<float>::epsilon());
    set(base,kProfile11.gameSystemRva,reinterpret_cast<uintptr_t>(system.data()));set(player.data(),0,image+kProfile11.playerVtableRva);
    auto capture11=[&]() { return captureSpeedrunTechs(WallMemory{},image,kProfile11,level,lookup); };
    state=capture11();expect(state.rows[0].state=="inactive","the actual1.1 code fingerprint accepts the same verified normal-rate calculation");
    set(player.data(),1024+256,5.f);state=capture11();expect(state.rows[0].state=="active","1.1 supports fast raise-rate observations rather than remaining build-blocked");
    set(player.data(),1024+64+48+8,reinterpret_cast<uintptr_t>(punch.data()));player[1024+600+2]=16;state=capture11();expect(state.rows[0].state=="active","1.1 ordinary punch uses its decoded melee flag and preceding weapon");
    set(player.data(),1024+256,1.6666666269302368f);state=capture11();expect(state.rows[0].state=="inactive","1.1 melee cannot keep BATS after the actual rate resets");
    set(player.data(),1024+64+48+8,reinterpret_cast<uintptr_t>(weapon.data()));player[1024+600+2]=0;
    base[0x1911620+12]^=1;state=capture11();expect(state.rows[0].state=="unavailable","a modified1.1 dexterity getter is rejected even when rate metadata still matches");base[0x1911620+12]^=1;
    set(base,0x25d8018,2.f);state=capture11();expect(state.rows[0].state=="unavailable","a changed normal-rate constant cannot pass unchanged code fingerprints");set(base,0x25d8018,1.f);
    set(module.data(),0,reinterpret_cast<uintptr_t>(arbalestName));set(weapon.data(),secondary+192,int64_t(-1));set(weapon.data(),secondary+376,int64_t(999960));set(weapon.data(),600,int32_t(2));set(weapon.data(),536,int64_t(30));player[1024+600+5]=128;
    state=capture11();expect(state.rows[1].state=="active","1.1 reflected Fast DBlade fields and unchanged bool leaf support prepared-hit activation");
    player[1024+600+5]=0;set(weapon.data(),536,int64_t(31));state=capture11();expect(state.rows[1].state=="inactive","ordinary1.1 Arbalest mastery remains a negative Fast DBlade control");
    set(weapon.data(),secondary+376,int64_t(-1));state=capture11();expect(state.rows[1].state=="inactive","1.1 consumed one-shot override remains inactive");
    schemas[mod].fields[2].type="int";state=capture11();expect(state.rows[1].state=="unavailable","mismatched time types cannot produce Fast DBlade status");
    VirtualFree(base,0,MEM_RELEASE);
    puts("PASS: actual engine-field tech reader, configured BATS for multiple weapons and upgrade states, consumed next-charge override, prepared-hit timing/provenance, holstering/consumption and metadata/build/identity guards.");
}
