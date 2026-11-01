#include "../src/vulkan_layer.cpp"
static void expect(bool valid,const char* message) { if(!valid){fprintf(stderr,"FAIL: %s\n",message);exit(1);} }
template<class T> static void set(void* data,size_t offset,T value) { memcpy(static_cast<unsigned char*>(data)+offset,&value,sizeof(value)); }
int main() {
    std::unordered_map<std::string,GraphType> schemas;
    auto list=[&](const char* type,const char* element,bool pointers) {
        schemas[type]={24,"",{{"list",element,pointers?"**":"*",0,8},{"num","int","",8,4}},true};
    };
    const char* subs="idList < idDeclAnimWeb::idSubWebInfo , TAG_DECL_ANIMWEB , false >";
    const char* states="idList < idAtomicString , TAG_DECL_ANIMWEB , false >";
    const char* nodes="idList < idDeclAnimWebNode * , TAG_DECL_ANIMWEB , false >";
    const char* models="idList < idAnimWebModelInfo , TAG_DECL_ANIMWEB , false >";
    const char* props="idList < idMD6AnimProps , TAG_DECL_ANIMWEB , false >";
    list(subs,"idDeclAnimWeb::idSubWebInfo",false);list(states,"idAtomicString",false);list(nodes,"idDeclAnimWebNode",true);list(models,"idAnimWebModelInfo",false);list(props,"idMD6AnimProps",false);
    schemas["idAtomicString"]={8,"",{{"str","char","*",0,8}},true};
    schemas["idAnimator_AnimWeb"]={64,"",{{"decl","idDeclAnimWeb","*",0,8}},true};
    schemas["idDeclWeapon"]={64,"",{{"subweb_normal","idAtomicString","",0,8},{"secondaryFireDecl","idDeclWeapon","*",8,8}},true};
    schemas["idDeclAnimWeb"]={128,"",{{"subWebs",subs,"",0,24},{"states",states,"",24,24},{"nodes",nodes,"",48,24}},true};
    schemas["idDeclAnimWeb::idSubWebInfo"]={56,"",{{"name","idAtomicString","",24,8}},true};
    schemas["idDeclAnimWebNode"]={176,"",{{"subWebIndex","idIndex < short , idAnimWeb::invalidSubWebIndex_t , - 1 >","",50,2},
        {"stateIndex","idIndex < short , idAnimWeb::invalidStateIndex_t , - 1 >","",52,2},{"modelInfo",models,"",112,24}},true};
    schemas["idAnimWebModelInfo"]={64,"",{{"modelIndex","idIndex < short , idAnimWeb::invalidModelIndex_t , - 1 >","",0,2},{"animProps",props,"",24,24}},true};
    schemas["idMD6AnimProps"]={24,"",{{"anim","idMD6Anim","*",0,8},{"aliasHandle","idHandle < unsigned short , invalidAliasHandle_t , INVALID_ALIAS_HANDLE >","",8,2}},true};
    schemas["idMD6Anim"]={176,"",{{"animData","idMD6AnimData","*",96,8}},true};
    schemas["idMD6AnimData"]={144,"",{{"numFrames","unsigned short","",16,2},{"frameRate","unsigned short","",18,2}},true};
    auto lookup=[&](const std::string& type)->const GraphType& { static const GraphType empty;auto found=schemas.find(type);return found==schemas.end()?empty:found->second; };
    unsigned char definition[64]{},animator[64]{},web[128]{},subwebs[112]{},names[16]{},pointers[16]{},wrong[176]{},node[176]{},modelData[128]{},animations[48]{},animation[176]{},data[144]{};
    auto address=[](void* p){return reinterpret_cast<uintptr_t>(p);};
    const char* name="heavy_cannon",*other="other",*idle="idle",*bringup="bringup";
    set(definition,0,name);set(animator,0,address(web));
    set(web,0,address(subwebs));set(web,8,int32_t(2));set(subwebs,24,other);set(subwebs,56+24,name);
    set(web,24,address(names));set(web,32,int32_t(2));set(names,0,idle);set(names,8,bringup);
    set(web,48,address(pointers));set(web,56,int32_t(2));set(pointers,0,address(wrong));set(pointers,8,address(node));
    set(wrong,50,int16_t(1));set(wrong,52,int16_t(0));set(node,50,int16_t(1));set(node,52,int16_t(1));
    set(node,112,address(modelData));set(node,120,int32_t(2));set(modelData,0,int16_t(1));set(modelData,64,int16_t(0));
    set(modelData,64+24,address(animations));set(modelData,64+32,int32_t(2));set(animations,0,address(animation));set(animations,8,uint16_t(65535));
    set(animation,96,address(data));set(data,16,uint16_t(36));set(data,18,uint16_t(30));
    float seconds=0;
    expect(batsBringupClip(WallMemory{},address(animator),address(definition),lookup,seconds)&&std::abs(seconds-1.2f)<.0001f,"normal clip resolves the exact subweb/state/model and first animation");
    expect(configuredBatsState(5,seconds/.4f,.6f,.3f)=="inactive","naturally fast Heavy Cannon is not falsely flagged as BATS");
    expect(configuredBatsState(10,seconds/.4f,.6f,.3f)=="active","acceleration beyond even a fast weapon's own baseline is detected");
    set(wrong,52,int16_t(1));expect(!batsBringupClip(WallMemory{},address(animator),address(definition),lookup,seconds),"ambiguous matching graph nodes are rejected");set(wrong,52,int16_t(0));
    set(animations,8,uint16_t(7));expect(!batsBringupClip(WallMemory{},address(animator),address(definition),lookup,seconds),"unresolved alias handles cannot borrow a direct animation");set(animations,8,uint16_t(65535));
    set(data,18,uint16_t(0));expect(!batsBringupClip(WallMemory{},address(animator),address(definition),lookup,seconds),"invalid clip FPS never becomes a baseline");set(data,18,uint16_t(30));
    set(web,56,int32_t(8193));expect(!batsBringupClip(WallMemory{},address(animator),address(definition),lookup,seconds),"graph enumeration is bounded");set(web,56,int32_t(2));
    schemas[nodes].fields[0].ops="*";expect(!batsBringupClip(WallMemory{},address(animator),address(definition),lookup,seconds),"node pointer depth must match reflection");schemas[nodes].fields[0].ops="**";
    struct Weapon {const char* name;int ms;int frames;};
    const Weapon audited[]={{"Combat Shotgun",0,36},{"Heavy Cannon",400,36},{"Plasma Rifle",400,36},{"Rocket Launcher",0,30},
        {"Super Shotgun",400,36},{"Ballista",0,30},{"Chaingun",400,36},{"BFG",400,36},{"Crucible",0,28},{"Unmaykr",400,30},{"Chainsaw",500,36}};
    for(const auto& weapon:audited) for(int mode=0;mode<2;++mode) {
        const float clip=weapon.ms?float(weapon.frames)/30/(float(weapon.ms)*.001f):1.f;
        const float modScale=mode?2.f:1.f,normal=clip*modScale/.6f;
        expect(configuredBatsState(normal,clip*modScale,.6f,.3f)=="inactive","every audited weapon and distinct mod rate keeps its own ordinary baseline inactive");
        expect((configuredBatsState(5,clip*modScale,.6f,.3f)=="active")== (normal<4.999f),"rate 5 is a buff only when faster than this weapon and active mode's ordinary appearance");
    }
    schemas["idWeapon"]={64,"",{{"superchargeParms","idWeapon::superchargeParms_t","[2]",0,32},{"weaponModOverrides","idWeapon::weaponModOverrides_t","[2]",32,32}},true};
    schemas["idWeapon::superchargeParms_t"]={16,"",{{"weaponDecl","idDeclWeapon","*",0,8}},true};
    schemas["idWeapon::weaponModOverrides_t"]={16,"",{{"weaponDecl","idDeclWeapon","*",0,8}},true};
    unsigned char weapon[64]{},secondary[64]{},modded[64]{},supercharged[64]{};uintptr_t selected=0;
    set(definition,8,address(secondary));
    expect(batsModeDefinition(WallMemory{},address(weapon),address(definition),0,lookup,selected)&&selected==address(definition),"primary mode uses base clip declaration");
    expect(batsModeDefinition(WallMemory{},address(weapon),address(definition),1,lookup,selected)&&selected==address(secondary),"secondary mode uses its own clip declaration");
    set(weapon,48,address(modded));expect(batsModeDefinition(WallMemory{},address(weapon),address(definition),1,lookup,selected)&&selected==address(modded),"active mod declaration takes precedence over ordinary secondary");
    set(weapon,16,address(supercharged));expect(batsModeDefinition(WallMemory{},address(weapon),address(definition),1,lookup,selected)&&selected==address(supercharged),"supercharged mode takes precedence over active mod");
    expect(!batsModeDefinition(WallMemory{},address(weapon),address(definition),2,lookup,selected),"invalid mode cannot read beyond either override array");
    schemas["idList < idStatusEffect * , TAG_IDLIST , false >"]={24,"",{{"list","idStatusEffect","**",0,8},{"num","int","",8,4}},true};
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x4000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));expect(image!=nullptr,"neutral effect fixture image");
    auto profile=kProfile51;profile.imageSize=0x4000;const auto imageAddress=address(image);
    unsigned char effects[104]{},effect[16]{};uintptr_t entries[1]{address(effect)};
    set(effects,0,address(entries));set(effects,8,int32_t(1));set(effect,0,imageAddress+0x100);
    set(image,0x1e0,imageAddress+0x300);const unsigned char constantGetter[]{0xf3,0x0f,0x10,0x05,0xf8,0,0,0,0xc3};
    memcpy(image+0x300,constantGetter,9);set(image,0x400,1.f);
    expect(batsNeutralStatusEffects(WallMemory{},imageAddress,profile,address(effects),104,1,lookup),"retained neutral Crucible status does not disable BATS for every weapon");
    set(image,0x400,.5f);expect(!batsNeutralStatusEffects(WallMemory{},imageAddress,profile,address(effects),104,1,lookup),"non-neutral status multipliers cannot become false BATS positives");
    set(image,0x400,1.f);image[0x300]=0xe9;expect(!batsNeutralStatusEffects(WallMemory{},imageAddress,profile,address(effects),104,1,lookup),"unknown virtual effect code is never called or guessed");
    VirtualFree(image,0,MEM_RELEASE);
    puts("PASS: real bring-up clip metadata, naturally fast weapons, accelerated rates, mode matrix, ambiguity/alias/FPS/list guards.");
}
