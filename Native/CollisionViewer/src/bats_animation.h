#pragma once

struct BatsArray { uintptr_t data=0,address=0;int32_t count=0;uint32_t stride=0; };
static bool batsNeutralStatusEffects(const WallMemory& memory,uintptr_t base,const GameProfile& profile,uintptr_t address,
                                     uint32_t bytes,int32_t count,const GraphLookup& lookup,uint32_t switchSlot=0xe0) {
    if(count==0)return true;
    if(count<0||count>10||switchSlot>0x800||switchSlot%8)return false;
    const auto* data=techField(lookup,"idList < idStatusEffect * , TAG_IDLIST , false >","list");
    const auto* num=techField(lookup,"idList < idStatusEffect * , TAG_IDLIST , false >","num");
    if(!data||data->size!=8||data->type!="idStatusEffect"||data->ops!="**"||uint64_t(data->offset)+8>bytes||!num||uint64_t(num->offset)+4>bytes)return false;
    uintptr_t list=0;if(!memory.get(address+data->offset,list)||!list)return false;
    auto inside=[&](uintptr_t at,size_t size){return at>=base&&at-base<profile.imageSize&&size<=profile.imageSize-(at-base);};
    for(int32_t i=0;i<count;++i) {
        uintptr_t effect=0,vtable=0,getter=0;
        if(!memory.get(list+size_t(i)*8,effect))return false;
        if(!effect)continue;
        unsigned char code[9]{};int32_t displacement=0;float multiplier=0;
        // A constant-one switch-speed getter is neutral whether the effect is
        // equipped, expired or inactive. Crucible retains such an entry in the
        // list after use. Never execute its virtual methods from the worker.
        if(!memory.get(effect,vtable)||!inside(vtable,size_t(switchSlot)+8)||!memory.get(vtable+switchSlot,getter)||!inside(getter,9)||!memory.read(getter,code,9)||
           code[0]!=0xf3||code[1]!=0x0f||code[2]!=0x10||code[3]!=0x05||code[8]!=0xc3)return false;
        memcpy(&displacement,code+4,4);const auto constant=getter+8+displacement;
        if(!inside(constant,4)||!memory.get(constant,multiplier)||multiplier!=1.f)return false;
        uintptr_t finalEffect=0,finalVtable=0,finalGetter=0;
        if(!memory.get(list+size_t(i)*8,finalEffect)||finalEffect!=effect||!memory.get(effect,finalVtable)||finalVtable!=vtable||
           !memory.get(vtable+switchSlot,finalGetter)||finalGetter!=getter)return false;
    }
    uintptr_t finalList=0;int32_t finalCount=-1;
    return memory.get(address+data->offset,finalList)&&finalList==list&&memory.get(address+num->offset,finalCount)&&finalCount==count;
}
static bool batsArray(const WallMemory& memory,uintptr_t object,const GraphLookup& lookup,const char* owner,const char* member,
                      const char* element,bool pointers,int32_t limit,BatsArray& result) {
    const auto* field=techField(lookup,owner,member);
    if(!field||!techInline(field,field->type.c_str(),lookup)) return false;
    const auto* data=techField(lookup,field->type,"list");const auto* count=techField(lookup,field->type,"num");
    const auto& schema=lookup(element);
    if(!data||data->size!=8||data->type!=element||data->ops!=(pointers?"**":"*")||!count||count->type!="int"||count->size!=4||!count->ops.empty()||
       (!pointers&&(!schema.complete||!schema.size||schema.size>0x200000))) return false;
    result.address=object+field->offset;result.stride=pointers?8:schema.size;
    return memory.get(result.address+data->offset,result.data)&&memory.get(result.address+count->offset,result.count)&&
        result.count>=0&&result.count<=limit&&(!result.count||result.data);
}
static bool batsIndex(const GraphField* field) { return field&&field->size==2&&field->ops.empty()&&field->type.rfind("idIndex < short ,",0)==0; }
static bool batsUnsignedShort(const GraphField* field) { return field&&field->size==2&&field->ops.empty()&&field->type=="unsigned short"; }
static std::string batsAtomicString(const WallMemory& memory,uintptr_t address,const GraphLookup& lookup) {
    auto* str=techField(lookup,"idAtomicString","str");uintptr_t text=0;
    return techPointer(str,"char")&&memory.get(address+str->offset,text)?readEntityText(memory,text,512):std::string{};
}
static bool batsModeDefinition(const WallMemory& memory,uintptr_t weapon,uintptr_t base,int32_t mode,const GraphLookup& lookup,uintptr_t& selected) {
    if(mode<0||mode>1)return false;
    // Same precedence as idWeapon::GetDeclWeapon(mode): supercharge, mod,
    // secondary declaration, base declaration. Timing milliseconds still come
    // from the base inventory declaration in the audited SetWeapon calculation.
    for(const auto& pair:{std::pair<const char*,const char*>{"superchargeParms","idWeapon::superchargeParms_t"},
                         {"weaponModOverrides","idWeapon::weaponModOverrides_t"}}) {
        const auto* field=techField(lookup,"idWeapon",pair.first);const auto& schema=lookup(pair.second);
        const auto* decl=techField(lookup,pair.second,"weaponDecl");
        if(!field||field->type!=pair.second||field->ops!="[2]"||!schema.complete||!schema.size||field->size!=schema.size*2||!techPointer(decl,"idDeclWeapon")||
           !memory.get(weapon+field->offset+size_t(mode)*schema.size+decl->offset,selected))return false;
        if(selected)return true;
    }
    const auto* secondary=techField(lookup,"idDeclWeapon","secondaryFireDecl");
    if(mode==1) { if(!techPointer(secondary,"idDeclWeapon")||!memory.get(base+secondary->offset,selected))return false;if(selected)return true; }
    selected=base;return base!=0;
}
// SetWeapon asks GetAnimInfo for model 0, animation 0 of the selected weapon's
// normal subweb/bringup state. Its calculation uses numFrames / frameRate
// (not the frames-1 playback deadline used by countdowns).
static bool batsBringupClip(const WallMemory& memory,uintptr_t animator,uintptr_t definition,const GraphLookup& lookup,float& seconds) {
    const auto* web=techField(lookup,"idAnimator_AnimWeb","decl");
    const auto* sub=techField(lookup,"idDeclWeapon","subweb_normal");
    if(!techPointer(web,"idDeclAnimWeb")||!techInline(sub,"idAtomicString",lookup)) return false;
    uintptr_t declaration=0;if(!memory.get(animator+web->offset,declaration)||!declaration) return false;
    const auto subName=batsAtomicString(memory,definition+sub->offset,lookup);if(subName.empty()) return false;
    BatsArray subwebs,states,nodes;
    if(!batsArray(memory,declaration,lookup,"idDeclAnimWeb","subWebs","idDeclAnimWeb::idSubWebInfo",false,512,subwebs)||
       !batsArray(memory,declaration,lookup,"idDeclAnimWeb","states","idAtomicString",false,512,states)||
       !batsArray(memory,declaration,lookup,"idDeclAnimWeb","nodes","idDeclAnimWebNode",true,8192,nodes)) return false;
    auto* subField=techField(lookup,"idDeclAnimWeb::idSubWebInfo","name");
    auto* nodeSub=techField(lookup,"idDeclAnimWebNode","subWebIndex");auto* nodeState=techField(lookup,"idDeclAnimWebNode","stateIndex");
    if(!techInline(subField,"idAtomicString",lookup)||!batsIndex(nodeSub)||!batsIndex(nodeState)) return false;
    int16_t subIndex=-1,stateIndex=-1;
    for(int32_t i=0;i<subwebs.count;++i) if(batsAtomicString(memory,subwebs.data+size_t(i)*subwebs.stride+subField->offset,lookup)==subName) {
        if(subIndex!=-1)return false;subIndex=static_cast<int16_t>(i);
    }
    for(int32_t i=0;i<states.count;++i) if(batsAtomicString(memory,states.data+size_t(i)*states.stride,lookup)=="bringup") {
        if(stateIndex!=-1)return false;stateIndex=static_cast<int16_t>(i);
    }
    if(subIndex<0||stateIndex<0)return false;
    uintptr_t selected=0;
    for(int32_t i=0;i<nodes.count;++i) {
        uintptr_t node=0;int16_t s=-1,t=-1;
        if(!memory.get(nodes.data+size_t(i)*8,node)||!node||!memory.get(node+nodeSub->offset,s)||!memory.get(node+nodeState->offset,t))return false;
        if(s==subIndex&&t==stateIndex) { if(selected)return false;selected=node; }
    }
    if(!selected)return false;
    BatsArray models;
    auto* modelIndex=techField(lookup,"idAnimWebModelInfo","modelIndex");
    if(!batsIndex(modelIndex)||!batsArray(memory,selected,lookup,"idDeclAnimWebNode","modelInfo","idAnimWebModelInfo",false,32,models))return false;
    uintptr_t selectedModel=0;
    for(int32_t i=0;i<models.count;++i) {
        const auto address=models.data+size_t(i)*models.stride;int16_t index=-1;if(!memory.get(address+modelIndex->offset,index))return false;
        if(index==0) { if(selectedModel)return false;selectedModel=address; }
    }
    BatsArray animations;
    if(!selectedModel||!batsArray(memory,selectedModel,lookup,"idAnimWebModelInfo","animProps","idMD6AnimProps",false,256,animations)||!animations.count)return false;
    auto* anim=techField(lookup,"idMD6AnimProps","anim");auto* alias=techField(lookup,"idMD6AnimProps","aliasHandle");
    auto* data=techField(lookup,"idMD6Anim","animData");auto* frames=techField(lookup,"idMD6AnimData","numFrames");auto* fps=techField(lookup,"idMD6AnimData","frameRate");
    if(!techPointer(anim,"idMD6Anim")||!alias||alias->size!=2||!alias->ops.empty()||!techPointer(data,"idMD6AnimData")||!batsUnsignedShort(frames)||!batsUnsignedShort(fps))return false;
    uint16_t handle=0,frameCount=0,frameRate=0;uintptr_t animation=0,frameData=0;
    // Alias-based resources need a separate MD6 alias resolver. Do not guess.
    if(!memory.get(animations.data+alias->offset,handle)||handle!=65535||!memory.get(animations.data+anim->offset,animation)||!animation||
       !memory.get(animation+data->offset,frameData)||!frameData||!memory.get(frameData+frames->offset,frameCount)||!frameCount||
       !memory.get(frameData+fps->offset,frameRate)||!frameRate||frameRate>1000)return false;
    uintptr_t finalWeb=0,finalAnimation=0,finalData=0;uint16_t finalFrames=0,finalFps=0,finalAlias=0;
    BatsArray finalSubwebs,finalStates,finalNodes,finalModels,finalAnimations;
    if(!memory.get(animator+web->offset,finalWeb)||finalWeb!=declaration||batsAtomicString(memory,definition+sub->offset,lookup)!=subName||
       !batsArray(memory,declaration,lookup,"idDeclAnimWeb","subWebs","idDeclAnimWeb::idSubWebInfo",false,512,finalSubwebs)||finalSubwebs.data!=subwebs.data||finalSubwebs.count!=subwebs.count||
       !batsArray(memory,declaration,lookup,"idDeclAnimWeb","states","idAtomicString",false,512,finalStates)||finalStates.data!=states.data||finalStates.count!=states.count||
       !batsArray(memory,declaration,lookup,"idDeclAnimWeb","nodes","idDeclAnimWebNode",true,8192,finalNodes)||finalNodes.data!=nodes.data||finalNodes.count!=nodes.count||
       !batsArray(memory,selected,lookup,"idDeclAnimWebNode","modelInfo","idAnimWebModelInfo",false,32,finalModels)||finalModels.data!=models.data||finalModels.count!=models.count||
       !batsArray(memory,selectedModel,lookup,"idAnimWebModelInfo","animProps","idMD6AnimProps",false,256,finalAnimations)||finalAnimations.data!=animations.data||finalAnimations.count!=animations.count||
       !memory.get(animations.data+anim->offset,finalAnimation)||finalAnimation!=animation||!memory.get(animations.data+alias->offset,finalAlias)||finalAlias!=handle||
       !memory.get(animation+data->offset,finalData)||finalData!=frameData||!memory.get(frameData+frames->offset,finalFrames)||finalFrames!=frameCount||
       !memory.get(frameData+fps->offset,finalFps)||finalFps!=frameRate)return false;
    seconds=float(frameCount)/float(frameRate);return std::isfinite(seconds)&&seconds>0&&seconds<=3600;
}
