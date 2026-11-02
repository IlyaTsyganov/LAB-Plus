#include "../src/vulkan_layer.cpp"
static void expect(bool value,const char* message) { if(!value) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
struct FieldFixture { const char* type; const char* ops; const char* name; int offset,size; unsigned char tail[40]{}; };
static_assert(sizeof(FieldFixture)==72);
struct TypeFixture { const char* name; const char* parent; uint64_t size; uint64_t pad; FieldFixture* rows; };
static std::unordered_map<std::string,TypeFixture*> fixtureTypes;
static void* findFixtureClass(const char* type) { auto found=fixtureTypes.find(type); return found==fixtureTypes.end()?nullptr:found->second; }
int main() {
    expect(funDemonPlayerClass("idDemonPlayer_Revenant")&&funDemonPlayerClass("idDemonPlayer"),"a separate demon player entity is recognized as demon control");
    expect(!funDemonPlayerClass("idPlayer")&&!funDemonPlayerClass("idAnimCamera")&&!funDemonPlayerClass("idDemonPlayerCamera"),"Slayer and cinematic camera entities do not trigger demon suspension");
    GameCamera cutCamera{};cutCamera.position[0]=2345.064208984375f;cutCamera.position[1]=-1524.0955810546875f;cutCamera.position[2]=-75.0849380493164f;
    float playerPov[3]{36.0003662109375f,-1604.0003662109375f,19.15818214416504f};
    expect(!funPlayerPov(true,true,&cutCamera,playerPov),"captured 1.1 external cutscene overrides stale first-person ownership flag");
    memcpy(cutCamera.position,playerPov,12);expect(funPlayerPov(true,true,&cutCamera,playerPov),"cinematic at the player's actual POV retains full-screen colour and unmuffled audio");
    cutCamera.position[0]+=.1f;expect(funPlayerPov(true,true,&cutCamera,playerPov),"small first-person cinematic camera shake is tolerated");
    expect(funPlayerPov(true,false,nullptr,nullptr),"ordinary first-person gameplay does not depend on cinematic camera sampling");
    expect(!funPlayerPov(false,false,&cutCamera,playerPov),"explicit third-person view stays outside");
    expect(!funPlayerPov(true,true,nullptr,playerPov)&&!funPlayerPov(true,true,&cutCamera,nullptr),"unknown cinematic camera withholds screen colour and muffles audio");
    const float animatedOrigin[]{61.0099983215332f,-1422.75f,17.020000457763672f},animatedAxis[]{1,0,0,0,1,0,0,0,1},animatedScale[]{1,1,1};
    const float povJoint[]{.2072606087f,-.9782857299f,.0002468429f,-.3189809024f,.9782763124f,.2072597146f,.0043475963f,6.2633652687f,-.0043043522f,-.0006596049f,.9999905229f,1.6214500666f};
    GameCamera animatedCamera{};animatedCamera.position[0]=60.691017150878906f;animatedCamera.position[1]=-1416.4866943359375f;animatedCamera.position[2]=18.641450881958008f;
    float dormantPlayerPov[]{61,-1425.800048828125f,21.88555145263672f};
    expect(funJointCameraMatch(animatedCamera,animatedOrigin,animatedAxis,animatedScale,povJoint),"captured POV camera follows the animated Slayer pose to sub-millimetre precision");
    expect(funPlayerPov(true,true,&animatedCamera,dormantPlayerPov,funAnimatedView(true,false,false)),"animated POV remains clear even when the dormant player origin is ten metres away");
    const float externalOrigin[]{2132.219970703125f,-1135.5999755859375f,3.5f};
    const float externalCameraJoint[]{-.0599437654f,-.9982016087f,.0005938094f,-.6743491292f,.9971315861f,-.0599070489f,-.0462584756f,-1.6876643896f,.0462108552f,-.0021807998f,.9989293218f,1.302094698f};
    GameCamera externalClose{};externalClose.position[0]=2131.5458984375f;externalClose.position[1]=-1137.287841796875f;externalClose.position[2]=4.801868438720703f;
    expect(funJointCameraMatch(externalClose,externalOrigin,animatedAxis,animatedScale,externalCameraJoint),"recorded third-person animation also contains a matching camera helper joint");
    const auto externalView=funAnimatedView(true,true,false);
    expect(externalView==FunAnimatedView::External&&!funPlayerPov(true,true,&externalClose,playerPov,externalView),"rendered cinematic actor prevents the camera helper from falsely selecting POV");
    expect(!funPlayerPov(true,true,&externalClose,externalClose.position,externalView),"explicit external-view evidence overrides a stale matching player-origin fallback");
    expect(funActorCommittedWithRig(0x5268,0x23b77),"captured second shot accepts the actor rendered after its camera helper stopped rendering");
    expect(funActorCommittedWithRig(0x21699,0x21699),"same-frame cinematic actor remains current");
    expect(!funActorCommittedWithRig(0x23b77,0x5268),"an older inactive actor cannot override a newly rendered POV rig");
    expect(funActorCommittedWithRig(100,98)&&!funActorCommittedWithRig(100,97),"only bounded renderer queue skew is accepted for older actor commits");
    expect(funActorCommittedWithRig(0xfffffffeu,1)&&!funActorCommittedWithRig(1,0xfffffffeu),"commit ordering survives frame counter wrap");
    expect(!funActorCommittedWithRig(~0u,100)&&!funActorCommittedWithRig(100,~0u),"unsubmitted models never count as current render evidence");
    const std::string shotPrefix="cineractive/e1m1_intro/khan_maykr_priest_meeting/khan_maykr_priest_meeting_s";
    expect(funDirectedShot(shotPrefix+"010_camera")==FunAnimatedView::PlayerPov&&funDirectedShot(shotPrefix+"030_camera")==FunAnimatedView::PlayerPov,"directed first-level POV shots retain screen colour and clear music");
    for(auto shot:{"020","035","040","050","155","160"}) expect(funDirectedShot(shotPrefix+shot+"_camera")==FunAnimatedView::External,"directed external shots override a matching camera helper");
    expect(funDirectedShot("cineractive/other_level/camera_s035_camera")==FunAnimatedView::Unknown&&funDirectedShot(shotPrefix+"x35_camera")==FunAnimatedView::Unknown&&funDirectedShot(shotPrefix+"035_camera_extra")==FunAnimatedView::Unknown,"shot rules cannot spill into another scene or malformed clip names");
    const std::string sentinelPrefix="cineractive/e1m2_battle/sentinel_king/sentinel_king_s";
    for(auto shot:{"030","040","050","055","060","070","090","100","105","110","115","120","130","135"})
        expect(funDirectedShot(sentinelPrefix+shot+"_camera")==FunAnimatedView::External,"all recorded Sentinel King outside shots use the corrected third-person labels");
    expect(funDirectedShot(sentinelPrefix+"080_camera")==FunAnimatedView::PlayerPov&&funDirectedShot(sentinelPrefix+"140_camera")==FunAnimatedView::PlayerPov,"Sentinel King has two labelled POV regions including its final shot");
    expect(funDirectedShot(sentinelPrefix+"080_camera_extra")==FunAnimatedView::Unknown&&funDirectedShot("cineractive/other_level/sentinel_king_s080_camera")==FunAnimatedView::Unknown,"Sentinel King rules are limited to its exact authored clip path");
    const std::string mechAnimation="cineractive/e1m2_battle/mech_hand_cannon/mech_hand_cannon_camera";
    expect(funDirectedShot(mechAnimation,0)==FunAnimatedView::PlayerPov&&funDirectedShot(mechAnimation,1)==FunAnimatedView::External,"mech hand cannon changes from POV to third person within the same authored clip");
    expect(funDirectedShot(mechAnimation)==FunAnimatedView::Unknown&&funDirectedShot(mechAnimation,2)==FunAnimatedView::Unknown,"unknown internal cut values do not invent a camera mode");
    const std::string spearAnimation="cineractive/e1m2_battle/mech_spear/mech_spear_camera";
    expect(funDirectedShot(spearAnimation,0)==FunAnimatedView::PlayerPov&&funDirectedShot(spearAnimation,1)==FunAnimatedView::External&&funDirectedShot(spearAnimation,0)==FunAnimatedView::PlayerPov,"recorded mech spear sequence follows internal cut values 0-1-0 as POV-external-POV");
    const std::string shoulderAnimation="cineractive/e1m2_battle/mech_shoulder_cannon/mech_shoulder_cannon_camera";
    expect(funDirectedShot(shoulderAnimation,0)==FunAnimatedView::PlayerPov&&funDirectedShot(shoulderAnimation,1)==FunAnimatedView::External&&funDirectedShot(shoulderAnimation,0)==FunAnimatedView::PlayerPov,"recorded shoulder cannon sequence follows the labelled POV-external-POV cuts");
    const std::string survivorPrefix="cineractive/e1m2_battle/sentinel_survivor/sentinel_survivor_s";
    const std::array<const char*,14> survivorShots{"010","030","040","050","060","070","080","090","100","110","120","130","140","150"};
    for(size_t i=0;i<survivorShots.size();++i)
        expect(funDirectedShot(survivorPrefix+survivorShots[i]+"_camera")==((i==0||i==13)?FunAnimatedView::PlayerPov:FunAnimatedView::External),"complete captured survivor sequence has POV endpoints and twelve external shots");
    const std::string possessionPrefix="cineractive/e1m3_cult/revenant_possesion/revenant_possesion_s";
    const std::string shotgunPrefix="cineractive/e1m3_cult/revenant_gives_shotgun/revenant_gives_shotgun_s";
    expect(funDirectedShot(possessionPrefix+"010_camera")==FunAnimatedView::PlayerPov&&funDirectedShot(possessionPrefix+"020_camera")==FunAnimatedView::External,"validated Revenant possession intro keeps the user's first-to-third-person sequence");
    expect(funDirectedShot(shotgunPrefix+"010_camera")==FunAnimatedView::External&&funDirectedShot(shotgunPrefix+"015_camera")==FunAnimatedView::External&&funDirectedShot(shotgunPrefix+"020_camera")==FunAnimatedView::PlayerPov,"Revenant shotgun return stays external through its short transition clip then returns to POV");
    const std::string hunterPrefix="cineractive/e1m4_boss/doom_hunter_intro/doom_hunter_intro_s";
    const std::string secondPriestPrefix="cineractive/e1m4_boss/second_priest_death/second_priest_death_s";
    expect(funDirectedShot(secondPriestPrefix+"010_camera")==FunAnimatedView::PlayerPov&&funDirectedShot(secondPriestPrefix+"030_camera")==FunAnimatedView::External,"recorded second priest death scene follows the user's first-to-third-person sequence");
    const std::string heartLeftPrefix="cineractive/e2m1_nest/heart_destruction_left/heart_destruction_left_s";
    expect(funDirectedShot(heartLeftPrefix+"010_camera")==FunAnimatedView::PlayerPov&&funDirectedShot(heartLeftPrefix+"020_camera")==FunAnimatedView::External&&funDirectedShot(heartLeftPrefix+"030_camera")==FunAnimatedView::PlayerPov,"recorded left-heart cinematic follows POV-external-POV");
    expect(funDirectedShot("cineractive/e2m1_nest/heart_destruction_center/heart_destruction_center_locked_camera")==FunAnimatedView::Unknown,"the left-heart rule cannot change the validated centre locked-camera scene");
    const std::string heartRightPrefix="cineractive/e2m1_nest/heart_destruction_right/heart_destruction_right_s";
    const std::string bridgeAnimation="cineractive/e2m2_base/crossing_bridge/crossing_bridge_camera";
    const std::string turretOnePrefix="cineractive/e2m2_base/destroy_turrets_1/destroy_turrets_1_s";
    const std::string turretTwoPrefix="cineractive/e2m2_base/destroy_turrets_2/destroy_turrets_2_s";
    const std::string bfgPrefix="cineractive/e2m3_core/bfg10k_firing/bfg10k_firing_s";
    const std::string cannonControlAnimation="cineractive/e2m3_core/human_cannonball_control_room/human_cannonball_control_room_s010_camera";
    const std::string powerCruciblePrefix="cineractive/e3m1_slayer/power_crucible/power_crucible_s";
    const std::string vegaRingPrefix="cineractive/e3m3_maykr/vega_console_ring/vega_console_ring_s";
    const std::string purpleRingPrefix="cineractive/e3m3_maykr/purple_console_ring/purple_console_ring_s";
    const std::string yellowRingAnimation="cineractive/e3m3_maykr/yellow_console_ring/yellow_console_ring_s010_camera";
    const std::string ringPortalAnimation="cineractive/e3m3_maykr/ring_portal_activation/ring_portal_activation_s010_camera";
    const std::string iconStageOneAnimation="cineractive/e3m4_boss/intro_icon_stage1/intro_icon_stage1_s010_camera";
    for(int cut:{-1,0,1,2})
        expect(funDirectedShot(iconStageOneAnimation,cut)==FunAnimatedView::PlayerPov,"current Final Sin stage1 intro stays in first person through any internal camera cuts");
    expect(funDirectedShot("cineractive/e3m4_boss/intro_icon_stage1/intro_icon_stage1_s020_camera")==FunAnimatedView::Unknown&&
        funDirectedShot("cineractive/e3m4_boss/intro_icon_stage2/intro_icon_stage2_s010_camera")==FunAnimatedView::Unknown,"all-POV instruction is confined to the identified current Final Sin intro resource");
    expect(funDirectedShot(yellowRingAnimation,0)==FunAnimatedView::PlayerPov&&
        funDirectedShot(purpleRingPrefix+"020_camera",0)==FunAnimatedView::External&&
        funDirectedShot(ringPortalAnimation,0)==FunAnimatedView::External,"consecutive yellow console and portal scenes follow POV-external then single external using the reused purple clip");
    expect(funDirectedShot("cineractive/e3m3_maykr/yellow_console_ring/yellow_console_ring_s020_camera")==FunAnimatedView::Unknown&&
        funDirectedShot("cineractive/e3m3_maykr/ring_portal_activation/ring_portal_activation_s020_camera")==FunAnimatedView::Unknown,"only recorded yellow and portal aliases receive directed overrides");
    expect(funDirectedShot(purpleRingPrefix+"010_camera",0)==FunAnimatedView::PlayerPov&&
        funDirectedShot(purpleRingPrefix+"020_camera",0)==FunAnimatedView::External,"purple console ring follows the captured first-to-third-person sequence with identical cut markers");
    expect(funDirectedShot(purpleRingPrefix+"030_camera")==FunAnimatedView::Unknown,"the two confirmed purple ring aliases do not assign unseen shots");
    expect(funDirectedShot(vegaRingPrefix+"010_camera",0)==FunAnimatedView::PlayerPov&&
        funDirectedShot(vegaRingPrefix+"020_camera",0)==FunAnimatedView::External,"Vega console ring follows the captured first-to-third-person sequence with identical cut markers");
    expect(funDirectedShot(vegaRingPrefix+"030_camera")==FunAnimatedView::Unknown,"the two confirmed Vega ring aliases do not assign unseen shots");
    const std::string iconResurrectionPrefix="cineractive/e3m3_maykr/icon_resurrection/icon_resurrection_s";
    const std::string khanFightIntroPrefix="cineractive/e3m3_maykr/khan_maykr_fight_stage1_intro/khan_maykr_fight_stage1_intro_s";
    const std::string khanDeathPrefix="cineractive/e3m3_maykr/khan_maykr_fight_stage2_kill/khan_boss_death_s";
    const std::array<const char*,15> khanDeathResourceShots{"010","020","030","040","050","060","070","080","090","100","110","120","130","140","150"};
    for(size_t i=0;i<khanDeathResourceShots.size();++i)
        expect(funDirectedShot(khanDeathPrefix+khanDeathResourceShots[i]+"_camera",0)==
            (i==0?FunAnimatedView::PlayerPov:FunAnimatedView::External),"full Khan death resources use POV only in the first clip including uncaptured external s140 and s150");
    expect(funDirectedShot("cineractive/e3m3_maykr/khan_maykr_fight_stage2_kill/khan_maykr_fight_stage2_kill_s010_camera")==FunAnimatedView::Unknown,"Khan death rules require the actual khan_boss_death basename");
    const std::array<const char*,5> khanFightIntroShots{"010","030","040","050","060"};
    for(size_t i=0;i<khanFightIntroShots.size();++i)
        expect(funDirectedShot(khanFightIntroPrefix+khanFightIntroShots[i]+"_camera",0)==
            (i==0?FunAnimatedView::PlayerPov:FunAnimatedView::External),"Khan fight intro uses POV only in its first clip and external for every remaining captured shot");
    expect(funDirectedShot("cineractive/e3m3_maykr/khan_maykr_fight_stage2_intro/khan_maykr_fight_stage2_intro_s010_camera")==FunAnimatedView::Unknown,"stage1 intro labels do not classify another Khan fight scene");
    const std::array<const char*,29> iconResurrectionShots{"010","020","030","040","050","060","070","080","090","100","110","120","130","140","150","160","170","180","190","200","210","220","230","240","250","260","270","280","290"};
    for(size_t i=0;i<iconResurrectionShots.size();++i)
        expect(funDirectedShot(iconResurrectionPrefix+iconResurrectionShots[i]+"_camera",0)==
            ((i==0||i==4||i==28)?FunAnimatedView::PlayerPov:FunAnimatedView::External),"icon resurrection anchors POV to first fifth and final authored clips across all twenty-nine fragments");
    expect(funDirectedShot(iconResurrectionPrefix+"260_camera")==FunAnimatedView::External&&
        funDirectedShot(iconResurrectionPrefix+"290_camera")==FunAnimatedView::PlayerPov,"twenty-six user angles cannot mistake authored s260 for the actual final clip s290");
    expect(funDirectedShot(powerCruciblePrefix+"040_camera",0)==FunAnimatedView::PlayerPov,"the final power-crucible shot uses the user's corrected first-person view");
    for(const char* code:{"005","010","020","030","050"})
        expect(funDirectedShot(powerCruciblePrefix+code+"_camera")==FunAnimatedView::Unknown,"the final-shot correction preserves existing detection for validated earlier power-crucible shots and unseen aliases");
    const std::string launcherPrefix="cineractive/e2m3_core/human_cannonball_launcher/human_cannonball_launcher_s";
    const std::string escapePodPrefix="cineractive/e2m3_core/escape_pod/escape_pod_s";
    const std::string hallEntrancePrefix="cineractive/e2m4_boss/hall_entrance/hall_entrance_s";
    const std::string arenaBeastPrefix="cineractive/e2m4_boss/beast_of_the_arena/beast_of_the_arena_s";
    const std::string thirdPriestPrefix="cineractive/e2m4_boss/third_priest_death/thrid_priest_death_s";
    const std::array<const char*,8> thirdPriestShots{"010","020","030","040","050","060","070","080"};
    for(size_t i=0;i<thirdPriestShots.size();++i)
        expect(funDirectedShot(thirdPriestPrefix+thirdPriestShots[i]+"_camera",0)==
            (i==0?FunAnimatedView::PlayerPov:FunAnimatedView::External),"third priest death preserves the authored thrid spelling and uses POV only in the first of eight shots");
    expect(funDirectedShot("cineractive/e2m4_boss/third_priest_death/third_priest_death_s010_camera")==FunAnimatedView::Unknown,"a corrected spelling cannot replace the recorded third-priest resource name");
    const std::array<const char*,11> arenaBeastShots{"010","040","100.","110.","120","130","140","150","160","170","180"};
    for(size_t i=0;i<arenaBeastShots.size();++i)
        expect(funDirectedShot(arenaBeastPrefix+arenaBeastShots[i]+"_camera",0)==
            ((i==0||i==10)?FunAnimatedView::PlayerPov:FunAnimatedView::External),"eleven captured arena-beast clips use POV only at the endpoints including literal dotted external aliases");
    expect(funDirectedShot(arenaBeastPrefix+"010._camera")==FunAnimatedView::Unknown&&
        funDirectedShot(hallEntrancePrefix+"110._camera")==FunAnimatedView::Unknown,"exact dotted arena aliases do not broaden camera-name matching");
    const std::array<const char*,13> hallEntranceShots{"010","020","030","040","050","060","070","080","090","100","110","120","130"};
    const std::array<int,13> hallEntranceViews{1,3,3,3,3,3,3,3,3,3,1,3,1};
    for(size_t i=0;i<hallEntranceShots.size();++i)
        expect(funDirectedShot(hallEntrancePrefix+hallEntranceShots[i]+"_camera")==
            (hallEntranceViews[i]==1?FunAnimatedView::PlayerPov:FunAnimatedView::External),"Sentinel Prime hall entrance follows all thirteen labelled shots including the final POV-external-POV");
    const std::array<const char*,6> escapePodShots{"010","020","030","040","050","060"};
    for(size_t i=0;i<escapePodShots.size();++i)
        expect(funDirectedShot(escapePodPrefix+escapePodShots[i]+"_camera",0)==(i==0?FunAnimatedView::PlayerPov:FunAnimatedView::External),"fresh escape-pod retry uses POV only for the first of six shots despite every cut marker being zero");
    expect(funDirectedShot("cineractive/e2m3_core/escape_pod/escape_pod_locked_camera")==FunAnimatedView::Unknown,"escape-pod sequence cannot classify unrelated camera aliases");
    const std::array<const char*,6> launcherShots{"010","020","030","040","050","060"};
    const std::array<int,6> launcherCuts{0,1,0,1,0,1},launcherViews{1,3,1,3,3,3};
    for(size_t i=0;i<launcherShots.size();++i)
        expect(funDirectedShot(launcherPrefix+launcherShots[i]+"_camera",launcherCuts[i])==(launcherViews[i]==1?FunAnimatedView::PlayerPov:FunAnimatedView::External),"human cannonball launch follows all six labelled views including external landing with cut value zero");
    expect(funDirectedShot(launcherPrefix+"010_camera",1)==FunAnimatedView::External&&funDirectedShot(launcherPrefix+"030_camera",1)==FunAnimatedView::External,"launcher cut markers switch outside immediately during short transitions before the next clip name appears");
    expect(funDirectedShot(cannonControlAnimation)==FunAnimatedView::PlayerPov&&funDirectedShot(cannonControlAnimation,0)==FunAnimatedView::PlayerPov,"single-angle human cannonball control room stays in player POV");
    expect(funDirectedShot("cineractive/e2m3_core/human_cannonball_control_room/human_cannonball_control_room_s020_camera")==FunAnimatedView::Unknown,"single confirmed control-room alias does not assign unseen camera resources");
    const std::array<const char*,11> bfgShots{"010","020","030","030","040","050","060","070","080","090","100"};
    const std::array<int,11> bfgCuts{0,0,0,1,0,0,0,0,0,0,0};
    const std::array<int,11> bfgViews{1,3,3,3,3,3,1,3,1,3,1};
    for(size_t i=0;i<bfgShots.size();++i)
        expect(funDirectedShot(bfgPrefix+bfgShots[i]+"_camera",bfgCuts[i])==(bfgViews[i]==1?FunAnimatedView::PlayerPov:FunAnimatedView::External),"complete BFG10K sequence includes the internal s030 cut and all four POV regions");
    const std::string samuelPrefix="cineractive/e2m2_base/confront_samuel/confront_samuel_s";
    const std::array<const char*,15> samuelResourceShots{"010","040","050","060","070","090","100","110","120","130","140","150","160","170","180"};
    for(size_t i=0;i<samuelResourceShots.size();++i)
        expect(funDirectedShot(samuelPrefix+samuelResourceShots[i]+"_camera")==((i==0||i==14)?FunAnimatedView::PlayerPov:FunAnimatedView::External),"complete Samuel camera resources use POV only at their first and final shots");
    expect(funDirectedShot(samuelPrefix+"130_camera")==FunAnimatedView::External&&funDirectedShot(samuelPrefix+"180_camera")==FunAnimatedView::PlayerPov,"incomplete live capture cannot mistake s130 for the final resource s180");
    expect(funDirectedShot("cineractive/e2m2_base/confront_samuel/_wip/s010/confront_samuel_s010.0094_camera")==FunAnimatedView::Unknown,"obsolete WIP camera aliases are excluded from the canonical scene sequence");
    expect(funDirectedShot(turretTwoPrefix+"010_camera")==FunAnimatedView::PlayerPov&&funDirectedShot(turretTwoPrefix+"020_camera")==FunAnimatedView::External,"second ARC turret keeps the captured POV and external clips before ordinary player control resumes");
    expect(funDirectedShot(turretTwoPrefix+"030_camera")==FunAnimatedView::Unknown,"an uncaptured second-turret clip keeps general detection");
    expect(funDirectedShot(turretOnePrefix+"010_camera")==FunAnimatedView::PlayerPov&&funDirectedShot(turretOnePrefix+"020_camera")==FunAnimatedView::External&&funDirectedShot(turretOnePrefix+"030_camera")==FunAnimatedView::PlayerPov,"first ARC turret scene follows the captured POV-external-POV sequence");
    expect(funDirectedShot(bridgeAnimation,0)==FunAnimatedView::External&&funDirectedShot(bridgeAnimation)==FunAnimatedView::External,"recorded ARC bridge flyby is always external despite having one camera and a zero cut value");
    expect(!funPlayerPov(true,true,&cutCamera,cutCamera.position,funDirectedShot(bridgeAnimation,0)),"single-angle external rule overrides even a matching stale player POV origin");
    const std::string heartCenterPrefix="cineractive/e2m1_nest/heart_destruction_center/heart_destruction_center_s";
    expect(funDirectedShot(heartCenterPrefix+"010_camera")==FunAnimatedView::PlayerPov&&funDirectedShot(heartCenterPrefix+"020_camera")==FunAnimatedView::External&&funDirectedShot(heartCenterPrefix+"030_camera")==FunAnimatedView::External,"centre-heart sequence starts in POV then stays external through both remaining animation segments");
    expect(funDirectedShot(heartRightPrefix+"010_camera")==FunAnimatedView::PlayerPov&&funDirectedShot(heartRightPrefix+"020_camera")==FunAnimatedView::External,"captured right-heart clips switch from POV to external");
    expect(funDirectedShot(heartRightPrefix+"030_camera")==FunAnimatedView::Unknown,"uncaptured right-heart clip names retain general detection rather than inventing another authored shot");
    expect(!funPlayerPov(true,true,&cutCamera,playerPov,funDirectedShot(heartRightPrefix+"020_camera"))&&funPlayerPov(true,false,&cutCamera,playerPov),"right-heart external cut returns to normal first-person control after the cinematic ends");
    const std::array<const char*,16> hunterShots{"010","020","030","040","050","060","070","080","090","100","110","120","130","140","150","170"};
    for(size_t i=0;i<hunterShots.size();++i)
        expect(funDirectedShot(hunterPrefix+hunterShots[i]+"_camera",0)==(i==0?FunAnimatedView::PlayerPov:FunAnimatedView::External),"Doom Hunter intro starts in POV and all subsequent recorded clips remain external despite a zero internal cut value");
    expect(!funJointCameraMatch(cutCamera,animatedOrigin,animatedAxis,animatedScale,povJoint),"a camera outside the body does not match an unrelated animated pose");
    auto muffledGain=[](double frequency) {
        float first=0,second=0;double input=0,output=0;
        for(unsigned i=0;i<48000;++i) {
            float sample=static_cast<float>(sin(6.283185307179586*frequency*i/48000));
            float result=funMuffledSample(sample,first,second);
            if(i>24000) { input+=double(sample)*sample;output+=double(result)*result; }
        }
        return sqrt(output/input);
    };
    expect(muffledGain(80)>.9&&muffledGain(1000)<.14&&muffledGain(6000)<.005,"muffled mix retains bass and strongly attenuates mids and treble");
    expect(validLabArguments({"labplus_fun","0"})&&validLabArguments({"labplus_fun","1"}),"console accepts exact on/off");
    for(const auto& args:std::vector<std::vector<std::string>>{{"labplus_fun"},{"labplus_fun","2"},{"labplus_fun","-1"},{"labplus_fun","1","x"},{"labplus_fun","1;quit"}})
        expect(!validLabArguments(args),"invalid fun arguments rejected");
    float volume=0;
    expect(funNumber("0.625",volume)&&volume==.625f,"preserve fractional music volume");
    for(auto text:{"nan","inf","1;quit","-0.1","2",""}) expect(!funNumber(text,volume),"reject malformed channel settings");
    unsigned char musicCall[]{0x48,0x8b,0x05,0,0,0,0,0x41,0xb9,0xf4,0x01,0,0,0x8b,0x8b,0,0,0,0,
        0x49,0x83,0xc8,0xff,0xc6,0x44,0x24,0x28,0,0xc7,0x44,0x24,0x20,4,0,0,0,0xf3,0x0f,0x10,0x40,0x0c,
        0x0f,0x28,0xc8,0xf3,0x0f,0x5d,0xcf,0xf3,0x0f,0x5f,0xce,0x0f,0x28,0xc9,0xe8,0,0,0,0};
    const uintptr_t callAt=0x100000,musicObject=0x120000,rtpcSetter=0x130000;
    int32_t musicRelative=static_cast<int32_t>(musicObject-callAt-7),setterRelative=static_cast<int32_t>(rtpcSetter-callAt-60);uint32_t rtpcOffset=0x84438;
    memcpy(musicCall+3,&musicRelative,4);memcpy(musicCall+15,&rtpcOffset,4);memcpy(musicCall+56,&setterRelative,4);
    auto route=decodeFunMusicCall(musicCall,callAt,musicObject);
    expect(route.setter==rtpcSetter&&route.idOffset==rtpcOffset,"resolve the music-only Wwise route");
    expect(!decodeFunMusicCall(musicCall,callAt,musicObject+128),"SFX/master cvar references cannot select the music bus");
    musicCall[27]=1;expect(!decodeFunMusicCall(musicCall,callAt,musicObject),"reject a changed Wwise call ABI");
    expect(funVisorMaterial("models/characters/doomslayer/doomslayer_game_helmet_visor"),"integrated Slayer visor selected");
    expect(!funVisorMaterial("models/characters/doomslayer/doomslayer_game_torso")&&!funVisorMaterial("models/characters/enemy/helmet_visor"),"body and foreign visor excluded");
    alignas(16) std::array<unsigned char,512> dense{};
    uint16_t keys[]{100,1815,1820};memcpy(dense.data(),keys,sizeof(keys));
    struct Dense { uint32_t count,capacity;uintptr_t data; } block{3,22,reinterpret_cast<uintptr_t>(dense.data())};
    expect(funParmValue(reinterpret_cast<uintptr_t>(&block),1815)==block.data+48+16,"material values follow allocated capacity, not used key count");
    expect(!funParmValue(reinterpret_cast<uintptr_t>(&block),2000),"absent shader parameter is never guessed");
    block.capacity=2;expect(!funParmValue(reinterpret_cast<uintptr_t>(&block),1815),"invalid dense-map bounds rejected");
    block.count=0x80000003u;block.capacity=12;
    expect(funParmValue(reinterpret_cast<uintptr_t>(&block),1815)==block.data+24+16,"inline dense-map flag and eight-byte value alignment match engine accessors");
    block.count=3;
    expect(funHelmetModel("models/characters/doomslayer/helmet")&&funHelmetModel("models/player/praetor_visor"),"explicit Slayer helmet model accepted");
    expect(!funHelmetModel("models/characters/slayer/body")&&!funHelmetModel("models/enemies/sentinel/helmet"),"whole bodies and other helmets excluded");
    expect(funColor(0)!=funColor(.4)&&funColor(0)==funColor(6*.365+.001),"colour cycle follows musical beat");
    GameCamera camera{}; camera.matrix[0]=1; float nearSource[3]{0,0,1},farSource[3]{0,0,30},wideSource[3]{0,0,80},silentSource[3]{0,0,240},rightSource[3]{10,0,0},leftSource[3]{-10,0,0};
    expect(funSpatialMix(camera,nearSource).gain==1&&fabsf(funSpatialMix(camera,farSource).gain-.4f)<.001f&&funSpatialMix(camera,wideSource).gain>.14f&&funSpatialMix(camera,silentSource).gain==0,"wide cinematic shots remain audible with a gentler distance curve and bounded fade");
    expect(funSpatialMix(camera,rightSource).pan>0&&funSpatialMix(camera,leftSource).pan<0,"sound follows the camera's left/right direction");
    nearSource[0]=std::numeric_limits<float>::quiet_NaN(); expect(!funSpatialMix(camera,nearSource).located,"invalid actor positions do not corrupt audio gain");
    const float cinematicHead[]{5,0,0};FunSpatialMix missingHelmet;missingHelmet.gain=0;
    auto hiddenHelmet=funCinematicSpatialMix(camera,missingHelmet,cinematicHead);
    expect(hiddenHelmet.located&&hiddenHelmet.gain==1&&hiddenHelmet.pan>0,"unregistered outside cinema keeps helmet music even when the Slayer model is hidden");
    FunSpatialMix animatedHelmet{.4f,-.7f,true};auto movingHelmet=funCinematicSpatialMix(camera,animatedHelmet,cinematicHead);
    expect(movingHelmet.gain==funSpatialDistanceGain(3.f)&&movingHelmet.pan==-.7f,"live animated helmet direction takes precedence while cinematic listening distance stays three metres");
    const float distantHead[]{1000,0,0};auto distantCinema=funCinematicSpatialMix(camera,missingHelmet,distantHead);
    expect(distantCinema.located&&distantCinema.gain==hiddenHelmet.gain&&distantCinema.gain==funSpatialDistanceGain(3.f)&&distantCinema.pan>0&&funSpatialMix(camera,distantHead).gain==0,"near and distant cinematic shots keep the same three-metre helmet volume and real direction without changing ordinary third-person cutoff");
    const float cinematicHeadLeft[]{-5,0,0};expect(funCinematicSpatialMix(camera,missingHelmet,cinematicHeadLeft).pan<0,"three-metre cinematic music remains positional rather than a centred global track");
    expect(!funCinematicSpatialMix(camera,missingHelmet,nullptr).located&&funCinematicSpatialMix(camera,missingHelmet,nullptr).gain==0,"missing helmet coordinates never invent an audible world position");
    FieldFixture bloatedFields[]{{"idRenderModelInfo","","renderModelInfo",16,64},{}};
    FieldFixture infoFields[]{{"idRenderModel","*","modelPtr",0,8},{"idColor","","color",16,16},{}};
    FieldFixture modelFields[]{{"idAtomicString","","name",8,8},{"idRenderModelParms","","g",16,16},{"idList < idRenderModelSurface , TAG_IDLIST , false >","","surfaces",40,24},{}};
    FieldFixture colorFields[]{{"idVec4","","color",0,16},{}};
    FieldFixture atomFields[]{{"char","*","str",0,8},{}};
    TypeFixture bloated{"idBloatedEntity","idEntity",80,0,bloatedFields}, info{"idRenderModelInfo","",64,0,infoFields},
        model{"idRenderModel","",64,0,modelFields}, parms{"idRenderModelParms","",16,0,colorFields}, atom{"idAtomicString","",8,0,atomFields};
    fixtureTypes={{bloated.name,&bloated},{info.name,&info},{model.name,&model},{parms.name,&parms},{atom.name,&atom}};
    MeathookHudApi api{}; api.findClass=findFixtureClass;
    std::array<unsigned char,4096> image{}; const auto base=reinterpret_cast<uintptr_t>(image.data());
    uintptr_t getter=base+512; memcpy(image.data()+256+16,&getter,8);
    const unsigned char code[]{0x48,0x8d,0x05,0,0,0,0,0xc3}; memcpy(image.data()+512,code,8);
    int32_t displacement=1024-(512+7); memcpy(image.data()+515,&displacement,4);
    const char* className="idBloatedEntity"; memcpy(image.data()+1024,&className,8);
    std::array<unsigned char,80> entity{}; uintptr_t vtable=base+256; memcpy(entity.data(),&vtable,8);
    std::array<unsigned char,64> renderModel{}; uintptr_t render=reinterpret_cast<uintptr_t>(renderModel.data()); memcpy(entity.data()+16,&render,8);
    const char* path="models/characters/doomslayer/helmet"; memcpy(renderModel.data()+8,&path,8);
    const float initial[4]{.8f,.7f,.6f,.9f}; memcpy(entity.data()+32,initial,16); memcpy(renderModel.data()+16,initial,16);
    struct Slot { uint32_t id=7,generation=9; uintptr_t entity,previous=0,next=0; } slot{7,9,reinterpret_cast<uintptr_t>(entity.data())};
    struct Map { uintptr_t list; uint32_t count; uint32_t pad=0; } map{reinterpret_cast<uintptr_t>(&slot),1};
    auto profile=kProfile6663; profile.imageSize=4096; profile.typeGetterLimitRva=4096; profile.mapEntitiesOffset=0;
    FunHelmet helmet; helmet.initialize(api); helmet.update(api,base,profile,reinterpret_cast<uintptr_t>(&map),funColor(0));
    expect(memcmp(entity.data()+32,initial,12)!=0&&memcmp(entity.data()+44,initial+3,4)==0,"only helmet RGB changes; alpha preserved");
    helmet.restore(reinterpret_cast<uintptr_t>(&map));
    expect(!memcmp(entity.data()+32,initial,16)&&!memcmp(renderModel.data()+16,initial,16),"off restores both exact original colours");
    helmet.update(api,base,profile,reinterpret_cast<uintptr_t>(&map),funColor(0));
    ++slot.generation; float replacement[4]{1,1,1,1}; memcpy(entity.data()+32,replacement,16);
    helmet.restore(reinterpret_cast<uintptr_t>(&map));
    expect(!memcmp(entity.data()+32,replacement,16),"recycled entity generation is never restored");
    FieldFixture resourceFields[]{{"idAtomicString","","name",0,8},{"uint64","","resourceID",8,8},{"int","","staleCount",16,4},{}};
    FieldFixture materialFields[]{{"idParmBlock","","Parms",32,16},{}};
    FieldFixture parmFields[]{{"int","","parmIndex",8,4},{}};
    FieldFixture listFields[]{{"resourceList_t","*","resourceList",8,8},{"char","*","className",24,8},{"idResourceList","*","next",32,8},{}};
    TypeFixture resourceType{"idResource","",24,0,resourceFields},materialType{"idMaterial2","idResource",48,0,materialFields},
        parmType{"idDeclRenderParm","idResource",16,0,parmFields},listType{"idResourceList","",48,0,listFields};
    fixtureTypes.insert({{resourceType.name,&resourceType},{materialType.name,&materialType},{parmType.name,&parmType},{listType.name,&listType}});
    std::array<unsigned char,48> resourceList{},listOwner{};std::array<unsigned char,16> declaration{};
    const char* declarationName="glassalbedo";const char* listName="idDeclRenderParm";int declarationIndex=1815;
    memcpy(declaration.data(),&declarationName,8);memcpy(declaration.data()+8,&declarationIndex,4);
    uintptr_t declarationPointer=reinterpret_cast<uintptr_t>(declaration.data()),arrayPointer=reinterpret_cast<uintptr_t>(&declarationPointer);
    uint32_t one=1;memcpy(resourceList.data()+32,&arrayPointer,8);memcpy(resourceList.data()+40,&one,4);memcpy(resourceList.data()+44,&one,4);
    uintptr_t resourceListPointer=reinterpret_cast<uintptr_t>(resourceList.data());memcpy(listOwner.data()+8,&resourceListPointer,8);memcpy(listOwner.data()+24,&listName,8);
    uintptr_t ownerPointer=reinterpret_cast<uintptr_t>(listOwner.data());memcpy(image.data()+2500,&ownerPointer,8);
    unsigned char registryCode[]{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x1d,0,0,0,0,0x48,0x8b,0xf9,0x48,0x85,0xdb,0x74};
    int32_t registryRelative=2500-2017;memcpy(registryCode+13,&registryRelative,4);memcpy(image.data()+2000,registryCode,sizeof(registryCode));
    profile.typeGetterLimitRva=4096;
    FunVisor visor;visor.initialize(api,base,profile);expect(visor.available(),"resolve the visor parameter by declaration name through the resource registry");
    std::array<unsigned char,48> material{};const char* visorName="models/characters/doomslayer/doomslayer_game_helmet_visor";
    memcpy(material.data(),&visorName,8);block.capacity=22;memcpy(material.data()+32,&block,16);
    memcpy(dense.data()+64,initial,16);auto materialPointer=reinterpret_cast<uintptr_t>(material.data());
    visor.consider(materialPointer);expect(visor.selected(),"integrated visor selected without tinting the body model");visor.update(funColor(0));
    expect(memcmp(dense.data()+64,initial,12)&&!memcmp(dense.data()+76,initial+3,4),"glass colour changes while original transparency stays exact");
    visor.restore();expect(!memcmp(dense.data()+64,initial,16),"glass restores its exact previous RGBA");
    visor.consider(materialPointer);visor.update(funColor(0));++material[16];memcpy(dense.data()+64,replacement,16);
    visor.restore();expect(!memcmp(dense.data()+64,replacement,16),"reloaded material is never restored through stale resource identity");
    // The renderer consumes per-surface dynamic parameters, so override only
    // the visor's existing inline block, leaving joints and body data intact.
    FieldFixture surfaceFields[]{{"idMaterial2","*","material",0,8},{"idGrowableParmBlock < 12 >","","parmBlock",80,240},{}};
    TypeFixture surfaceType{"idRenderModelSurface","",320,0,surfaceFields};fixtureTypes[surfaceType.name]=&surfaceType;
    alignas(16) std::array<unsigned char,320> surface{};
    memcpy(surface.data(),&materialPointer,8);uint32_t originalGeneration=0;memcpy(material.data()+16,&originalGeneration,4);
    memcpy(dense.data()+64,initial,16);
    struct SurfaceHeader { uint32_t count,capacity;uintptr_t data; } surfaceHeader{0x80000002u,12,reinterpret_cast<uintptr_t>(surface.data()+96)};
    memcpy(surface.data()+80,&surfaceHeader,16);uint16_t engineKeys[12]{1097,1169,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff};memcpy(surface.data()+96,engineKeys,24);
    float engineValues[8]{1,2,3,4,5,6,7,8};memcpy(surface.data()+120,engineValues,32);
    struct SurfaceList { uintptr_t data;uint32_t count,capacity;uint64_t pad; } surfaceList{reinterpret_cast<uintptr_t>(surface.data()),1,1,0};
    memcpy(renderModel.data()+40,&surfaceList,24);
    FunVisor drawVisor;drawVisor.initialize(api,base,profile);
    drawVisor.considerSurface(materialPointer,surfaceList.data,render,reinterpret_cast<uintptr_t>(&slot),slot.entity,slot.id,slot.generation);
    expect(drawVisor.selected(),"visor draw surface selected with entity and model identity guards");
    expect(funParmValue(surfaceList.data+80,1815)==surfaceList.data+152,"new draw-time glass parameter fits existing surface allocation");
    drawVisor.update(funColor(0));expect(memcmp(surface.data()+152,initial,12)&&!memcmp(surface.data()+164,initial+3,4),"draw-time visor RGB changes while alpha is preserved");
    expect(!memcmp(dense.data()+64,initial,16),"shared source material stays unchanged");
    engineValues[0]=42;memcpy(surface.data()+120,engineValues,32);
    drawVisor.restore();expect(!funParmValue(surfaceList.data+80,1815),"off removes the inserted draw-time override");
    expect(!memcmp(surface.data()+120,engineValues,32),"off preserves engine updates to joint and geometry parameters");
    uint32_t restoredCount=0;memcpy(&restoredCount,surface.data()+80,4);expect(restoredCount==surfaceHeader.count,"off preserves the inline-storage marker and original parameter count");
    FieldFixture rigModelFields[]{{"idAtomicString","","name",8,8},{"idRenderModelParms","","g",16,76},{"int","","surfacesCommittedFrameCount",112,4},{}};
    FieldFixture rigParmsFields[]{{"idVec3","","origin",0,12},{"idMat3","","axis",12,36},{"idVec3","","scale",48,12},{"idVec4","","color",60,16},{}};
    FieldFixture skinFields[]{{"idMD6Model","*","md6Model",96,8},{"idJointAnimator","*","jointAnimator",104,8},{}};
    FieldFixture meshFields[]{{"idMD6Skel","*","skeleton",0,8},{}};
    FieldFixture skeletonFields[]{{"idMD6SkelData","*","data",0,8},{}};
    FieldFixture layoutFields[]{{"unsigned short","","numJoints",0,2},{}};
    FieldFixture animatorFields[]{{"idJointMat","*[4]","joints",0,32},{"int","","currentDeferred",32,4},{}};
    TypeFixture rigModelType{"idRenderModel","",128,0,rigModelFields},rigParmsType{"idRenderModelParms","",76,0,rigParmsFields},
        skinType{"idRenderModelSkinned","idRenderModel",112,0,skinFields},meshType{"idMD6Model","",8,0,meshFields},
        skeletonType{"idMD6Skel","",8,0,skeletonFields},layoutType{"idMD6SkelData","",2,0,layoutFields},animatorType{"idJointAnimator","",36,0,animatorFields};
    for(auto* type:{&rigModelType,&rigParmsType,&skinType,&meshType,&skeletonType,&layoutType,&animatorType}) fixtureTypes[type->name]=type;
    std::array<unsigned char,128> rigModel{};std::array<unsigned char,40> animatorData{};uint16_t jointCount=1;
    uintptr_t layoutPointer=reinterpret_cast<uintptr_t>(&jointCount),skeletonPointer=reinterpret_cast<uintptr_t>(&layoutPointer),meshPointer=reinterpret_cast<uintptr_t>(&skeletonPointer),
        animatorPointer=reinterpret_cast<uintptr_t>(animatorData.data()),posePointer=reinterpret_cast<uintptr_t>(povJoint);
    memcpy(rigModel.data()+96,&meshPointer,8);memcpy(rigModel.data()+104,&animatorPointer,8);memcpy(animatorData.data(),&posePointer,8);
    memcpy(rigModel.data()+16,animatedOrigin,12);memcpy(rigModel.data()+28,animatedAxis,36);memcpy(rigModel.data()+64,animatedScale,12);
    FunCameraRig rig;rig.initialize(api);
    expect(rig.matches(reinterpret_cast<uintptr_t>(rigModel.data()),animatedCamera),"production pose reader resolves the captured cinematic POV through reflected fields");
    expect(!rig.matches(reinterpret_cast<uintptr_t>(rigModel.data()),cutCamera),"production pose reader excludes the captured external camera");
    jointCount=1025;expect(!rig.matches(reinterpret_cast<uintptr_t>(rigModel.data()),animatedCamera),"pose reader bounds joint memory reads");
    jointCount=1;
    // Exercise the complete cached-entity classifier across a camera cut.
    std::array<unsigned char,128> actorModel=rigModel;
    std::array<unsigned char,80> rigEntity{},actorEntity{};
    std::array<unsigned char,40> actorAnimator{};
    const float actorJoint[]{1,0,0,0,0,1,0,0,0,0,1,0};
    uintptr_t actorPose=reinterpret_cast<uintptr_t>(actorJoint),actorAnimatorPointer=reinterpret_cast<uintptr_t>(actorAnimator.data());
    memcpy(actorAnimator.data(),&actorPose,8);memcpy(actorModel.data()+104,&actorAnimatorPointer,8);
    const char* rigName="md6def/player/human/base/tp_body.md6";
    const char* actorName="md6def/characters/humans/doomslayer/base/doomslayer_cine.md6";
    memcpy(rigModel.data()+8,&rigName,8);memcpy(actorModel.data()+8,&actorName,8);
    uintptr_t rigPointer=reinterpret_cast<uintptr_t>(rigModel.data()),actorPointer=reinterpret_cast<uintptr_t>(actorModel.data());
    memcpy(rigEntity.data(),&vtable,8);memcpy(actorEntity.data(),&vtable,8);
    memcpy(rigEntity.data()+16,&rigPointer,8);memcpy(actorEntity.data()+16,&actorPointer,8);
    Slot shotSlots[]{{1,1,reinterpret_cast<uintptr_t>(rigEntity.data())},{2,1,reinterpret_cast<uintptr_t>(actorEntity.data())}};
    Map shotMap{reinterpret_cast<uintptr_t>(shotSlots),2};
    uint32_t rigFrame=0x5268,actorFrame=~0u;
    memcpy(rigModel.data()+112,&rigFrame,4);memcpy(actorModel.data()+112,&actorFrame,4);
    FunHelmet shotHelmet;shotHelmet.initialize(api);
    shotHelmet.update(api,base,profile,reinterpret_cast<uintptr_t>(&shotMap),funColor(0),false);
    expect(shotHelmet.cameraAtAnimatedBody(animatedCamera)==FunAnimatedView::PlayerPov,"initial POV shot ignores an unsubmitted full actor");
    actorFrame=0x23b77;memcpy(actorModel.data()+112,&actorFrame,4);
    expect(shotHelmet.cameraAtAnimatedBody(animatedCamera)==FunAnimatedView::External,"camera cut switches to external even when the helper commit remains frozen");
    rigFrame=actorFrame+10;memcpy(rigModel.data()+112,&rigFrame,4);
    expect(shotHelmet.cameraAtAnimatedBody(animatedCamera)==FunAnimatedView::PlayerPov,"return to POV ignores an older cached full actor");
    // Head attachment is read from the animated pose, independently of the
    // camera-helper joint and the entity's stationary model root.
    FieldFixture headLayoutFields[]{{"unsigned short","","size",0,2},{"unsigned short","","numJoints",2,2},{"unsigned short","","inverseBasePoseOffset",10,2},{"unsigned short","","parentTblOffset",12,2},{}};
    TypeFixture headLayoutType{"idMD6SkelData","",64,0,headLayoutFields};fixtureTypes[headLayoutType.name]=&headLayoutType;
    std::array<unsigned char,6000> headLayout{};uint16_t headBytes=6000,headCount=92,bindOffset=512,parentOffset=64;int16_t neck=90;
    memcpy(headLayout.data(),&headBytes,2);memcpy(headLayout.data()+2,&headCount,2);memcpy(headLayout.data()+10,&bindOffset,2);memcpy(headLayout.data()+12,&parentOffset,2);
    memcpy(headLayout.data()+parentOffset+91*2,&neck,2);
    const float headBind[]{1,0,0,-.02286f,0,1,0,0,0,0,1,-1.72174f};memcpy(headLayout.data()+bindOffset+91*48,headBind,48);
    std::array<std::array<float,12>,92> headPose{};headPose[91]={1,0,0,4,0,1,0,-2,0,0,1,1.7f};headPose[69]={1,0,0,100,0,1,0,100,0,0,1,100};
    layoutPointer=reinterpret_cast<uintptr_t>(headLayout.data());posePointer=reinterpret_cast<uintptr_t>(headPose.data());memcpy(animatorData.data(),&posePointer,8);
    FunCameraRig headRig;headRig.initialize(api);float soundPosition[3]{};
    expect(headRig.headPosition(rigPointer,rigName,soundPosition)&&fabsf(soundPosition[0]-65.01f)<.001f&&fabsf(soundPosition[1]+1424.75f)<.001f&&fabsf(soundPosition[2]-18.72f)<.001f,"sound follows the animated head joint rather than root plus a fixed height");
    headPose[91][3]=-4;expect(headRig.headPosition(rigPointer,rigName,soundPosition)&&soundPosition[0]<animatedOrigin[0],"head movement updates the sound position while model origin and camera joint stay fixed");
    const float turnedAxis[]{0,1,0,-1,0,0,0,0,1},scaledBody[]{2,1,1};memcpy(rigModel.data()+28,turnedAxis,36);memcpy(rigModel.data()+64,scaledBody,12);
    expect(headRig.headPosition(rigPointer,rigName,soundPosition)&&fabsf(soundPosition[0]-63.01f)<.001f&&fabsf(soundPosition[1]+1430.75f)<.001f,"helmet source follows model rotation and scale");
    memcpy(rigModel.data()+28,animatedAxis,36);memcpy(rigModel.data()+64,animatedScale,12);
    expect(!headRig.headPosition(rigPointer,"md6def/unknown_body.md6",soundPosition),"unverified skeleton cannot supply an arbitrary sound attachment");
    neck=89;memcpy(headLayout.data()+parentOffset+91*2,&neck,2);expect(!headRig.headPosition(rigPointer,rigName,soundPosition),"mismatched head parent rejects a replaced skeleton");neck=90;memcpy(headLayout.data()+parentOffset+91*2,&neck,2);
    // Read an active camera's authored clip through the production entity cache.
    FieldFixture entityFields[]{{"idStr","","name",64,48},{}};
    FieldFixture cameraFields[]{{"bool","","isActive",112,1},{}};
    FieldFixture animCameraFields[]{{"idMidnightAnimMdntInfo","*","midnightAnimInfo",120,8},{"idVec3","","camPos",128,12},{"int","","lastCutValue",144,4},{}};
    FieldFixture clipInfoFields[]{{"idList < idMidnightAnimClipInfo , TAG_ARK_MIDNIGHT , false >","","clipList",0,24},{}};
    FieldFixture clipFields[]{{"idAtomicString","","anim",0,8},{}};
    TypeFixture entityType{"idEntity","",112,0,entityFields},cameraType{"idCamera","idBloatedEntity",120,0,cameraFields},
        animCameraType{"idAnimCamera","idCamera",160,0,animCameraFields},clipInfoType{"idMidnightAnimMdntInfo","",24,0,clipInfoFields},clipType{"idMidnightAnimClipInfo","",24,0,clipFields};
    for(auto* type:{&entityType,&cameraType,&animCameraType,&clipInfoType,&clipType}) fixtureTypes[type->name]=type;
    uintptr_t cameraGetter=base+576;memcpy(image.data()+320+16,&cameraGetter,8);memcpy(image.data()+576,code,8);int32_t cameraRelative=1152-(576+7);memcpy(image.data()+579,&cameraRelative,4);
    const char* cameraClassName="idAnimCamera";memcpy(image.data()+1152,&cameraClassName,8);
    std::array<unsigned char,160> cameraEntity{};uintptr_t cameraVtable=base+320;memcpy(cameraEntity.data(),&cameraVtable,8);
    const char* cameraName="khan_maykr_priest_meeting_camera";const char* actorEntityName="khan_maykr_priest_meeting_player";
    memcpy(cameraEntity.data()+72,&cameraName,8);memcpy(rigEntity.data()+72,&actorEntityName,8);cameraEntity[112]=1;
    std::array<unsigned char,24> cameraClip{};std::string clipName=shotPrefix+"020_camera";const char* clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
    SurfaceList cameraClipList{reinterpret_cast<uintptr_t>(cameraClip.data()),1,1,0};uintptr_t cameraInfo=reinterpret_cast<uintptr_t>(&cameraClipList);
    memcpy(cameraEntity.data()+120,&cameraInfo,8);memcpy(cameraEntity.data()+128,animatedCamera.position,12);
    Slot directedSlots[]{shotSlots[0],shotSlots[1],{3,3,reinterpret_cast<uintptr_t>(cameraEntity.data())}};Map directedMap{reinterpret_cast<uintptr_t>(directedSlots),3};
    FunHelmet directedHelmet;directedHelmet.initialize(api);directedHelmet.update(api,base,profile,reinterpret_cast<uintptr_t>(&directedMap),funColor(0),false);
    auto directed=directedHelmet.directedCamera(animatedCamera);
    expect(directed.view==FunAnimatedView::External&&directed.actorPrefix=="khan_maykr_priest_meeting_","production reader selects the labelled external clip and its own scene actor");
    clipName=shotPrefix+"030_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
    expect(directedHelmet.directedCamera(animatedCamera).view==FunAnimatedView::PlayerPov,"camera cuts change POV immediately without waiting for the entity rescan");
    GameCamera laggedCamera=animatedCamera;laggedCamera.position[0]+=20;
    expect(directedHelmet.directedCamera(laggedCamera).view==FunAnimatedView::PlayerPov,"one active directed camera tolerates animation/render frame skew at a cut");
    clipName=shotPrefix+"035_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
    expect(directedHelmet.directedCamera(animatedCamera).view==FunAnimatedView::External,"captured s030 POV switches to s035 third person at the next authored cut");
    clipName=sentinelPrefix+"080_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
    expect(directedHelmet.directedCamera(animatedCamera).view==FunAnimatedView::PlayerPov,"production reader enters the first Sentinel King POV interval");
    clipName=sentinelPrefix+"090_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
    expect(directedHelmet.directedCamera(animatedCamera).view==FunAnimatedView::External,"production reader leaves Sentinel King POV at the labelled s090 cut");
    clipName=sentinelPrefix+"140_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
    expect(directedHelmet.directedCamera(animatedCamera).view==FunAnimatedView::PlayerPov,"production reader restores POV for the final Sentinel King shot");
    cameraEntity[112]=0;expect(directedHelmet.directedCamera(animatedCamera).view==FunAnimatedView::Unknown,"inactive cached camera cannot force a shot mode");cameraEntity[112]=1;
    GameCamera soundCamera{};memcpy(soundCamera.position,animatedOrigin,12);soundCamera.matrix[0]=1;
    headPose[91][3]=4;auto helmetSound=directedHelmet.spatial(soundCamera,directed.actorPrefix);
    expect(helmetSound.located&&helmetSound.pan>0,"third-person music is panned from the live helmet position");
    headPose[91][3]=-4;expect(directedHelmet.spatial(soundCamera,directed.actorPrefix).pan<0,"music follows the moving helmet across the camera");
    expect(!directedHelmet.spatial(soundCamera,"another_scene_").located&&directedHelmet.spatial(soundCamera,"another_scene_").gain==0,"another cutscene's cached model cannot become the sound source");
    GameCamera platformCamera{};platformCamera.matrix[0]=1;platformCamera.position[0]=-17.54003143310547f;platformCamera.position[1]=-864.9989013671875f;platformCamera.position[2]=-30.32727813720703f;
    const float platformHelmet[]{-10.514235496520996f,-865.2009887695312f,-30.642642974853516f};
    expect(!funPlayerPov(true,true,&platformCamera,platformHelmet),"captured single-angle platform camera is outside the actual player POV");
    auto platformAudio=funCinematicSpatialMix(platformCamera,directedHelmet.spatial(platformCamera,"floating_platform_"),platformHelmet);
    expect(platformAudio.located&&platformAudio.gain==1&&platformAudio.pan>0,"recorded seven-metre platform flyby has global outside helmet sound without its own scene rule or render model");
    std::array<unsigned char,10000> fullLayout{};uint16_t fullBytes=10000,fullCount=163;int16_t fullNeck=87;
    memcpy(fullLayout.data(),&fullBytes,2);memcpy(fullLayout.data()+2,&fullCount,2);memcpy(fullLayout.data()+10,&bindOffset,2);memcpy(fullLayout.data()+12,&parentOffset,2);
    memcpy(fullLayout.data()+parentOffset+88*2,&fullNeck,2);const float fullHeadBind[]{1,0,0,-.054879f,0,1,0,0,0,0,1,-1.778335f};memcpy(fullLayout.data()+bindOffset+88*48,fullHeadBind,48);
    uintptr_t fullLayoutPointer=reinterpret_cast<uintptr_t>(fullLayout.data()),fullSkeletonPointer=reinterpret_cast<uintptr_t>(&fullLayoutPointer),fullMeshPointer=reinterpret_cast<uintptr_t>(&fullSkeletonPointer);
    memcpy(actorModel.data()+96,&fullMeshPointer,8);std::array<std::array<float,12>,163> fullPose{};fullPose[88]={1,0,0,3,0,1,0,0,0,0,1,1.8f};
    uintptr_t fullPosePointer=reinterpret_cast<uintptr_t>(fullPose.data());memcpy(actorAnimator.data(),&fullPosePointer,8);
    expect(headRig.headPosition(actorPointer,actorName,soundPosition)&&fabsf(soundPosition[0]-64.01f)<.001f,"full cinematic Slayer uses its verified head joint rather than the helper camera joint");
    const char* fullEntityName="khan_maykr_priest_meeting_doomslayer_cine";memcpy(actorEntity.data()+72,&fullEntityName,8);
    rigFrame=100;actorFrame=200;memcpy(rigModel.data()+112,&rigFrame,4);memcpy(actorModel.data()+112,&actorFrame,4);
    FunHelmet fullSourceHelmet;fullSourceHelmet.initialize(api);fullSourceHelmet.update(api,base,profile,reinterpret_cast<uintptr_t>(&directedMap),funColor(0),false);
    expect(fullSourceHelmet.spatial(soundCamera,directed.actorPrefix).pan>0,"currently rendered cinematic helmet wins over the old helper model");
    rigFrame=300;memcpy(rigModel.data()+112,&rigFrame,4);
    expect(fullSourceHelmet.spatial(soundCamera,directed.actorPrefix).pan<0,"inactive cinematic helmet cannot steal sound from a newly rendered player rig");
    cameraName="mech_hand_cannon_func_camera";actorEntityName="mech_hand_cannon_player";
    memcpy(cameraEntity.data()+72,&cameraName,8);memcpy(rigEntity.data()+72,&actorEntityName,8);
    clipName=mechAnimation;clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);int internalCut=0;memcpy(cameraEntity.data()+144,&internalCut,4);
    FunHelmet mechHelmet;mechHelmet.initialize(api);mechHelmet.update(api,base,profile,reinterpret_cast<uintptr_t>(&directedMap),funColor(0),false);
    auto mechShot=mechHelmet.directedCamera(animatedCamera);
    expect(mechShot.view==FunAnimatedView::PlayerPov&&mechShot.actorPrefix=="mech_hand_cannon_","authored scene prefix connects a func camera to its actual Slayer actor");
    internalCut=1;memcpy(cameraEntity.data()+144,&internalCut,4);mechShot=mechHelmet.directedCamera(animatedCamera);
    expect(mechShot.view==FunAnimatedView::External,"live internal cut value switches view without changing animation clip or rescanning");
    expect(mechHelmet.spatial(soundCamera,mechShot.actorPrefix).located&&mechHelmet.spatial(soundCamera,mechShot.actorPrefix).gain>0,"third-person mech music comes from its helmet rather than muting on the func camera prefix");
    cameraName="mech_spear_func_camera";actorEntityName="mech_spear_player";memcpy(cameraEntity.data()+72,&cameraName,8);memcpy(rigEntity.data()+72,&actorEntityName,8);
    clipName=spearAnimation;clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);mechHelmet.restore(reinterpret_cast<uintptr_t>(&directedMap));
    mechHelmet.update(api,base,profile,reinterpret_cast<uintptr_t>(&directedMap),funColor(0),false);
    for(int cut:{0,1,0}) {
        memcpy(cameraEntity.data()+144,&cut,4);auto spearShot=mechHelmet.directedCamera(animatedCamera);
        expect(spearShot.view==(cut==0?FunAnimatedView::PlayerPov:FunAnimatedView::External)&&spearShot.actorPrefix=="mech_spear_","production reader follows the complete recorded spear camera sequence and its own actor");
    }
    cameraName="mech_shoulder_cannon_func_camera";actorEntityName="mech_shoulder_cannon_player";
    memcpy(cameraEntity.data()+72,&cameraName,8);memcpy(rigEntity.data()+72,&actorEntityName,8);clipName=shoulderAnimation;clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
    mechHelmet.restore(reinterpret_cast<uintptr_t>(&directedMap));mechHelmet.update(api,base,profile,reinterpret_cast<uintptr_t>(&directedMap),funColor(0),false);
    for(int cut:{0,1,0}) {
        memcpy(cameraEntity.data()+144,&cut,4);auto shoulderShot=mechHelmet.directedCamera(animatedCamera);
        expect(shoulderShot.view==(cut==0?FunAnimatedView::PlayerPov:FunAnimatedView::External)&&shoulderShot.actorPrefix=="mech_shoulder_cannon_","production reader follows shoulder cannon cuts and associates its helmet source");
    }
    cameraName="sentinel_survivor_camera";actorEntityName="sentinel_survivor_player";
    memcpy(cameraEntity.data()+72,&cameraName,8);memcpy(rigEntity.data()+72,&actorEntityName,8);
    mechHelmet.restore(reinterpret_cast<uintptr_t>(&directedMap));mechHelmet.update(api,base,profile,reinterpret_cast<uintptr_t>(&directedMap),funColor(0),false);
    for(size_t i=0;i<survivorShots.size();++i) {
        clipName=survivorPrefix+survivorShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        auto survivorShot=mechHelmet.directedCamera(animatedCamera);
        expect(survivorShot.view==((i==0||i==13)?FunAnimatedView::PlayerPov:FunAnimatedView::External)&&survivorShot.actorPrefix=="sentinel_survivor_","production clip reader follows all fourteen labelled survivor shots without rescanning");
    }
    for(const auto& entry:std::vector<std::pair<std::string,FunAnimatedView>>{
        {possessionPrefix+"010_camera",FunAnimatedView::PlayerPov},{possessionPrefix+"020_camera",FunAnimatedView::External},
        {shotgunPrefix+"010_camera",FunAnimatedView::External},{shotgunPrefix+"015_camera",FunAnimatedView::External},{shotgunPrefix+"020_camera",FunAnimatedView::PlayerPov},
        {secondPriestPrefix+"010_camera",FunAnimatedView::PlayerPov},{secondPriestPrefix+"030_camera",FunAnimatedView::External},
        {heartLeftPrefix+"010_camera",FunAnimatedView::PlayerPov},{heartLeftPrefix+"020_camera",FunAnimatedView::External},{heartLeftPrefix+"030_camera",FunAnimatedView::PlayerPov},
        {heartRightPrefix+"010_camera",FunAnimatedView::PlayerPov},{heartRightPrefix+"020_camera",FunAnimatedView::External},
        {heartCenterPrefix+"010_camera",FunAnimatedView::PlayerPov},{heartCenterPrefix+"020_camera",FunAnimatedView::External},{heartCenterPrefix+"030_camera",FunAnimatedView::External},
        {bridgeAnimation,FunAnimatedView::External},
        {turretOnePrefix+"010_camera",FunAnimatedView::PlayerPov},{turretOnePrefix+"020_camera",FunAnimatedView::External},{turretOnePrefix+"030_camera",FunAnimatedView::PlayerPov},
        {turretTwoPrefix+"010_camera",FunAnimatedView::PlayerPov},{turretTwoPrefix+"020_camera",FunAnimatedView::External},
        {cannonControlAnimation,FunAnimatedView::PlayerPov}}) {
        clipName=entry.first;clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==entry.second,"production reader follows labelled possession and shotgun-return cinematic clips");
    }
    for(size_t i=0;i<hunterShots.size();++i) {
        clipName=hunterPrefix+hunterShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==(i==0?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader applies the complete captured Doom Hunter intro pattern");
    }
    for(size_t i=0;i<samuelResourceShots.size();++i) {
        clipName=samuelPrefix+samuelResourceShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==((i==0||i==14)?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader applies first-and-last POV over all fifteen Samuel resource cameras");
    }
    for(size_t i=0;i<bfgShots.size();++i) {
        clipName=bfgPrefix+bfgShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);memcpy(cameraEntity.data()+144,&bfgCuts[i],4);
        expect(mechHelmet.directedCamera(animatedCamera).view==(bfgViews[i]==1?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader follows the user's complete eleven-angle BFG10K pattern");
    }
    for(size_t i=0;i<launcherShots.size();++i) {
        clipName=launcherPrefix+launcherShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);memcpy(cameraEntity.data()+144,&launcherCuts[i],4);
        expect(mechHelmet.directedCamera(animatedCamera).view==(launcherViews[i]==1?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader follows the recorded launcher and landing camera sequence");
    }
    for(size_t i=0;i<escapePodShots.size();++i) {
        clipName=escapePodPrefix+escapePodShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        const int escapeCut=0;memcpy(cameraEntity.data()+144,&escapeCut,4);
        expect(mechHelmet.directedCamera(animatedCamera).view==(i==0?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader follows all six fresh escape-pod views without mistaking zero cut markers for POV");
    }
    for(size_t i=0;i<hallEntranceShots.size();++i) {
        clipName=hallEntrancePrefix+hallEntranceShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==(hallEntranceViews[i]==1?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader follows every labelled hall-entrance clip while the camera entity remains unchanged");
    }
    for(size_t i=0;i<arenaBeastShots.size();++i) {
        clipName=arenaBeastPrefix+arenaBeastShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==((i==0||i==10)?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader preserves both dotted resource names and all eleven arena-beast view assignments");
    }
    for(size_t i=0;i<thirdPriestShots.size();++i) {
        clipName=thirdPriestPrefix+thirdPriestShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==(i==0?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader follows all eight third-priest death views including the external final shot");
    }
    for(const char* code:{"005","010","020","030","040"}) {
        clipName=powerCruciblePrefix+code+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==(std::string(code)=="040"?FunAnimatedView::PlayerPov:FunAnimatedView::Unknown),"production reader applies the final power-crucible POV correction only to s040");
    }
    for(size_t i=0;i<iconResurrectionShots.size();++i) {
        clipName=iconResurrectionPrefix+iconResurrectionShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==((i==0||i==4||i==28)?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader applies icon resurrection POV anchors and keeps intervening fragments external through s280");
    }
    for(const char* code:{"010","020","030"}) {
        clipName=vegaRingPrefix+code+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        const auto expected=std::string(code)=="010"?FunAnimatedView::PlayerPov:std::string(code)=="020"?FunAnimatedView::External:FunAnimatedView::Unknown;
        expect(mechHelmet.directedCamera(animatedCamera).view==expected,"production reader applies both Vega console ring views while retaining detection for unseen resources");
    }
    for(const char* code:{"010","020","030"}) {
        clipName=purpleRingPrefix+code+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        const auto expected=std::string(code)=="010"?FunAnimatedView::PlayerPov:std::string(code)=="020"?FunAnimatedView::External:FunAnimatedView::Unknown;
        expect(mechHelmet.directedCamera(animatedCamera).view==expected,"production reader follows purple console ring's two labelled views and leaves unseen aliases unknown");
    }
    for(const auto& entry:std::vector<std::pair<std::string,FunAnimatedView>>{
        {yellowRingAnimation,FunAnimatedView::PlayerPov},{purpleRingPrefix+"020_camera",FunAnimatedView::External},
        {ringPortalAnimation,FunAnimatedView::External}}) {
        clipName=entry.first;clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==entry.second,"production reader follows both consecutive Urdak scenes including single-angle external portal activation");
    }
    for(size_t i=0;i<khanFightIntroShots.size();++i) {
        clipName=khanFightIntroPrefix+khanFightIntroShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==(i==0?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader keeps Khan fight intro external after the first shot through its final s060");
    }
    for(size_t i=0;i<khanDeathResourceShots.size();++i) {
        clipName=khanDeathPrefix+khanDeathResourceShots[i]+"_camera";clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
        expect(mechHelmet.directedCamera(animatedCamera).view==(i==0?FunAnimatedView::PlayerPov:FunAnimatedView::External),"production reader follows all fifteen loaded Khan death resources beyond the incomplete s130 recording");
    }
    clipName=iconStageOneAnimation;clipText=clipName.c_str();memcpy(cameraClip.data(),&clipText,8);
    for(int cut:{0,1,2}) {
        memcpy(cameraEntity.data()+144,&cut,4);
        expect(mechHelmet.directedCamera(animatedCamera).view==FunAnimatedView::PlayerPov,"production reader keeps the complete Final Sin stage1 intro in POV regardless of cut changes");
    }
    // Captured Revenant control keeps idPlayer and isControllingAi=false;
    // its managed controlledEntities list is the authoritative ownership link.
    uintptr_t demonGetter=base+640;memcpy(image.data()+384+16,&demonGetter,8);memcpy(image.data()+640,code,8);int32_t demonRelative=1280-(640+7);memcpy(image.data()+643,&demonRelative,4);
    const char* demonClassName="idDemonPlayer_Revenant";memcpy(image.data()+1280,&demonClassName,8);
    std::array<unsigned char,16> demonEntity{};uintptr_t demonVtable=base+384;memcpy(demonEntity.data(),&demonVtable,8);
    std::array<unsigned char,168> controlledPlayer{};const auto controlledAddress=reinterpret_cast<uintptr_t>(controlledPlayer.data());
    InspectorField controlledField{16,152,"idStaticList < idManagedClassPtr < idEntity > , 4 , false , TAG_IDLIST >"};
    SurfaceList controlledList{controlledAddress+40,1,4,0};memcpy(controlledPlayer.data()+16,&controlledList,24);
    uintptr_t controlledEntity=reinterpret_cast<uintptr_t>(demonEntity.data());memcpy(controlledPlayer.data()+48,&controlledEntity,8);
    expect(funControlledDemon(WallMemory{},controlledAddress,controlledField,base,profile),"actual managed Revenant ownership suppresses fun independently of a false AI flag");
    controlledEntity=reinterpret_cast<uintptr_t>(cameraEntity.data());memcpy(controlledPlayer.data()+48,&controlledEntity,8);
    expect(!funControlledDemon(WallMemory{},controlledAddress,controlledField,base,profile),"a cinematic camera in the control list keeps ordinary cutscene effects");
    controlledList.count=0;memcpy(controlledPlayer.data()+16,&controlledList,24);
    expect(!funControlledDemon(WallMemory{},controlledAddress,controlledField,base,profile),"releasing controlled entities resumes Slayer fun routing");
    controlledList.count=5;memcpy(controlledPlayer.data()+16,&controlledList,24);
    expect(!funControlledDemon(WallMemory{},controlledAddress,controlledField,base,profile),"controlled entity reads are bounded by the reflected inline capacity");
    controlledList.count=1;controlledList.data=base;memcpy(controlledPlayer.data()+16,&controlledList,24);
    expect(!funControlledDemon(WallMemory{},controlledAddress,controlledField,base,profile)&&!funControlledDemon(WallMemory{},0,controlledField,base,profile),"invalid managed-list storage and missing players cannot trigger suppression");
    FunAudio audio; expect(!audio.start(L"missing-fun-audio.wav"),"missing audio fails without changing game volume"); audio.stop(); audio.stop();
    puts("Fun mode checks passed: console, volume parsing, beat cycle, helmet selection, exact rollback and entity reuse.");
}
