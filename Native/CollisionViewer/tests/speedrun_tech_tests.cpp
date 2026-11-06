#include "../src/vulkan_layer.cpp"
#include "../src/speedrun_techs.h"
static void expect(bool valid,const char* message) { if(!valid) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
template<class T> static void set(void* data,size_t offset,T value) { memcpy(static_cast<unsigned char*>(data)+offset,&value,sizeof(value)); }

// Public fixtures are synthetic. Retail EXE audit data stays local in private/.
int main() {
    {
        std::unordered_map<std::string,GraphType> types;
        auto add=[&](const char* name,const char* parent,uint32_t size){GraphType type;type.complete=true;type.parent=parent;type.size=size;types[name]=std::move(type);};
        add("idWeapon","idInventoryItem",256);
        add("idPlasmaRifleCharged","idWeapon",384);add("idChaingun","idWeapon",320);
        add("idDoubleBarrelShotgun","idWeapon",352);add("idBFG","idWeapon",256);
        add("FurtherPlasmaSubclass","idPlasmaRifleCharged",512);
        GraphLookup lookup=[&](const std::string& name)->const GraphType& {static const GraphType missing;auto found=types.find(name);return found==types.end()?missing:found->second;};
        for(const char* name:{"idWeapon","idPlasmaRifleCharged","idChaingun","idDoubleBarrelShotgun","idBFG","FurtherPlasmaSubclass"})
            expect(comboChainWeaponClass(name,lookup),"CSC accepts verified weapon subclasses, including multi-level inheritance");
        add("NotAWeapon","idInventoryItem",384);
        expect(!comboChainWeaponClass("NotAWeapon",lookup)&&!comboChainWeaponClass("MissingWeaponType",lookup)&&!comboChainWeaponClass("",lookup),"unrelated or unknown class names cannot masquerade as weapons");
        add("BrokenChild","idWeapon",128);add("HugeChild","idWeapon",0x200001);
        expect(!comboChainWeaponClass("BrokenChild",lookup)&&!comboChainWeaponClass("HugeChild",lookup),"incompatible child sizes are rejected");
        add("CycleA","CycleB",300);add("CycleB","CycleA",300);
        expect(!comboChainWeaponClass("CycleA",lookup),"cyclic parent metadata terminates without accepting a weapon");
        types["idWeapon"].complete=false;
        expect(!comboChainWeaponClass("idPlasmaRifleCharged",lookup),"incomplete base metadata cannot certify a derived weapon");
    }
    ComboChainTracker chain;ComboChainSample chainSample{1,2,3,10,1,false,false,100};
    expect(!chain.update(chainSample),"ordinary idle has no CSC");
    chainSample.modChange=true;chainSample.observed=150;
    expect(!chain.update(chainSample),"ordinary mod change is not CSC");
    chainSample.observed=300;expect(!chain.update(chainSample),"a long ordinary mod animation is not CSC");
    chainSample.weapon=11;chainSample.observed=350;
    expect(chain.update(chainSample),"a handoff retaining mod-change starts CSC");
    chainSample.observed=450;expect(chain.update(chainSample),"CSC survives samples without a new input");
    chainSample.modChange=false;chainSample.observed=500;
    expect(!chain.update(chainSample),"clearing the native mod-change flag ends CSC immediately");
    chainSample.modChange=true;chainSample.ghostFire=true;chainSample.observed=550;
    expect(chain.update(chainSample),"coherent ghost-fire evidence can acquire an already-running chain");
    chainSample.ghostFire=false;chainSample.map=9;chainSample.observed=600;
    expect(!chain.update(chainSample),"a new map never inherits a previous CSC");
    chainSample.weapon=12;chainSample.observed=1200;
    expect(!chain.update(chainSample),"stale sample gaps cannot invent a handoff");
    chain.clear();chainSample.observed=1250;
    expect(!chain.update(chainSample),"an unreadable observation clears retained chain history");
    expect(inverseBatsObserved(true,true,false,true,false,true),"observed plasma ghost-fire flags identify Inverse BATS");
    expect(!inverseBatsObserved(false,true,false,true,false,true),"ordinary pending fire has no retained mod-change");
    expect(!inverseBatsObserved(true,true,true,true,false,true),"actual plasma firing is not ghost-fire");
    expect(!inverseBatsObserved(true,true,false,true,true,true),"dry-fire never becomes Inverse BATS");
    expect(!inverseBatsObserved(true,false,false,true,false,true),"mod-change alone does not prove a ghost shot");
    expect(fastDestroyerBladeArmed(999960,1999920),"shorter next-charge override is armed");
    for(int64_t once:{-1LL,0LL,1999920LL,2999880LL}) expect(!fastDestroyerBladeArmed(once,1999920),"consumed, zero and ordinary/long overrides are not fast");
    expect(configuredBatsState(5,1,.6f,.3f)=="active","faster configured bring-up is active");
    expect(configuredBatsState(1.6666666269302368f,1,.6f,.3f)=="inactive","Dexterity is part of the normal rate");
    expect(configuredBatsState(1,1,1,.3f)=="inactive"&&configuredBatsState(2,2,1,.3f)=="inactive","unupgraded and mode-scaled normal rates stay inactive");
    expect(configuredBatsState(.1f,1,1,.3f)=="slower"&&configuredBatsState(NAN,1,1,.3f)=="unavailable","slower rates remain distinct from invalid samples");
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
