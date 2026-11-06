#pragma once

static SyncPlayerFootprint readSyncPlayerFootprint(const WallMemory& memory,uintptr_t player,uintptr_t map) {
    SyncPlayerFootprint result;uintptr_t physics=0,owner=0,afterPhysics=0;uint32_t type=0;int index=-1,afterIndex=-1;
    Vec3 origin{},afterOrigin{};float axis[9]{},local[6]{},world[6]{},afterWorld[6]{};
    if(!memory.get(player+0x1d0,physics)||physics!=player+0x7d28||!memory.get(physics+0x70,owner)||owner!=map||
       !memory.get(physics+0x14,type)||type!=5||!memory.get(physics+0x3cc0,index)||index<0||index>8||
       !memory.get(physics+0xb0,origin)||!syncRangeFinite(origin)||!memory.read(physics+0x8c,axis,36)||!syncFacingOrthonormal(axis)||
       !memory.read(physics+0xc8,local,24)||!memory.read(physics+0xe0,world,24)||!syncAimBoundsValid(local)||!syncAimBoundsValid(world))return result;
    float check[6]{FLT_MAX,FLT_MAX,FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX};SyncFacingPose pose;memcpy(pose.axis,axis,36);pose.origin=origin;
    for(unsigned i=0;i<8;++i){const auto p=motionAdd(origin,syncRangeTransform(pose,{local[(i&1)?3:0],local[(i&2)?4:1],local[(i&4)?5:2]}));const float xyz[]{p.x,p.y,p.z};
        for(unsigned k=0;k<3;++k){check[k]=std::min(check[k],xyz[k]);check[k+3]=std::max(check[k+3],xyz[k]);}}
    for(unsigned i=0;i<6;++i)if(fabsf(check[i]-world[i])>.025f)return result;
    if(world[3]-world[0]>8||world[4]-world[1]>8||world[5]-world[2]>10||
       !memory.get(player+0x1d0,afterPhysics)||afterPhysics!=physics||!memory.get(physics+0x3cc0,afterIndex)||afterIndex!=index||
       !memory.get(physics+0xb0,afterOrigin)||memcmp(&origin,&afterOrigin,12)||!memory.read(physics+0xe0,afterWorld,24)||memcmp(world,afterWorld,24))return result;
    result.known=true;result.minimum={world[0]-origin.x,world[1]-origin.y,0};result.maximum={world[3]-origin.x,world[4]-origin.y,0};return result;
}
static bool syncContextInt(const WallMemory& memory,uintptr_t at,const char* name,int& value) {
    uintptr_t object=0,text=0;return memory.get(at,object)&&object&&memory.get(object+40,text)&&readEntityText(memory,text,128)==name&&memory.get(object+8,value);
}
// A visual aim aid only: do not feed a shot-sphere hit into the native focus
// gate below. History, cached contacts and fallback selection stay separate.
static bool readSyncSphereFocusGroups(const WallMemory& memory,uintptr_t base,const EntityReference& target,std::vector<SyncHitSphere>& spheres) {
    for(auto& sphere:spheres){sphere.focusKnown=false;sphere.focusGroup.clear();}
    const unsigned char expected[]{0x48,0x63,0x44,0xd1,0x08};unsigned char code[5]{};
    uintptr_t animator=0,decl=0,afterAnimator=0,afterDecl=0;SyncPreviewList groups,afterGroups;
    if(!memory.read(base+0x5d721d,code,5)||memcmp(code,expected,5)||!memory.get(target.address+0x1a20,animator)||!animator||
       !memory.get(animator,decl)||!decl||!syncPreviewList(memory,decl+0xe0,128,groups))return false;
    struct Group {uintptr_t address=0;SyncPreviewList joints;std::vector<short> ids;std::string name;};
    std::vector<uintptr_t> pointers(groups.count),afterPointers(groups.count);std::vector<Group> data;
    if(groups.count&&!memory.read(groups.data,pointers.data(),pointers.size()*8))return false;
    for(auto address:pointers){Group group;group.address=address;
        if(!address||!syncPreviewList(memory,address+0x10,512,group.joints)||!syncPreviewString(memory,address+8,group.name))return false;
        group.ids.resize(group.joints.count);if(!group.ids.empty()&&!memory.read(group.joints.data,group.ids.data(),group.ids.size()*2))return false;
        data.push_back(std::move(group));}
    for(const auto& group:data){SyncPreviewList after;std::vector<short> ids(group.ids.size());std::string name;
        if(!memory.get(group.address+0x10,after)||memcmp(&after,&group.joints,24)||
           (!ids.empty()&&!memory.read(group.joints.data,ids.data(),ids.size()*2))||ids!=group.ids||
           !syncPreviewString(memory,group.address+8,name)||name!=group.name)return false;}
    if(!memory.get(target.address+0x1a20,afterAnimator)||animator!=afterAnimator||!memory.get(animator,afterDecl)||decl!=afterDecl||
       !memory.get(decl+0xe0,afterGroups)||memcmp(&groups,&afterGroups,24)||
       (!pointers.empty()&&!memory.read(groups.data,afterPointers.data(),afterPointers.size()*8))||pointers!=afterPointers)return false;
    for(auto& sphere:spheres){sphere.focusKnown=true;
        for(const auto& group:data)if(std::find(group.ids.begin(),group.ids.end(),sphere.jointIndex)!=group.ids.end()){sphere.focusGroup=group.name;break;}}
    return true;
}
// Exact supported 1.1 paths: native cached focus contact, or the history
// selector when explicitly enabled. Never replace them with the first hurt-
// sphere hit. The default fallback geometric joint search remains unknown.
static SyncJointFocus readSyncJointFocus(const WallMemory& memory,uintptr_t base,uintptr_t player,const EntityReference& target) {
    SyncJointFocus result;int historyMode=-1,semiRandom=-1,joint=-1;unsigned char contact[76]{},afterContact[76]{};
    const unsigned char expected[]{0x48,0x63,0x44,0xd1,0x08};unsigned char code[5]{};
    if(!memory.read(base+0x5d721d,code,5)||memcmp(code,expected,5)||
       !syncContextInt(memory,base+0x61a9db0,"sync_UseFocusHistory",historyMode))return result;
    SyncPreviewList history,afterHistory;std::vector<int> historyRows;
    if(historyMode==1){int count=0;if(!syncContextInt(memory,base+0x61a9d30,"player_focusMaxHistory",count)||count<1||count>128||
            !syncPreviewList(memory,player+0x7768,512,history)||history.count<count)return result;
        historyRows.resize(count);if(!memory.read(history.data,historyRows.data(),count*4))return result;
        joint=historyRows[0];int best=1;
        for(int i=0;i<count-1;++i){int matches=0;for(int j=1;j<count;++j)matches+=historyRows[j]==historyRows[i];if(matches>best){best=matches;joint=historyRows[i];}}
    } else if(historyMode==0){if(!syncContextInt(memory,base+0x61a9cb0,"sync_semiRandom",semiRandom)||semiRandom!=0||!memory.read(player+0x15c2c,contact,76))return result;
        int type=0,model=-1;uint32_t token=0;memcpy(&type,contact,4);memcpy(&model,contact+48,4);memcpy(&joint,contact+52,4);memcpy(&token,contact+56,4);
        if(type!=4||token!=target.id||model<0||(uint16_t(model)==0xffff))return result;
    } else return result;
    if(joint<0||joint>32767)return result;
    uintptr_t animator=0,decl=0,afterAnimator=0,afterDecl=0;SyncPreviewList groups,afterGroups;
    if(!memory.get(target.address+0x1a20,animator)||!animator||!memory.get(animator,decl)||!decl||!syncPreviewList(memory,decl+0xe0,128,groups))return result;
    for(int i=0;i<groups.count;++i){uintptr_t group=0;SyncPreviewList joints,afterJoints;
        if(!memory.get(groups.data+i*8,group)||!group||!syncPreviewList(memory,group+0x10,512,joints))return {};
        std::vector<short> rows(joints.count);if(joints.count&&!memory.read(joints.data,rows.data(),rows.size()*2))return {};
        if(std::find(rows.begin(),rows.end(),short(joint))!=rows.end()){
            if(!syncPreviewString(memory,group+8,result.group)||!memory.get(group+0x10,afterJoints)||memcmp(&joints,&afterJoints,24))return {};break;}}
    if(!memory.get(target.address+0x1a20,afterAnimator)||afterAnimator!=animator||!memory.get(animator,afterDecl)||afterDecl!=decl||
       !memory.get(decl+0xe0,afterGroups)||memcmp(&groups,&afterGroups,24))return {};
    if(historyMode){std::vector<int> after(historyRows.size());if(!memory.get(player+0x7768,afterHistory)||memcmp(&history,&afterHistory,24)||
            !memory.read(history.data,after.data(),after.size()*4)||after!=historyRows)return {};}
    else if(!memory.read(player+0x15c2c,afterContact,76)||memcmp(contact,afterContact,76))return {};
    int afterMode=-1;if(!syncContextInt(memory,base+0x61a9db0,"sync_UseFocusHistory",afterMode)||afterMode!=historyMode)return {};
    if(!historyMode){int afterRandom=-1;if(!syncContextInt(memory,base+0x61a9cb0,"sync_semiRandom",afterRandom)||afterRandom!=semiRandom)return {};}
    result.known=true;return result;
}
