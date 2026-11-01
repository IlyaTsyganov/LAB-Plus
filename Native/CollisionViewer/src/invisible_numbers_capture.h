#pragma once
static InvisibleNumbersSnapshot captureInvisibleNumbers(const WallMemory& memory,uintptr_t base,const GameProfile& profile,
    const LevelIdentity& level,const GraphLookup& lookup,unsigned features) {
    InvisibleNumbersSnapshot result; result.captured=encounterEpochMillis(); result.level=level; result.phase="live";
    EnemyTelemetryCapture enemies; enemies.state="disabled";
    if(features&9) enemies=captureEnemyTelemetry(memory,base,profile,level,lookup,(features&8)!=0);
    result.enemiesState=enemies.state; result.detail=enemies.detail; result.unavailable=enemies.unavailable;
    result.enemies=std::move(enemies.enemies); result.barEnemies=std::move(enemies.barEnemies);
    if(features&8) {
        const auto* flame=timerField(lookup,"idPlayer","flameBelchNextUseTime");
        if(timerTimeField(flame)) {
            result.gameClockFlameOffset=flame->offset;
            const auto clock=readGameTimerClock(memory,base,profile,level,flame->offset);
            for(auto& row:result.barEnemies) refreshEnemyStagger(memory,base,profile,row,clock.valid?clock.ticks:-1);
        }
    }
    GameTimerSnapshot timers; timers.phase="disabled";
    if(features&2) timers=captureGameTimers(memory,base,profile,level,lookup);
    result.timersState=timers.phase;
    for(auto& row:timers.rows) {
        bool known=std::isfinite(row.remaining)&&row.remaining>=0;
        if(!known&&(row.state=="active"||row.state=="cooldown")) {
            row.detail=row.state+"; "+row.detail; row.state="unavailable";
        }
        result.timers.push_back({std::move(row.key),std::move(row.group),std::move(row.label),std::move(row.state),std::move(row.detail),known?row.remaining:0,known,
            std::isfinite(row.maximum)&&row.maximum>0?row.maximum:-1});
    }
    SpeedrunTechSnapshot techs;
    if(features&4) techs=captureSpeedrunTechs(memory,base,profile,level,lookup);
    for(auto& row:techs.rows) result.techs.push_back({std::move(row.key),std::move(row.label),std::move(row.state),std::move(row.detail)});
    if(enemies.state=="loading"||timers.phase=="loading") { result.phase="loading"; result.enemies.clear(); result.barEnemies.clear(); result.timers.clear(); result.techs.clear(); }
    return result;
}
