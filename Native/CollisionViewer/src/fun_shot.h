#pragma once

struct FunShotView { FunAnimatedView view=FunAnimatedView::Unknown; std::string actorPrefix; };

// Authored clips recorded with the user's POV labels.
// This entity drives several different shots; its name/ownership and camera
// helper joint alone cannot distinguish them. Other scenes keep detection.
static FunAnimatedView funDirectedShot(const std::string& animation,int internalCut=-1) {
    if(internalCut==1&&(animation=="cineractive/e2m3_core/human_cannonball_launcher/human_cannonball_launcher_s010_camera"||
                       animation=="cineractive/e2m3_core/human_cannonball_launcher/human_cannonball_launcher_s030_camera")) return FunAnimatedView::External;
    if(animation=="cineractive/e2m3_core/human_cannonball_control_room/human_cannonball_control_room_s010_camera") return FunAnimatedView::PlayerPov;
    if(animation=="cineractive/e3m1_slayer/power_crucible/power_crucible_s040_camera") return FunAnimatedView::PlayerPov;
    if(animation=="cineractive/e3m3_maykr/vega_console_ring/vega_console_ring_s010_camera") return FunAnimatedView::PlayerPov;
    if(animation=="cineractive/e3m3_maykr/vega_console_ring/vega_console_ring_s020_camera") return FunAnimatedView::External;
    if(animation=="cineractive/e3m3_maykr/purple_console_ring/purple_console_ring_s010_camera") return FunAnimatedView::PlayerPov;
    if(animation=="cineractive/e3m3_maykr/purple_console_ring/purple_console_ring_s020_camera") return FunAnimatedView::External;
    if(animation=="cineractive/e3m3_maykr/yellow_console_ring/yellow_console_ring_s010_camera") return FunAnimatedView::PlayerPov;
    if(animation=="cineractive/e3m3_maykr/ring_portal_activation/ring_portal_activation_s010_camera") return FunAnimatedView::External;
    if(animation=="cineractive/e3m4_boss/intro_icon_stage1/intro_icon_stage1_s010_camera") return FunAnimatedView::PlayerPov;
    // These two shipped clip names contain a literal dot after the shot number.
    if(animation=="cineractive/e2m4_boss/beast_of_the_arena/beast_of_the_arena_s100._camera"||
       animation=="cineractive/e2m4_boss/beast_of_the_arena/beast_of_the_arena_s110._camera") return FunAnimatedView::External;
    if(animation=="cineractive/e2m2_base/destroy_turrets_2/destroy_turrets_2_s010_camera") return FunAnimatedView::PlayerPov;
    if(animation=="cineractive/e2m2_base/destroy_turrets_2/destroy_turrets_2_s020_camera") return FunAnimatedView::External;
    if(animation=="cineractive/e2m2_base/crossing_bridge/crossing_bridge_camera") return FunAnimatedView::External;
    if(animation=="cineractive/e2m1_nest/heart_destruction_right/heart_destruction_right_s010_camera") return FunAnimatedView::PlayerPov;
    if(animation=="cineractive/e2m1_nest/heart_destruction_right/heart_destruction_right_s020_camera") return FunAnimatedView::External;
    if(animation=="cineractive/e1m2_battle/mech_hand_cannon/mech_hand_cannon_camera"||
       animation=="cineractive/e1m2_battle/mech_spear/mech_spear_camera"||
       animation=="cineractive/e1m2_battle/mech_shoulder_cannon/mech_shoulder_cannon_camera") {
        if(internalCut==0) return FunAnimatedView::PlayerPov;
        if(internalCut==1) return FunAnimatedView::External;
        return FunAnimatedView::Unknown;
    }
    struct Family {const char* prefix;const char* firstPov;const char* secondPov;const char* thirdPov=nullptr;const char* fourthPov=nullptr;};
    static constexpr Family families[]{
        {"cineractive/e1m1_intro/khan_maykr_priest_meeting/khan_maykr_priest_meeting_s","010","030"},
        {"cineractive/e1m2_battle/sentinel_king/sentinel_king_s","080","140"},
        {"cineractive/e1m2_battle/sentinel_survivor/sentinel_survivor_s","010","150"},
        {"cineractive/e1m3_cult/revenant_possesion/revenant_possesion_s","010",nullptr},
        {"cineractive/e1m3_cult/revenant_gives_shotgun/revenant_gives_shotgun_s","020",nullptr},
        {"cineractive/e1m4_boss/doom_hunter_intro/doom_hunter_intro_s","010",nullptr},
        {"cineractive/e1m4_boss/second_priest_death/second_priest_death_s","010",nullptr},
        {"cineractive/e2m1_nest/heart_destruction_left/heart_destruction_left_s","010","030"},
        {"cineractive/e2m1_nest/heart_destruction_center/heart_destruction_center_s","010",nullptr},
        {"cineractive/e2m2_base/destroy_turrets_1/destroy_turrets_1_s","010","030"},
        {"cineractive/e2m2_base/confront_samuel/confront_samuel_s","010","180"},
        {"cineractive/e2m3_core/bfg10k_firing/bfg10k_firing_s","010","060","080","100"},
        {"cineractive/e2m3_core/human_cannonball_launcher/human_cannonball_launcher_s","010","030"},
        {"cineractive/e2m3_core/escape_pod/escape_pod_s","010",nullptr},
        {"cineractive/e2m4_boss/hall_entrance/hall_entrance_s","010","110","130"},
        {"cineractive/e2m4_boss/beast_of_the_arena/beast_of_the_arena_s","010","180"},
        {"cineractive/e2m4_boss/third_priest_death/thrid_priest_death_s","010",nullptr},
        {"cineractive/e3m3_maykr/icon_resurrection/icon_resurrection_s","010","050","290"},
        {"cineractive/e3m3_maykr/khan_maykr_fight_stage1_intro/khan_maykr_fight_stage1_intro_s","010",nullptr},
        {"cineractive/e3m3_maykr/khan_maykr_fight_stage2_kill/khan_boss_death_s","010",nullptr},
    };
    for(const auto& family:families) {
        const size_t length=strlen(family.prefix);
        if(animation.compare(0,length,family.prefix)) continue;
        if(animation.size()!=length+10||animation.compare(length+3,7,"_camera")) return FunAnimatedView::Unknown;
        const auto shot=animation.substr(length,3);
        if(shot.find_first_not_of("0123456789")!=std::string::npos) return FunAnimatedView::Unknown;
        return shot==family.firstPov||(family.secondPov&&shot==family.secondPov)||
            (family.thirdPov&&shot==family.thirdPov)||(family.fourthPov&&shot==family.fourthPov)?FunAnimatedView::PlayerPov:FunAnimatedView::External;
    }
    return FunAnimatedView::Unknown;
}
