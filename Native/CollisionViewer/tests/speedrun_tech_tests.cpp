#include "../src/vulkan_layer.cpp"
#include "../src/speedrun_techs.h"
static void expect(bool valid,const char* message) { if(!valid) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
template<class T> static void set(void* data,size_t offset,T value) { memcpy(static_cast<unsigned char*>(data)+offset,&value,sizeof(value)); }

// Public fixtures are synthetic. Retail EXE audit data stays local in private/.
int main() {
    expect(fastDestroyerBladeArmed(999960,1999920),"shorter next-charge override is armed");
    for(int64_t once:{-1LL,0LL,1999920LL,2999880LL}) expect(!fastDestroyerBladeArmed(once,1999920),"consumed, zero and ordinary/long overrides are not fast");
    expect(configuredBatsState(5,1,.6f,.3f)=="active","faster configured bring-up is active");
    expect(configuredBatsState(1.6666666269302368f,1,.6f,.3f)=="inactive","Dexterity is part of the normal rate");
    expect(configuredBatsState(1,1,1,.3f)=="inactive"&&configuredBatsState(2,2,1,.3f)=="inactive","unupgraded and mode-scaled normal rates stay inactive");
    expect(configuredBatsState(.1f,1,1,.3f)=="unavailable"&&configuredBatsState(NAN,1,1,.3f)=="unavailable","invalid samples remain internally unverified");
    const unsigned char flagCode[]{0x0f,0xb6,0x41,0x05,0xc1,0xe8,0x07,0xc3};
    auto bit=decodeTechBoolGetter(flagCode,sizeof(flagCode),7);
    expect(bit&&bit.byte==5&&bit.mask==128,"synthetic packed bool getter decodes its actual bit");
    expect(!decodeTechBoolGetter(flagCode,sizeof(flagCode),5),"getter must fit the declared object");
    unsigned char unsupported[8]{};memcpy(unsupported,flagCode,8);unsupported[0]=0xe9;
    expect(!decodeTechBoolGetter(unsupported,8,7),"hooked/unknown instructions are never executed or guessed");

    FastBladeTracker tracker;FastBladeObservation sample;
    sample.identity={1,2,3,4,5};sample.module="weapon/player/gauss_rifle_ballista";
    sample.once=sample.afterHit=999960;sample.nominal=-1;sample.secondaryShot=10;
    sample.chargeState=2;sample.held=true;sample.flagKnown=true;
    expect(tracker.observe(sample).state=="inactive","ordinary mastered Arbalest recharge is a negative control");
    sample.secondaryShot=11;sample.interruptedModChange=true;
    expect(tracker.observe(sample).state=="active","prepared hit with interrupted mod change activates immediately");
    sample.held=false;sample.interruptedModChange=false;sample.chargeState=0;
    expect(tracker.observe(sample).state=="active","preparation survives holstering");
    sample.module="weapon/player/gauss_rifle_destroyer";sample.nominal=1999920;
    expect(tracker.observe(sample).state=="active","selecting Destroyer Blade is not the activation trigger");
    sample.once=-1;expect(tracker.observe(sample).state=="inactive","consumption clears preparation");
    sample.once=999960;sample.module="weapon/player/gauss_rifle_ballista";sample.nominal=-1;
    sample.held=true;sample.secondaryShot=12;sample.chargeState=2;sample.interruptedModChange=false;
    expect(tracker.observe(sample).state=="inactive","ordinary mastery hit does not rearm");
    sample.interruptedModChange=true;expect(tracker.observe(sample).state=="active","new interrupted hit can rearm");
    sample.identity.token++;sample.interruptedModChange=false;
    expect(tracker.observe(sample).state=="inactive","generation reuse cannot inherit preparation");

    constexpr size_t bytes=0x8000;
    auto* base=static_cast<unsigned char*>(VirtualAlloc(nullptr,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    expect(base!=nullptr,"synthetic PE allocation");
    const uintptr_t image=reinterpret_cast<uintptr_t>(base);
    auto profile=kProfile66632;profile.imageSize=bytes;
    ConfiguredBatsCode decoded{};
    expect(!decodeBatsSignature(WallMemory{},image,profile,bytes-1,decoded),"calculation must fit the image");
    expect(!decodeBatsSignature(WallMemory{},image,profile,0x1000,decoded),"arbitrary executable bytes cannot prove BATS");
    expect(!findBatsSignature(WallMemory{},image,profile,decoded),"missing PE headers are rejected");
    IMAGE_DOS_HEADER dos{};dos.e_magic=IMAGE_DOS_SIGNATURE;dos.e_lfanew=0x100;set(base,0,dos);
    IMAGE_NT_HEADERS64 nt{};nt.Signature=IMAGE_NT_SIGNATURE;nt.FileHeader.NumberOfSections=1;
    nt.FileHeader.TimeDateStamp=profile.timestamp;nt.FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER64);
    nt.OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;nt.OptionalHeader.SizeOfImage=bytes;set(base,0x100,nt);
    IMAGE_SECTION_HEADER section{};section.VirtualAddress=0x1000;section.Misc.VirtualSize=0x2000;section.Characteristics=IMAGE_SCN_MEM_EXECUTE;
    set(base,0x100+sizeof(nt),section);
    expect(!findBatsSignature(WallMemory{},image,profile,decoded),"valid PE with no audited match stays unsupported");
    section.VirtualAddress=bytes-16;set(base,0x100+sizeof(nt),section);
    expect(!findBatsSignature(WallMemory{},image,profile,decoded),"out-of-image executable sections are rejected");
    auto lookup=[](const std::string&)->const GraphType& { static const GraphType empty;return empty; };
    auto capture=captureSpeedrunTechs(WallMemory{},image,profile,LevelIdentity{},lookup);
    expect(!capture.rows.empty()&&capture.rows[0].key=="bats"&&capture.rows[0].state=="inactive"&&!capture.rows[0].verified,"unverified production BATS remains binary inactive");
    VirtualFree(base,0,MEM_RELEASE);
    puts("PASS: synthetic BATS rate and signature guards, packed flags, Fast DBlade activation/consumption/identity and binary publication.");
}
