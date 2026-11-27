#pragma once

struct LabCvarDefinition { const char* name; const wchar_t* setting; const char* request; int initial,minimum,maximum; const wchar_t* source=nullptr; };
static constexpr LabCvarDefinition labCvarDefinitions[]{
    {"labplus_viewer_show",L"Enabled","labplus_viewer",0,0,1},
    {"labplus_triggers_show",L"ShowTriggers","labplus_triggers",1,0,1},
    {"labplus_collision_show",L"ShowCollision","labplus_collision",1,0,1},
    {"labplus_shots_show",L"ShowShotCollision","labplus_shots",0,0,1},
    {"labplus_walls_show",L"ShowWalls","labplus_walls",1,0,1},
    {"labplus_inspector_show",L"InspectVolumes","labplus_inspector",0,0,1},
    {"labplus_disabled_volumes_show",L"ShowDisabledVolumes","labplus_disabled",0,0,1},
    {"labplus_volume_history_show",L"ShowVolumeHistory","labplus_history",0,0,1},
    {"labplus_path_show",L"ShowResearchPath","labplus_path",1,0,1},
    {"labplus_player_show",L"ShowPlayerCollider","labplus_player",0,0,1},
    {"labplus_surface_show",L"ShowSupportHud","labplus_surface",0,0,1},
    {"labplus_attempts_show",L"ShowMotionPaths","labplus_attempts",1,0,1},
    {"labplus_encounters_show",L"ShowActiveEncountersHud","labplus_encounters",0,0,1},
    {"labplus_viewer_distance",L"Distance","labplus_distance",40,20,200},
    {"labplus_viewer_fill",L"CollisionMode","labplus_fill",1,0,1},
    {"labplus_enemies",L"ShowEnemyHealthHud","labplus_enemies",0,0,1},
    {"labplus_enemy_hp_bars",L"ShowEnemyHealthBars","labplus_enemy_hp_bars",0,0,1},
    {"labplus_position",L"ShowPlayerPositionHud","labplus_position",0,0,1},
    {"labplus_angles",L"ShowPlayerAnglesHud","labplus_angles",0,0,1},
    {"labplus_timers",L"ShowGameTimersHud","labplus_timers",0,0,1},
    {"labplus_techs",L"ShowSpeedrunTechsHud","labplus_techs",0,0,1},
    {"labplus_record",L"RecordResearch","labplus_record",0,0,1},
    {"labplus_speedometer_show",L"SpeedometersEnabled","labplus_speedometer_show",0,0,1,L"hud/console.ini"},
    {"labplus_speedometer_showlanding",L"LandingEnabled","labplus_speedometer_showlanding",1,0,1,L"hud/console.ini"},
    {"labplus_speedometer_showvertical",L"VerticalEnabled","labplus_speedometer_showvertical",1,0,1,L"hud/console.ini"},
    {"labplus_speedometer_verticalsign",L"VerticalShowSign","labplus_speedometer_verticalsign",1,0,1,L"hud/console.ini"}
};
static const LabCvarDefinition* findLabCvarDefinition(const std::string& name) {
    for(const auto& definition:labCvarDefinitions) if(name==definition.name) return &definition;
    return nullptr;
}
static int savedLabCvarValue(const LabCvarDefinition& definition,const std::wstring& settings) {
    const auto source=definition.source?(std::filesystem::path(settings).parent_path()/definition.source).wstring():settings;
    int value=static_cast<int>(GetPrivateProfileIntW(L"Viewer",definition.setting,definition.initial,source.c_str()));
    return std::clamp(value,definition.minimum,definition.maximum);
}
