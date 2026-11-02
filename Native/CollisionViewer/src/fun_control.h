#pragma once

static bool funDemonPlayerClass(const std::string& type) {
    return type=="idDemonPlayer"||type.rfind("idDemonPlayer_",0)==0;
}

static bool funControlledDemon(const WallMemory& memory,uintptr_t player,const InspectorField& field,uintptr_t base,const GameProfile& profile) {
    if(!player||!field||field.size!=152||field.type.find("idStaticList < idManagedClassPtr < idEntity >")!=0) return false;
    struct List {uintptr_t data;uint32_t count,capacity;} list{};
    const auto at=player+field.offset;
    if(!memory.get(at,list)||list.count>4||list.capacity!=4||list.data!=at+24) return false;
    for(uint32_t i=0;i<list.count;++i) {
        uintptr_t entity=0,vtable=0;
        if(memory.get(list.data+size_t(i)*32+8,entity)&&entity&&memory.get(entity,vtable)&&
            funDemonPlayerClass(readEntityClass(memory,vtable,base,profile))) return true;
    }
    return false;
}
