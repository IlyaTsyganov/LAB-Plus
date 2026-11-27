#pragma once

struct LabCvarDefinition { const char* name; const wchar_t* setting; const char* request; int initial,minimum,maximum; const wchar_t* source=nullptr; const char* description=""; };
static constexpr LabCvarDefinition labCvarDefinitions[]{
    {"labplus_viewer_show",L"Enabled","labplus_viewer",0,0,1,nullptr,"Show the collision viewer and its enabled overlays."},
    {"labplus_triggers_show",L"ShowTriggers","labplus_triggers",1,0,1,nullptr,"Show trigger volumes."},
    {"labplus_collision_show",L"ShowCollision","labplus_collision",1,0,1,nullptr,"Show world geometry that blocks player movement."},
    {"labplus_shots_show",L"ShowShotCollision","labplus_shots",0,0,1,nullptr,"Show geometry that blocks projectiles."},
    {"labplus_walls_show",L"ShowWalls","labplus_walls",1,0,1,nullptr,"Show invisible walls."},
    {"labplus_inspector_show",L"InspectVolumes","labplus_inspector",0,0,1,nullptr,"Show details of the volume under the crosshair."},
    {"labplus_disabled_volumes_show",L"ShowDisabledVolumes","labplus_disabled",0,0,1,nullptr,"Show disabled trigger and wall volumes."},
    {"labplus_volume_history_show",L"ShowVolumeHistory","labplus_history",0,0,1,nullptr,"Show previously observed volumes after they change or disappear."},
    {"labplus_path_show",L"ShowResearchPath","labplus_path",1,0,1,nullptr,"Show recorded research paths."},
    {"labplus_player_show",L"ShowPlayerCollider","labplus_player",0,0,1,nullptr,"Show the player's collision shape."},
    {"labplus_surface_show",L"ShowSupportHud","labplus_surface",0,0,1,nullptr,"Show support surface and player contact information."},
    {"labplus_attempts_show",L"ShowMotionPaths","labplus_attempts",1,0,1,nullptr,"Show recorded movement attempts."},
    {"labplus_encounters_show",L"ShowActiveEncountersHud","labplus_encounters",0,0,1,nullptr,"Show active encounters and their state in the HUD."},
    {"labplus_viewer_distance",L"Distance","labplus_distance",40,20,200,nullptr,"Maximum viewer draw distance in metres."},
    {"labplus_viewer_fill",L"CollisionMode","labplus_fill",1,0,1,nullptr,"Render filled collision surfaces (1) or outlines (0)."},
    {"labplus_enemies_show",L"ShowEnemyHealthHud","labplus_enemies",0,0,1,nullptr,"Show living enemies and their health in the HUD."},
    {"labplus_enemy_hp_bars_show",L"ShowEnemyHealthBars","labplus_enemy_hp_bars",0,0,1,nullptr,"Show health bars above enemies."},
    {"labplus_position_show",L"ShowPlayerPositionHud","labplus_position",0,0,1,nullptr,"Show player position coordinates in the debug HUD."},
    {"labplus_angles_show",L"ShowPlayerAnglesHud","labplus_angles",0,0,1,nullptr,"Show player view angles in the debug HUD."},
    {"labplus_timers_show",L"ShowGameTimersHud","labplus_timers",0,0,1,nullptr,"Show equipment, powerup and animation countdowns."},
    {"labplus_techs_show",L"ShowSpeedrunTechsHud","labplus_techs",0,0,1,nullptr,"Show detected speedrun technique states."},
    {"labplus_record",L"RecordResearch","labplus_record",0,0,1,nullptr,"Record research samples (1) or stop recording (0)."},
    {"labplus_speedometer_show",L"SpeedometersEnabled","labplus_speedometer_show",0,0,1,L"hud/console.ini","Enable the speedometer display."},
    {"labplus_speedometer_landing_show",L"LandingEnabled","labplus_speedometer_landing_show",1,0,1,L"hud/console.ini","Show landing horizontal speed, including the brief impact peak."},
    {"labplus_speedometer_vertical_show",L"VerticalEnabled","labplus_speedometer_vertical_show",1,0,1,L"hud/console.ini","Show vertical movement speed."},
    {"labplus_speedometer_sign_vertical",L"VerticalShowSign","labplus_speedometer_sign_vertical",1,0,1,L"hud/console.ini","Show signed vertical speed (1) or its absolute magnitude (0)."}
};
static const char* const* labCvarValueCompletions(const LabCvarDefinition& definition) {
    // The engine keeps this pointer. These are cvar values; the separate
    // engine command "toggle" must never be offered as a value argument.
    static const char* const booleanValues[]{"0","1",nullptr};
    return definition.minimum==0&&definition.maximum==1?booleanValues:nullptr;
}
static const std::vector<LabCvarDefinition>& nativeLabCvarDefinitions() {
    static const auto definitions=[] {
        std::vector<LabCvarDefinition> result(std::begin(labCvarDefinitions),std::end(labCvarDefinitions));
        for(auto definition:labCvarDefinitions) {
            // Preserve short setting names as real cvars too, so existing
            // "toggle labplus_viewer" binds work. The encounter alias also
            // lists entities without arguments and must remain a command.
            if(definition.maximum!=1||!strcmp(definition.name,definition.request)||!strcmp(definition.request,"labplus_encounters")) continue;
            definition.name=definition.request; result.push_back(definition);
        }
        return result;
    }();
    return definitions;
}
static const LabCvarDefinition* findLabCvarDefinition(const std::string& name) {
    for(const auto& definition:nativeLabCvarDefinitions()) if(name==definition.name) return &definition;
    return nullptr;
}
static int savedLabCvarValue(const LabCvarDefinition& definition,const std::wstring& settings) {
    const auto source=definition.source?(std::filesystem::path(settings).parent_path()/definition.source).wstring():settings;
    int value=static_cast<int>(GetPrivateProfileIntW(L"Viewer",definition.setting,definition.initial,source.c_str()));
    return std::clamp(value,definition.minimum,definition.maximum);
}
