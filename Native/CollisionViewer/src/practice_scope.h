#pragma once
// Exact storage names. Never accept user-supplied path fragments as Steam keys.
struct PracticeSlotScope { const char* slot; const char* folder; unsigned index; const char* family; };
static constexpr PracticeSlotScope practiceSlots[]{
    {"GAME-AUTOSAVE0","AUTOSAVE0",0,"GAME"},{"GAME-AUTOSAVE1","AUTOSAVE1",1,"GAME"},{"GAME-AUTOSAVE2","AUTOSAVE2",2,"GAME"},
    {"DLC1-AUTOSAVE3","AUTOSAVE3",0,"DLC1"},{"DLC1-AUTOSAVE4","AUTOSAVE4",1,"DLC1"},{"DLC1-AUTOSAVE5","AUTOSAVE5",2,"DLC1"},
    {"DLC2-AUTOSAVE6","AUTOSAVE6",0,"DLC2"},{"DLC2-AUTOSAVE7","AUTOSAVE7",1,"DLC2"},{"DLC2-AUTOSAVE8","AUTOSAVE8",2,"DLC2"}
};
static const PracticeSlotScope* practiceSlot(const std::string& slot) {
    for(const auto& scope:practiceSlots) if(slot==scope.slot) return &scope; return nullptr;
}
static const PracticeSlotScope* practiceRuntimeSlot(const std::string& folderName) {
    for(const auto& scope:practiceSlots) if(folderName==scope.folder) return &scope;
    return nullptr;
}
static bool practiceMapMatches(const std::string& level,const PracticeSlotScope& slot) {
    if(std::string(slot.family)=="GAME") return level.rfind("game/sp/",0)==0||level=="game/hub/hub";
    if(std::string(slot.family)=="DLC1") return level.rfind("game/dlc/e4m",0)==0||level=="game/dlc/hub/hub";
    return level.rfind("game/dlc2/e5m",0)==0;
}
static std::string practiceStorageKey(const std::string& slot,size_t index) {
    static const char* names[]{"game.details","game.details-BACKUP","game_duration.dat","game_duration.dat-BACKUP"};
    if(!practiceSlot(slot)||index>=4) throw std::runtime_error("Unsupported campaign slot or storage filename.");
    return slot+"/"+names[index];
}

static bool practiceSlotAvailable(const std::string& slot,unsigned slotCount) {
    for(size_t i=0;i<std::size(practiceSlots);++i) if(slot==practiceSlots[i].slot) return i<slotCount;
    return false;
}
