#pragma once
struct PlayerPoseLayout {
    uint32_t physics=~0u,position=~0u,angles=~0u;
};
static PlayerPoseLayout resolvePlayerPoseLayout(const std::function<InspectorField(const std::string&,const char*)>& lookup) {
    PlayerPoseLayout result;
    auto physics=lookup("idEntity","physics"),current=lookup("idHavokPhysics_Player","current"),position=lookup("playerPState_t","worldOrigin"),angles=lookup("idHavokPhysics_Player","viewAngles");
    if(physics&&physics.type=="idPhysics*"&&physics.size==8)result.physics=physics.offset;
    if(current&&current.type=="playerPState_t"&&position&&position.type=="idVec3"&&position.size==12&&uint64_t(position.offset)+12<=current.size)
        result.position=current.offset+position.offset;
    if(angles&&angles.type=="idAngles"&&angles.size==12)result.angles=angles.offset;
    return result;
}
struct PlayerPoseValue { Vec3 position{},angles{};bool positionKnown=false,anglesKnown=false; };
static PlayerPoseValue readPlayerPose(const WallMemory& memory,uintptr_t player,const PlayerPoseLayout& layout) {
    PlayerPoseValue result;uintptr_t physics=0,final=0;unsigned char removed=1;
    if(layout.physics==~0u||!memory.get(player+0xa4,removed)||removed||!memory.get(player+layout.physics,physics)||!physics)return result;
    auto vector=[&](uint32_t offset,Vec3& value){return offset!=~0u&&memory.get(physics+offset,value)&&std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z)&&
        fabsf(value.x)<1000000&&fabsf(value.y)<1000000&&fabsf(value.z)<1000000;};
    result.positionKnown=vector(layout.position,result.position);result.anglesKnown=vector(layout.angles,result.angles);
    if(!memory.get(player+layout.physics,final)||final!=physics||!memory.get(player+0xa4,removed)||removed)return {};
    return result;
}
static std::vector<std::string> playerPoseLines(const PlayerPoseValue& value,bool position,bool angles) {
    std::vector<std::string> lines;
    auto line=[](Vec3 v,const char* title,const char* x,const char* y,const char* z,unsigned decimals){
        std::ostringstream out;out.imbue(std::locale::classic());out.setf(std::ios::fixed);out.precision(decimals);
        out<<title<<x<<v.x<<"  "<<y<<v.y<<"  "<<z<<v.z;return out.str();};
    if(position&&value.positionKnown)lines.push_back(line(value.position,"Position: ","X ","Y ","Z ",3));
    if(angles&&value.anglesKnown)lines.push_back(line(value.angles,"Angles (deg): ","Pitch ","Yaw ","Roll ",2));
    if(!lines.empty())lines.insert(lines.begin(),"LAB Plus | Player position / angles");
    return lines;
}
class PlayerPoseHud {
    PlayerPoseLayout layout;ULONGLONG retryAt=0;bool requested=false;
public:
    void clear(){debugHudStack().clear(DebugHudBlock::PlayerPose);requested=false;}
    void frame(const MeathookHudApi* api,const GameProfile* profile) {
        if(!profile||(!showPlayerPositionHud.load()&&!showPlayerAnglesHud.load())){clear();return;}
        const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));const WallMemory memory;
        auto level=readLevelIdentity(memory,base,*profile);
        if(!level||!gameplayVisible(memory,base,*profile)||readConsoleFraction(memory,base,*profile)>0){clear();return;}
        if((layout.physics==~0u||layout.position==~0u||layout.angles==~0u)&&GetTickCount64()>=retryAt){
            retryAt=GetTickCount64()+2000;layout=resolvePlayerPoseLayout([&](const std::string& type,const char* field){return inspectorField(*api,type,field);});
        }
        uintptr_t player=0,vtable=0;
        if(!memory.get(level.map+profile->mapPlayerOffset,player)||!memory.get(player,vtable)||vtable!=base+profile->playerVtableRva){clear();return;}
        auto value=readPlayerPose(memory,player,layout);
        if(readLevelIdentity(memory,base,*profile)!=level){clear();return;}
        auto lines=playerPoseLines(value,showPlayerPositionHud.load(),showPlayerAnglesHud.load());
        if(lines.empty()){clear();return;}
        if(!requested&&api->executeCommand){api->executeCommand("com_debugHUD 1");requested=true;}
        debugHudStack().publish(DebugHudBlock::PlayerPose,api,lines,16,.72f,0xffffd040u);
    }
};
