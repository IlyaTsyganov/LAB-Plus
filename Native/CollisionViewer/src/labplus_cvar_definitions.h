#pragma once

struct LabCvarDefinition { const char* name; const wchar_t* setting; const char* request; int initial,minimum,maximum; };
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
    {"labplus_viewer_fill",L"CollisionMode","labplus_fill",1,0,1}
};
static const LabCvarDefinition* findLabCvarDefinition(const std::string& name) {
    for(const auto& definition:labCvarDefinitions) if(name==definition.name) return &definition;
    return nullptr;
}
static int savedLabCvarValue(const LabCvarDefinition& definition,const std::wstring& settings) {
    int value=static_cast<int>(GetPrivateProfileIntW(L"Viewer",definition.setting,definition.initial,settings.c_str()));
    return std::clamp(value,definition.minimum,definition.maximum);
}
