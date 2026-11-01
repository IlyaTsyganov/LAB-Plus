#pragma once
// Included after the shared read-only timer helpers.
// The release-event lookup is read, never called. This is the original
// idEventDef indexed getter on the independently audited timer builds. Its
// release callback takes the source idMD6Anim pointer on the audited EXEs.
static int syncReleaseEventNumber(const WallMemory& memory,uintptr_t base,const GameProfile& profile,const GraphLookup& lookup) {
    uintptr_t getter=0;
    if(profile.timestamp==0x5e794131&&profile.imageSize==0x1e706000)getter=0x44b2f0;
    if(profile.timestamp==0x5ec7deca&&profile.imageSize==0x1d551000)getter=0x44f870;
    if(profile.timestamp==0x607e69b0&&profile.imageSize==0x1b5dd000)getter=0x474360;
    if(const auto* code=observationCodeProfile(profile))getter=code->event;
    if(!getter)return -1;
    unsigned char code[31]{};
    const unsigned char prefix[]{0x48,0x83,0xec,0x28,0x81,0xfa,0xff,0x0f,0,0,0x77,0x13,0x48,0x63,0xc2,0x48,0x8d,0x0d};
    const unsigned char tail[]{0x48,0x8b,0x04,0xc1,0x48,0x83,0xc4,0x28,0xc3};
    if(!memory.read(base+getter,code,sizeof(code))||memcmp(code,prefix,sizeof(prefix))||memcmp(code+22,tail,sizeof(tail)))return -1;
    int32_t relative=0;memcpy(&relative,code+18,4);
    const uintptr_t table=base+getter+22+relative;
    if(table<base||table-base>=profile.imageSize||4096*8>profile.imageSize-(table-base))return -1;
    auto name=timerField(lookup,"idEventDef","name"),number=timerField(lookup,"idEventDef","eventnum"),
         type=timerField(lookup,"idEventDef","type"),args=timerField(lookup,"idEventDef","numargs"),
         format=timerField(lookup,"idEventDef","formatspec"),argTypes=timerField(lookup,"idEventDef","argTypes"),argNames=timerField(lookup,"idEventDef","argNames");
    if(!name||name->type!="char"||name->ops!="*"||name->size!=8||!timerIntField(number)||!timerIntField(args)||
       !type||type->size!=4||!type->ops.empty()||type->type!="eventType_t"||!timerPointer(format,"char")||!timerPointer(argTypes,"char")||!timerPointer(argNames,"char"))return -1;
    auto matches=[&](uintptr_t definition,int index) {
        uintptr_t text=0,spec=0,types=0,names=0;int32_t actual=-1,kind=-1,count=-1;
        return definition>=base&&definition-base<profile.imageSize&&lookup("idEventDef").size<=profile.imageSize-(definition-base)&&
            memory.get(definition+name->offset,text)&&readEntityText(memory,text,128)=="ae_releasePlayerFromSync"&&
            memory.get(definition+number->offset,actual)&&actual==index&&memory.get(definition+type->offset,kind)&&kind==2&&
            memory.get(definition+args->offset,count)&&count==1&&
            memory.get(definition+format->offset,spec)&&readEntityText(memory,spec,32)=="4"&&
            memory.get(definition+argTypes->offset,types)&&readEntityText(memory,types,64)=="idMD6Anim*;"&&
            memory.get(definition+argNames->offset,names)&&readEntityText(memory,names,32)=="anim;";
    };
    struct Cached { HANDLE remote{};uintptr_t base=0,definition=0;int index=-1; };
    static thread_local Cached cached;
    uintptr_t current=0;
    if(cached.remote==memory.remote&&cached.base==base&&cached.index>=0&&memory.get(table+size_t(cached.index)*8,current)&&
       current==cached.definition&&matches(current,cached.index))return cached.index;
    cached={};
    std::array<uintptr_t,4096> definitions{};
    if(!memory.read(table,definitions.data(),sizeof(definitions)))return -1;
    for(size_t index=0;index<definitions.size();++index) {
        if(!definitions[index])break;
        if(matches(definitions[index],static_cast<int>(index))) {
            cached={memory.remote,base,definitions[index],static_cast<int>(index)};return static_cast<int>(index);
        }
    }
    return -1;
}
static bool syncPlayerLockProfile(const GameProfile* profile) {
    // Exact event getters are audited per executable. The actual per-model
    // release event, ownership and animation timeline remain mandatory.
    return profile&&observationCodeProfile(*profile);
}
static bool syncReleaseDeadline(uint16_t frames,uint16_t fps,float frame,int64_t started,float fraction,
                                const float (&scales)[3],float rate,int64_t ticksPerSecond,int64_t& deadline) {
    if(frames<2||!fps||fps>1000||!std::isfinite(frame)||frame<0||frame>float(frames-1)||started<0||
       !std::isfinite(fraction)||fraction>=0||!std::isfinite(rate)||ticksPerSecond!=999960)return false;
    for(float value:scales)if(!std::isfinite(value)||fabsf(value)>100)return false;
    const float effective=(((scales[2]*scales[0])-1.f)*scales[1]+1.f)*rate;
    if(!std::isfinite(effective)||effective<=0||effective>100)return false;
    const long double duration=std::ceil(static_cast<long double>(frame)*ticksPerSecond/(static_cast<long double>(fps)*effective));
    if(!std::isfinite(static_cast<double>(duration))||duration<0||duration>gameTimerTicksPerSecond*120||duration>INT64_MAX-started)return false;
    deadline=started+static_cast<int64_t>(duration);return true;
}
// Old builds through 2.1 store an explicit disabled bool. From 3.0 onward it is
// ANIM_EVENT_FLAG_DISABLED in the reflected enum; there is no bool to read.
// Verify the enum belonging to this exact EXE instead of treating a missing
// field as an enabled event.
static bool syncPackedEventFlags(const WallMemory& memory,uintptr_t base,const GameProfile* profile) {
    if(!profile)return false;
    const auto* code=observationCodeProfile(*profile);
    if(!code||!code->syncEventFlagsEnum||code->syncEventFlagsEnum>=profile->imageSize||32>profile->imageSize-code->syncEventFlagsEnum)return false;
    const auto at=base+code->syncEventFlagsEnum;
    uintptr_t name=0,values=0;uint64_t kind=0;uint32_t count=0;
    if(!memory.get(at,name)||readEntityText(memory,name,128)!="animEventFlags_t"||!memory.get(at+8,kind)||kind!=0x80000||
       !memory.get(at+20,count)||count!=5||!memory.get(at+24,values)||values<base||values-base>=profile->imageSize||
       uint64_t(count+1)*16>profile->imageSize-(values-base))return false;
    const char* names[]{"ANIM_EVENT_FLAG_NONE","ANIM_EVENT_FLAG_DISABLED","ANIM_EVENT_FLAG_DEBUG","ANIM_EVENT_FLAG_LOCKED","ANIM_EVENT_ACTIVATE_IN_RANGE"};
    const uint64_t bits[]{0,2,4,8,16};
    for(unsigned i=0;i<5;++i) {
        uintptr_t text=0;uint64_t value=~0ULL;
        if(!memory.get(values+i*16,text)||!memory.get(values+i*16+8,value)||value!=bits[i]||readEntityText(memory,text,128)!=names[i])return false;
    }
    uintptr_t terminator=1;return memory.get(values+count*16,terminator)&&!terminator;
}
static bool syncAnimationRefLayout(const GraphLookup& lookup,const GraphField* field,uint32_t& stringOffset) {
    if(!field||!field->ops.empty()||field->size!=8)return false;
    if(field->type=="idAtomicStringT < md6AnimAtomicString_t >") {stringOffset=0;return true;}
    if(field->type!="idMD6AnimRef")return false;
    const auto& ref=lookup("idMD6AnimRef");const auto& atomic=lookup("idAtomicString");
    const auto* text=timerField(lookup,"idAtomicString","str");
    if(!ref.complete||ref.size!=field->size||ref.parent!="idAtomicString"||!atomic.complete||atomic.size>ref.size||
       !timerPointer(text,"char")||uint64_t(text->offset)+8>ref.size)return false;
    stringOffset=text->offset;return true;
}
static bool syncEventStorage(const WallMemory& memory,uintptr_t base,const GameProfile* profile,const GraphLookup& lookup,const GraphField* disabled) {
    if(disabled)return timerBoolField(disabled);
    const auto* row=timerField(lookup,"idMD6AnimEvent","row");
    return row&&row->type=="unsigned char"&&row->size==1&&row->ops.empty()&&syncPackedEventFlags(memory,base,profile);
}
static bool syncReadEventDisabled(const WallMemory& memory,uintptr_t event,const GraphField* field,uint32_t flags,unsigned char& disabled) {
    if(!field) {disabled=(flags&2)!=0;return true;}
    return memory.get(event+field->offset,disabled)&&disabled<=1;
}
static GameTimerRow captureSyncPlayerControlLock(const WallMemory& memory,uintptr_t player,uintptr_t sync,const GameTimerClock& clock,
                                                const GraphLookup& lookup,int releaseNumber,uintptr_t base=0,const GameProfile* profile=nullptr) {
    GameTimerRow row{"interaction","Sync animations","Player lock","unavailable","Player release timeline unavailable",-1};
    auto managedPointer=[&](const GraphField* field,const char* target)->const GraphField* {
        const std::string type=std::string("idManagedClassPtr < ")+target+" >";
        if(!timerInline(field,type,lookup))return nullptr;
        const auto* pointer=timerField(lookup,type,"ptr");return timerPointer(pointer,target)?pointer:nullptr;
    };
    const auto* saved=timerField(lookup,"idPlayer","savedSyncEntity"),*master=timerField(lookup,"idPlayer","syncMaster");
    const auto* savedPtr=managedPointer(saved,"idSyncEntity"),*masterPtr=managedPointer(master,"idSyncEntity");
    uintptr_t savedSync=0,masterSync=0;
    if(!savedPtr||!masterPtr||!memory.get(player+saved->offset+savedPtr->offset,savedSync)||!memory.get(player+master->offset+masterPtr->offset,masterSync))return row;
    auto released=[&] {uintptr_t a=0,b=0;return memory.get(player+saved->offset+savedPtr->offset,a)&&memory.get(player+master->offset+masterPtr->offset,b)&&a!=sync&&b!=sync;};
    if(savedSync!=sync&&masterSync!=sync) {row.state="inactive";row.remaining=0;row.detail="Player control has been released";return row;}
    if(!clock.valid) {row.detail=clock.detail;return row;}
    if(releaseNumber<0||releaseNumber>4095) {row.detail="Player release event registry unavailable";return row;}
    const auto* stack=timerField(lookup,"idAnimatedEntity","animStack"),*pause=timerField(lookup,"idAnimStack","pauseTime"),*local=timerField(lookup,"idAnimStack","useClientLocalTime");
    int64_t pausedAt=0;unsigned char localTime=1;
    if(!timerInline(stack,"idAnimStack",lookup)||!timerTimeField(pause)||!timerBoolField(local)||
       !memory.get(sync+stack->offset+pause->offset,pausedAt)||!memory.get(sync+stack->offset+local->offset,localTime)||localTime)return row;
    auto animator=timerField(lookup,"idSyncEntity","animatorSyncEntity"),models=timerField(lookup,"idAnimator_AnimWeb","modelAnimStates"),
         web=timerField(lookup,"idAnimator_AnimWeb","decl"),modelTree=timerField(lookup,"idAnimator_AnimWeb::idModelAnimState","tree"),
         declarations=timerField(lookup,"idDeclAnimWeb","models"),definition=timerField(lookup,"idDeclAnimWeb::idModelAnimInfo","model");
    const std::string statesType="idList < idAnimator_AnimWeb::idModelAnimState , TAG_ANIMWEB , false >",modelInfo="idDeclAnimWeb::idModelAnimInfo",
        modelsType="idList < idDeclAnimWeb::idModelAnimInfo , TAG_DECL_ANIMWEB , false >";
    auto statesData=timerField(lookup,statesType,"list"),statesCount=timerField(lookup,statesType,"num"),
         declData=timerField(lookup,modelsType,"list"),declCount=timerField(lookup,modelsType,"num");
    auto list=[&](const GraphField* container,const std::string& type,const GraphField* data,const GraphField* count,const char* element) {
        return timerInline(container,type,lookup)&&data&&data->size==8&&data->ops=="*"&&data->type==element&&timerIntField(count)&&lookup(element).complete&&lookup(element).size;
    };
    if(!timerInline(animator,"idAnimator_SyncEntity",lookup)||!list(models,statesType,statesData,statesCount,"idAnimator_AnimWeb::idModelAnimState")||
       !timerPointer(web,"idDeclAnimWeb")||!timerPointer(modelTree,"idMD6Node")||!list(declarations,modelsType,declData,declCount,modelInfo.c_str())||!timerPointer(definition,"idDeclMD6"))return row;
    const uintptr_t animatorAddress=sync+animator->offset,stateAddress=animatorAddress+models->offset;
    uintptr_t states=0,webDecl=0,decls=0;int32_t stateNum=0,declNum=0;
    if(!memory.get(stateAddress+statesData->offset,states)||!memory.get(stateAddress+statesCount->offset,stateNum)||
       !memory.get(animatorAddress+web->offset,webDecl)||!webDecl||!memory.get(webDecl+declarations->offset+declData->offset,decls)||
       !memory.get(webDecl+declarations->offset+declCount->offset,declNum)||stateNum<1||stateNum>64||stateNum!=declNum||!states||!decls)return row;
    auto animEvents=timerField(lookup,"idDeclMD6","animEvents"),infos=timerField(lookup,"idAnimEvents","list");
    const std::string infosType="idList < idAnimEvents::animEventInfo_t , TAG_MD6_ANIMEVENTS , false >",infoType="idAnimEvents::animEventInfo_t",
         eventsType="idList < idMD6AnimEvent , TAG_MD6_ANIMEVENTS , false >";
    auto infoData=timerField(lookup,infosType,"list"),infoCount=timerField(lookup,infosType,"num"),animRef=timerField(lookup,infoType,"animRef"),
         events=timerField(lookup,infoType,"events"),eventData=timerField(lookup,eventsType,"list"),eventCount=timerField(lookup,eventsType,"num"),
         eventFrame=timerField(lookup,"idMD6AnimEvent","frameNum"),eventNumber=timerField(lookup,"idMD6AnimEvent","eventNum"),
         eventDisabled=timerField(lookup,"idMD6AnimEvent","disabled"),eventFlags=timerField(lookup,"idMD6AnimEvent","flags");
    uint32_t animRefStringOffset=0;
    row.detail="Player-release animation event metadata unavailable";
    if(!timerInline(animEvents,"idAnimEvents",lookup)||!list(infos,infosType,infoData,infoCount,infoType.c_str())||
       !syncAnimationRefLayout(lookup,animRef,animRefStringOffset)||
       !list(events,eventsType,eventData,eventCount,"idMD6AnimEvent")||!eventFrame||eventFrame->size!=4||eventFrame->type!="float"||!eventFrame->ops.empty()||
       !eventNumber||eventNumber->size!=2||eventNumber->type!="short"||!eventNumber->ops.empty()||!syncEventStorage(memory,base,profile,lookup,eventDisabled)||
       !eventFlags||eventFlags->size!=4||!eventFlags->ops.empty()||eventFlags->type!="animEventFlags_t")return row;
    auto nodeType=timerField(lookup,"idMD6Node","type"),left=timerField(lookup,"idMD6Branch","left"),right=timerField(lookup,"idMD6Branch","right"),
         leftTime=timerField(lookup,"idMD6Branch","leftTimeOverride"),rightTime=timerField(lookup,"idMD6Branch","rightTimeOverride"),
         leafAnim=timerField(lookup,"idMD6Leaf","anim"),wrap=timerField(lookup,"idMD6Leaf","wrapMode"),weight=timerField(lookup,"idMD6Leaf","lastTotalWeight"),
         start=timerField(lookup,"idMD6LeafPlay","startTime"),scales=timerField(lookup,"idMD6LeafPlay","gameRateScales"),rate=timerField(lookup,"idMD6LeafPlay","rateScale"),
         fraction=timerField(lookup,"idMD6LeafPlay","startTimeFraction"),ticks=timerField(lookup,"idMD6LeafPlay","lastTicksPerSec"),
         animData=timerField(lookup,"idMD6Anim","animData"),frames=timerField(lookup,"idMD6AnimData","numFrames"),fps=timerField(lookup,"idMD6AnimData","frameRate");
    auto scalar=[](const GraphField* field){return field&&field->type=="float"&&field->size==4&&field->ops.empty();};
    auto small=[](const GraphField* field,const char* type,uint32_t size){return field&&field->type==type&&field->size==size&&field->ops.empty();};
    if(!small(nodeType,"unsigned char",1)||!timerPointer(left,"idMD6Node")||!timerPointer(right,"idMD6Node")||!timerTimeField(leftTime)||!timerTimeField(rightTime)||
       !timerPointer(leafAnim,"idMD6Anim")||!small(wrap,"unsigned char",1)||!scalar(weight)||!timerTimeField(start)||!scalar(rate)||!scalar(fraction)||!timerTimeField(ticks)||
       !scales||scales->type!="idArray < float , 3 >"||!scales->ops.empty()||scales->size!=12||!timerPointer(animData,"idMD6AnimData")||
       !small(frames,"unsigned short",2)||!small(fps,"unsigned short",2))return row;
    struct Schedule {uintptr_t leaf,animation,data,definition,event,root;int32_t model;uint16_t frames,fps;int64_t started,deadline;float eventFrame,rate,fraction,scales[3];int64_t ticks;};
    std::vector<Schedule> schedules;
    const auto stateSize=lookup("idAnimator_AnimWeb::idModelAnimState").size,declSize=lookup(modelInfo).size,infoSize=lookup(infoType).size,eventSize=lookup("idMD6AnimEvent").size;
    size_t work=0;
    for(int32_t model=0;model<stateNum;++model) {
        uintptr_t root=0,md6=0;
        if(!memory.get(states+size_t(model)*stateSize+modelTree->offset,root)||!memory.get(decls+size_t(model)*declSize+definition->offset,md6))return row;
        if(!root||!md6)continue;
        std::vector<uintptr_t> pending{root},seen;
        while(!pending.empty()) {
            const auto leaf=pending.back();pending.pop_back();if(!leaf)continue;
            if(++work>20000){row.detail="Player-release scan budget exhausted";return row;}
            if(seen.size()>=128||std::find(seen.begin(),seen.end(),leaf)!=seen.end())return row;seen.push_back(leaf);
            unsigned char type=255;if(!memory.get(leaf+nodeType->offset,type))return row;
            if(type==0||type==3||type==4) {
                uintptr_t a=0,b=0;int64_t ta=0,tb=0;
                if(!memory.get(leaf+left->offset,a)||!memory.get(leaf+right->offset,b)||!memory.get(leaf+leftTime->offset,ta)||!memory.get(leaf+rightTime->offset,tb)||ta>=0||tb>=0)return row;
                pending.push_back(a);pending.push_back(b);continue;
            }
            if(type!=2)continue;
            uintptr_t animation=0,data=0;unsigned char mode=1;float leafWeight=0,scaleValues[3]{},rateValue=0,fractionValue=0;int64_t started=0,tps=0;uint16_t frameCount=0,frameRate=0;
            if(!memory.get(leaf+leafAnim->offset,animation)||!memory.get(leaf+wrap->offset,mode)||!memory.get(leaf+weight->offset,leafWeight)||!std::isfinite(leafWeight))return row;
            if(!animation||mode||leafWeight<=.001f)continue;
            const auto animationName=timerResourceName(memory,animation,lookup);if(animationName.empty())return row;
            if(!memory.get(animation+animData->offset,data)||!data||!memory.get(data+frames->offset,frameCount)||!memory.get(data+fps->offset,frameRate)||
               !memory.get(leaf+start->offset,started)||!memory.get(leaf+scales->offset,scaleValues)||!memory.get(leaf+rate->offset,rateValue)||
               !memory.get(leaf+fraction->offset,fractionValue)||!memory.get(leaf+ticks->offset,tps))return row;
            uintptr_t infoList=0;int32_t infoNum=0;
            const auto infoAddress=md6+animEvents->offset+infos->offset;
            if(!memory.get(infoAddress+infoData->offset,infoList)||!memory.get(infoAddress+infoCount->offset,infoNum)||infoNum<0||infoNum>10000||(!infoList&&infoNum))return row;
            for(int32_t index=0;index<infoNum;++index) {
                if(++work>20000){row.detail="Player-release scan budget exhausted";return row;}
                const auto info=infoList+size_t(index)*infoSize;uintptr_t text=0;
                if(!memory.get(info+animRef->offset+animRefStringOffset,text))return row;
                if(readEntityText(memory,text,512)!=animationName)continue;
                uintptr_t eventList=0;int32_t eventNum=0;
                const auto eventAddress=info+events->offset;
                if(!memory.get(eventAddress+eventData->offset,eventList)||!memory.get(eventAddress+eventCount->offset,eventNum)||eventNum<0||eventNum>4096||(!eventList&&eventNum))return row;
                for(int32_t indexEvent=0;indexEvent<eventNum;++indexEvent) {
                    if(++work>20000){row.detail="Player-release scan budget exhausted";return row;}
                    const auto event=eventList+size_t(indexEvent)*eventSize;int16_t number=-1;unsigned char disabled=1;uint32_t flags=~0u;float frame=0;int64_t deadline=0;
                    if(!memory.get(event+eventNumber->offset,number))return row;
                    if(number!=releaseNumber)continue;
                    if(!memory.get(event+eventFlags->offset,flags)||!syncReadEventDisabled(memory,event,eventDisabled,flags,disabled)||!memory.get(event+eventFrame->offset,frame))return row;
                    if(disabled)continue;
                    if(flags) {row.detail="Player-release event has unsupported activation flags";return row;}
                    if(!syncReleaseDeadline(frameCount,frameRate,frame,started,fractionValue,scaleValues,rateValue,tps,deadline)) {row.detail="Player-release event clock or frame is unsupported";return row;}
                    schedules.push_back({leaf,animation,data,md6,event,root,model,frameCount,frameRate,started,deadline,frame,rateValue,fractionValue,{scaleValues[0],scaleValues[1],scaleValues[2]},tps});
                }
                uintptr_t finalEvents=0;int32_t finalEventNum=0;
                if(!memory.get(eventAddress+eventData->offset,finalEvents)||finalEvents!=eventList||!memory.get(eventAddress+eventCount->offset,finalEventNum)||finalEventNum!=eventNum)return row;
            }
            uintptr_t finalInfos=0;int32_t finalInfoNum=0;
            if(!memory.get(infoAddress+infoData->offset,finalInfos)||finalInfos!=infoList||!memory.get(infoAddress+infoCount->offset,finalInfoNum)||finalInfoNum!=infoNum)return row;
        }
        uintptr_t finalTree=0,finalDefinition=0;
        if(!memory.get(states+size_t(model)*stateSize+modelTree->offset,finalTree)||finalTree!=root||!memory.get(decls+size_t(model)*declSize+definition->offset,finalDefinition)||finalDefinition!=md6)return row;
    }
    if(released()){row.state="inactive";row.remaining=0;row.detail="Player control has been released";return row;}
    if(schedules.empty()){row.detail="Active player lock has no verified release event in its current model timelines";return row;}
    const auto& schedule=schedules.front();
    for(const auto& observed:schedules) {
        if(observed.deadline!=schedule.deadline||observed.started!=schedule.started){row.detail="Active models have ambiguous player-release timelines";return row;}
        uintptr_t animation=0,data=0,root=0;int64_t started=0,tps=0;float rateValue=0,fractionValue=0,scaleValues[3]{},frame=0,currentWeight=0;unsigned char disabled=1,mode=1;int16_t number=-1;uint16_t frameCount=0,frameRate=0;uint32_t flags=~0u;
        if(!memory.get(observed.leaf+leafAnim->offset,animation)||animation!=observed.animation||!memory.get(observed.leaf+start->offset,started)||started!=observed.started||
           !memory.get(observed.leaf+rate->offset,rateValue)||rateValue!=observed.rate||!memory.get(observed.leaf+fraction->offset,fractionValue)||fractionValue!=observed.fraction||
           !memory.get(observed.leaf+scales->offset,scaleValues)||memcmp(scaleValues,observed.scales,sizeof(scaleValues))||!memory.get(observed.leaf+ticks->offset,tps)||tps!=observed.ticks||
           !memory.get(observed.event+eventFrame->offset,frame)||frame!=observed.eventFrame||
           !memory.get(observed.event+eventNumber->offset,number)||number!=releaseNumber||!memory.get(observed.event+eventFlags->offset,flags)||flags||
           !syncReadEventDisabled(memory,observed.event,eventDisabled,flags,disabled)||disabled||
           !memory.get(observed.leaf+wrap->offset,mode)||mode||!memory.get(observed.leaf+weight->offset,currentWeight)||!std::isfinite(currentWeight)||currentWeight<=.001f||
           !memory.get(animation+animData->offset,data)||data!=observed.data||!memory.get(data+frames->offset,frameCount)||frameCount!=observed.frames||
           !memory.get(data+fps->offset,frameRate)||frameRate!=observed.fps||!memory.get(states+size_t(observed.model)*stateSize+modelTree->offset,root)||root!=observed.root)return row;
    }
    uintptr_t finalStates=0,finalWeb=0,finalDecls=0;int32_t finalStateNum=0,finalDeclNum=0;
    if(!memory.get(stateAddress+statesData->offset,finalStates)||finalStates!=states||!memory.get(stateAddress+statesCount->offset,finalStateNum)||finalStateNum!=stateNum||
       !memory.get(animatorAddress+web->offset,finalWeb)||finalWeb!=webDecl||!memory.get(webDecl+declarations->offset+declData->offset,finalDecls)||finalDecls!=decls||
       !memory.get(webDecl+declarations->offset+declCount->offset,finalDeclNum)||finalDeclNum!=declNum)return row;
    int64_t finalPause=0;unsigned char finalLocal=1;
    if(!memory.get(sync+stack->offset+pause->offset,finalPause)||finalPause!=pausedAt||!memory.get(sync+stack->offset+local->offset,finalLocal)||finalLocal)return row;
    if(!timerRemaining(schedule.deadline,pausedAt>0?pausedAt:clock.ticks,row.remaining,120))return row;
    row.maximum=double(schedule.deadline-schedule.started)/gameTimerTicksPerSecond;row.state="active";
    row.detail="Until the verified player-release event in the active model timeline";return row;
}

static GameTimerRow captureSyncEntityCurrentClip(const WallMemory& memory,uintptr_t sync,const GameTimerClock& clock,const GraphLookup& lookup) {
    GameTimerRow row{"sync_animation","Sync animations","Sync (current clip)","unavailable","Sync clip metadata unavailable",-1};
    auto working=timerField(lookup,"idSyncEntity","isCurrentlyWorking"),anim=timerField(lookup,"idSyncEntity","currentSyncEntityAnim"),
         stack=timerField(lookup,"idAnimatedEntity","animStack"),tree=timerField(lookup,"idAnimStack","tree"),
         local=timerField(lookup,"idAnimStack","useClientLocalTime"),pause=timerField(lookup,"idAnimStack","pauseTime");
    if(!timerBoolField(working)||!timerPointer(anim,"idMD6Anim")||!timerInline(stack,"idAnimStack",lookup)||
       !timerPointer(tree,"idMD6Node")||!timerBoolField(local)||!timerTimeField(pause)) return row;
    uintptr_t currentAnim=0,root=0; unsigned char on=0,useLocal=0; int64_t pausedAt=0;
    row.detail="Active sync; current clip end time unavailable";
    if(!clock.valid||!memory.get(sync+anim->offset,currentAnim)||!currentAnim||!memory.get(sync+stack->offset+tree->offset,root)||!root||
       !memory.get(sync+stack->offset+local->offset,useLocal)||useLocal||!memory.get(sync+stack->offset+pause->offset,pausedAt)) return row;
    auto nodeType=timerField(lookup,"idMD6Node","type"),left=timerField(lookup,"idMD6Branch","left"),right=timerField(lookup,"idMD6Branch","right"),
         leftOverride=timerField(lookup,"idMD6Branch","leftTimeOverride"),rightOverride=timerField(lookup,"idMD6Branch","rightTimeOverride"),
         leafAnim=timerField(lookup,"idMD6Leaf","anim"),wrap=timerField(lookup,"idMD6Leaf","wrapMode"),weight=timerField(lookup,"idMD6Leaf","lastTotalWeight"),
         start=timerField(lookup,"idMD6LeafPlay","startTime"),scales=timerField(lookup,"idMD6LeafPlay","gameRateScales"),rate=timerField(lookup,"idMD6LeafPlay","rateScale"),
         dataField=timerField(lookup,"idMD6Anim","animData"),framesField=timerField(lookup,"idMD6AnimData","numFrames"),fpsField=timerField(lookup,"idMD6AnimData","frameRate");
    auto byte=[](const GraphField* f){return f&&f->type=="unsigned char"&&f->ops.empty()&&f->size==1;};
    auto scalar=[](const GraphField* f){return f&&f->type=="float"&&f->ops.empty()&&f->size==4;};
    auto ushort=[](const GraphField* f){return f&&f->type=="unsigned short"&&f->ops.empty()&&f->size==2;};
    if(!byte(nodeType)||!timerPointer(left,"idMD6Node")||!timerPointer(right,"idMD6Node")||!timerTimeField(leftOverride)||!timerTimeField(rightOverride)||!timerPointer(leafAnim,"idMD6Anim")||
       !byte(wrap)||!scalar(weight)||!timerTimeField(start)||!scales||scales->ops!=""||scales->type!="idArray < float , 3 >"||scales->size!=12||
       !scalar(rate)||!timerPointer(dataField,"idMD6AnimData")||!ushort(framesField)||!ushort(fpsField)) return row;
    uintptr_t data=0; uint16_t frames=0,fps=0;
    if(!memory.get(currentAnim+dataField->offset,data)||!data||!memory.get(data+framesField->offset,frames)||!memory.get(data+fpsField->offset,fps)) return row;
    std::vector<uintptr_t> pending{root},visited; std::vector<int64_t> ends,durations;
    struct ObservedLeaf { uintptr_t address;int64_t started;unsigned char mode;float rate,scales[3]; };
    std::vector<ObservedLeaf> observed;
    bool complete=true;
    while(!pending.empty()) {
        auto node=pending.back();pending.pop_back();if(!node) continue;
        if(visited.size()>=128||std::find(visited.begin(),visited.end(),node)!=visited.end()) { complete=false; break; } visited.push_back(node);
        unsigned char type=255;
        if(!memory.get(node+nodeType->offset,type)) { complete=false; break; }
        // Reflected node/wrap enums audited on both clock profiles.
        if(type==0||type==3||type==4) {
            uintptr_t a=0,b=0;int64_t leftTime=0,rightTime=0;
            if(!memory.get(node+left->offset,a)||!memory.get(node+right->offset,b)||!memory.get(node+leftOverride->offset,leftTime)||
               !memory.get(node+rightOverride->offset,rightTime)||leftTime>=0||rightTime>=0) { complete=false; break; }
            pending.push_back(a);pending.push_back(b);continue;
        }
        if(type!=2) continue;
        uintptr_t animation=0; unsigned char mode=255; float totalWeight=0,rateValue=0,scaleValues[3]{}; int64_t started=0,endTime=0;
        if(!memory.get(node+leafAnim->offset,animation)) { complete=false; break; }
        if(animation!=currentAnim) continue;
        if(!memory.get(node+wrap->offset,mode)||!memory.get(node+weight->offset,totalWeight)||!std::isfinite(totalWeight)||
           !memory.get(node+start->offset,started)||!memory.get(node+rate->offset,rateValue)||!memory.get(node+scales->offset,scaleValues)) { complete=false; break; }
        if(mode!=0||totalWeight<=.001f) continue;
        if(!timerLeafEnd(frames,fps,started,scaleValues,rateValue,endTime)) { complete=false; break; }
        ObservedLeaf sample{node,started,mode,rateValue,{scaleValues[0],scaleValues[1],scaleValues[2]}};observed.push_back(sample);
        ends.push_back(endTime);durations.push_back(endTime-started);
    }
    for(const auto& sample:observed) {
        uintptr_t animation=0;int64_t started=0;unsigned char mode=255;float currentRate=0,currentScales[3]{},currentWeight=0;
        if(!memory.get(sample.address+leafAnim->offset,animation)||animation!=currentAnim||!memory.get(sample.address+start->offset,started)||started!=sample.started||
           !memory.get(sample.address+wrap->offset,mode)||mode!=sample.mode||!memory.get(sample.address+rate->offset,currentRate)||currentRate!=sample.rate||
           !memory.get(sample.address+scales->offset,currentScales)||memcmp(currentScales,sample.scales,sizeof(currentScales))||
           !memory.get(sample.address+weight->offset,currentWeight)||!std::isfinite(currentWeight)||currentWeight<=.001f)complete=false;
    }
    uintptr_t finalAnim=0,finalRoot=0,finalData=0;int64_t finalPaused=0;unsigned char finalLocal=1;uint16_t finalFrames=0,finalFps=0;
    if(!complete||ends.empty()||!memory.get(sync+anim->offset,finalAnim)||finalAnim!=currentAnim||!memory.get(sync+stack->offset+tree->offset,finalRoot)||finalRoot!=root||
       !memory.get(sync+working->offset,on)||on!=1||!memory.get(sync+stack->offset+pause->offset,finalPaused)||finalPaused!=pausedAt||
       !memory.get(sync+stack->offset+local->offset,finalLocal)||finalLocal!=useLocal||!memory.get(currentAnim+dataField->offset,finalData)||finalData!=data||
       !memory.get(data+framesField->offset,finalFrames)||finalFrames!=frames||!memory.get(data+fpsField->offset,finalFps)||finalFps!=fps) return row;
    auto bounds=std::minmax_element(ends.begin(),ends.end());
    if(*bounds.second-*bounds.first>int64_t(gameTimerTicksPerSecond*.05)) { row.detail="Active sync has overlapping clip timelines"; return row; }
    int64_t now=pausedAt>0?pausedAt:clock.ticks;
    if(!timerRemaining(*bounds.second,now,row.remaining,120)) return row;
    auto durationBounds=std::minmax_element(durations.begin(),durations.end());
    if(*durationBounds.second-*durationBounds.first<=int64_t(gameTimerTicksPerSecond*.05))timerSetMaximum(row,double(*durationBounds.second)/gameTimerTicksPerSecond);
    row.state="active"; row.detail="Current clamped sync clip: "+timerResourceName(memory,currentAnim,lookup)+(pausedAt>0?" (paused)":""); return row;
}

// Player-owned syncs can be started by interaction components without entering
// the melee mechanic's reference list. Keep each actual active sync separate;
// reusing one glory-kill countdown for a button would mislabel unrelated work.
static std::vector<GameTimerRow> captureSyncAnimationTimers(const WallMemory& memory,uintptr_t player,const GameTimerClock& clock,const GraphLookup& lookup,uintptr_t base=0,const GameProfile* profile=nullptr) {
    enum Source : unsigned { Player=1,Melee=2,Mechanic=4,Interactable=8 };
    struct Reference { uintptr_t address=0,entity=0;uint32_t token=0,ptrOffset=0,tokenOffset=0;unsigned source=0; };
    struct Candidate { uintptr_t entity=0;unsigned sources=0; };
    std::vector<Reference> references;std::vector<Candidate> candidates;
    auto managed=[&](const GraphField* field,const char* target)->bool {
        const std::string type=std::string("idManagedClassPtr < ")+target+" >";
        if(!timerInline(field,type,lookup))return false;
        const auto* ptr=timerField(lookup,type,"ptr");const auto* token=timerField(lookup,type,"spawnId");
        return timerPointer(ptr,target)&&token&&token->size==4&&token->ops.empty()&&token->type=="idSpawnId";
    };
    auto add=[&](uintptr_t address,const std::string& type,unsigned source)->bool {
        const auto* ptr=timerField(lookup,type,"ptr");const auto* token=timerField(lookup,type,"spawnId");
        if(!ptr||!token)return false;
        Reference ref;ref.address=address;ref.ptrOffset=ptr->offset;ref.tokenOffset=token->offset;ref.source=source;
        if(!memory.get(address+ref.ptrOffset,ref.entity)||!memory.get(address+ref.tokenOffset,ref.token))return false;
        references.push_back(ref);
        if(ref.entity&&source) {
            auto found=std::find_if(candidates.begin(),candidates.end(),[&](const auto& item){return item.entity==ref.entity;});
            if(found==candidates.end())candidates.push_back({ref.entity,source});else found->sources|=source;
        }
        return true;
    };
    auto unavailable=[](const char* detail) {
        return std::vector<GameTimerRow>{{"sync_animation","Sync animations","Sync (current clip)","unavailable",detail,-1}};
    };
    auto master=timerField(lookup,"idPlayer","syncMaster"),melee=timerField(lookup,"idPlayer","syncMelee"),
         current=timerField(lookup,"idSyncAttack","currentSyncEntity"),mechanic=timerField(lookup,"idPlayer","playerMechanicSync"),
         listField=timerField(lookup,"idPlayerMechanicSync","syncAnimEntities");
    const std::string syncType="idManagedClassPtr < idSyncEntity >",listType="idList < idManagedClassPtr < idSyncEntity > , TAG_IDLIST , false >";
    const auto& managedSync=lookup(syncType);
    auto data=timerField(lookup,listType,"list"),num=timerField(lookup,listType,"num");
    if(!managed(master,"idSyncEntity")||!timerInline(melee,"idSyncAttack",lookup)||!managed(current,"idSyncEntity")||
       !timerInline(mechanic,"idPlayerMechanicSync",lookup)||!timerInline(listField,listType,lookup)||
       !data||data->size!=8||data->ops!="*"||data->type!=syncType||!timerIntField(num)||
       !add(player+master->offset,syncType,Player)||!add(player+melee->offset+current->offset,syncType,Melee))
        return unavailable("Player sync reference metadata unavailable");
    uintptr_t listAddress=player+mechanic->offset+listField->offset,list=0;int32_t count=0;
    if(!memory.get(listAddress+data->offset,list)||!memory.get(listAddress+num->offset,count)||count<0||count>256||(!list&&count))
        return unavailable("Player sync reference list unavailable");
    for(int32_t index=0;index<count;++index)if(!add(list+size_t(index)*managedSync.size,syncType,Mechanic))return unavailable("Player sync reference changed");
    for(const char* name:{"savedSyncEntity","mostRecentSyncEntity"}) {
        const auto* field=timerField(lookup,"idPlayer",name);
        if(managed(field,"idSyncEntity")&&!add(player+field->offset,syncType,Player))return unavailable("Additional player sync reference unavailable");
    }
    bool interactionSourceKnown=false;
    const auto* last=timerField(lookup,"idPlayer","lastUsedInteractable");
    auto component=timerField(lookup,"idInteractable","interaction"),active=timerField(lookup,"idInteractionComponent","activeSyncEntity"),
         activator=timerField(lookup,"idInteractionComponent","playerActivator"),owner=timerField(lookup,"idInteractionComponent","ownerEntity");
    if(managed(last,"idInteractable")&&timerInline(component,"idInteractionComponent",lookup)&&managed(active,"idSyncEntity")&&
       managed(activator,"idEntity")&&managed(owner,"idAnimatedEntity")) {
        const std::string lastType="idManagedClassPtr < idInteractable >";
        if(!add(player+last->offset,lastType,0))return unavailable("Last interactable reference unavailable");
        const auto target=references.back().entity;interactionSourceKnown=true;
        if(target) {
            unsigned char removed=1;
            if(!memory.get(target+0xa4,removed)||removed>1)return unavailable("Last interactable identity unavailable");
            if(!removed) {
                const auto at=target+component->offset;
                if(!add(at+activator->offset,activator->type,0)||!add(at+owner->offset,owner->type,0))return unavailable("Interaction owner reference unavailable");
                const auto actualOwner=references.back().entity,actualPlayer=references[references.size()-2].entity;
                // The component is inline in this interactable. A stale lastUsed
                // reference or another player's component cannot supply a timer.
                if(actualOwner==target&&actualPlayer==player&&!add(at+active->offset,syncType,Interactable))
                    return unavailable("Interaction sync reference unavailable");
            }
        }
    }
    auto stable=[&] {
        uintptr_t finalList=0;int32_t finalCount=0;
        if(!memory.get(listAddress+data->offset,finalList)||finalList!=list||!memory.get(listAddress+num->offset,finalCount)||finalCount!=count)return false;
        for(const auto& ref:references) {
            uintptr_t entity=0;uint32_t token=0;
            if(!memory.get(ref.address+ref.ptrOffset,entity)||entity!=ref.entity||!memory.get(ref.address+ref.tokenOffset,token)||token!=ref.token)return false;
        }
        return true;
    };
    auto working=timerField(lookup,"idSyncEntity","isCurrentlyWorking"),participant=timerField(lookup,"idSyncEntity","syncPlayer"),
         interactable=timerField(lookup,"idSyncEntity","isInteractableSync");
    if(!timerBoolField(working)||!managed(participant,"idPlayer"))return unavailable("Active sync ownership metadata unavailable");
    const auto* playerPointer=timerField(lookup,participant->type,"ptr");
    std::vector<GameTimerRow> result;unsigned meleeCount=0,interactionCount=0,genericCount=0;
    for(const auto& candidate:candidates) {
        unsigned char removed=1,on=0;uintptr_t syncPlayer=0;
        if(!memory.get(candidate.entity+0xa4,removed)||removed>1||!memory.get(candidate.entity+working->offset,on)||on>1)
            return unavailable("Active sync identity unavailable");
        if(removed||!on)continue;
        if(!memory.get(candidate.entity+participant->offset+playerPointer->offset,syncPlayer))return unavailable("Active sync player unavailable");
        if(syncPlayer!=player)continue;
        unsigned char isInteraction=0;
        if(timerBoolField(interactable)&&(!memory.get(candidate.entity+interactable->offset,isInteraction)||isInteraction>1))return unavailable("Sync interaction state unavailable");
        auto row=captureSyncEntityCurrentClip(memory,candidate.entity,clock,lookup);
        if(candidate.sources&Interactable||isInteraction) {
            // A player-owned interaction can release control before the NPC
            // clip ends (tokens, Gore Nests, buttons, and other authored syncs).
            // Use its actual release event, without an asset-name whitelist.
            const bool playerLock=syncPlayerLockProfile(profile);
            if(playerLock)row=captureSyncPlayerControlLock(memory,player,candidate.entity,clock,lookup,syncReleaseEventNumber(memory,base,*profile,lookup),base,profile);
            row.key=interactionCount++?"interaction_"+std::to_string(candidate.entity):"interaction";
            row.label=playerLock?"Player lock":"Interaction (current clip)";
        } else if(candidate.sources&Melee) {
            row.key=meleeCount++?"sync_animation_"+std::to_string(candidate.entity):"sync_animation";
            row.label="Glory kill / chainsaw (current clip)";
        } else {
            const bool playerLock=syncPlayerLockProfile(profile);
            if(playerLock)row=captureSyncPlayerControlLock(memory,player,candidate.entity,clock,lookup,syncReleaseEventNumber(memory,base,*profile,lookup),base,profile);
            row.key="sync_other_"+std::to_string(++genericCount);row.label=playerLock?"Player lock":"Scripted sync (current clip)";
        }
        if(!memory.get(candidate.entity+participant->offset+playerPointer->offset,syncPlayer)||syncPlayer!=player||
           !memory.get(candidate.entity+working->offset,on)||on!=1||!memory.get(candidate.entity+0xa4,removed)||removed)
            return unavailable("Sync finished or changed owner during capture");
        result.push_back(std::move(row));
    }
    if(!stable())return unavailable("Sync reference generation changed during capture");
    if(!meleeCount)result.push_back({"sync_animation","Sync animations","Glory kill / chainsaw (current clip)","inactive","No active player melee sync",0});
    if(!interactionCount)result.push_back({"interaction","Sync animations","Interaction (current clip)",interactionSourceKnown?"inactive":"unavailable",
        interactionSourceKnown?"No active player interaction sync":"Interactable sync source metadata unavailable",interactionSourceKnown?0.:-1.});
    return result;
}
