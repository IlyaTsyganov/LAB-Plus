#include "../src/vulkan_layer.cpp"
#include <limits>
static void expect(bool condition,const char* message) { if(!condition) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
int main() {
    enabled=false;
    expect(!nativeOverlayNeeded(),"disabled geometry and hidden HUD do not request Vulkan drawing");
    for(auto* option:{&showEnemyHealthHud,&showGameTimersHud,&showSpeedrunTechsHud,&showSupportHud,&showActiveEncountersHud}) {
        option->store(true); expect(nativeOverlayNeeded(),"each text HUD renders with collision geometry disabled"); option->store(false);
    }
    enabled=true;
    for(const char* command:{"labplus_enemies","labplus_enemy_hp_bars","labplus_timers","labplus_techs"}) {
        expect(validLabArguments({command,"1"})&&validLabArguments({command,"0"})&&!validLabArguments({command,"2"}),"number feature actions enforce0/1");
    }
    LevelIdentity level; level.map=0x20000; level.world=0x30000; level.name="arena";
    InvisibleNumbersSnapshot snapshot; snapshot.level=level; snapshot.phase="live"; snapshot.captured=1000;
    snapshot.enemiesState="ready"; snapshot.timersState="ready";
    InvisibleEnemyRow enemy; enemy.entity={0x40000,level.map,0x50000,1,2,"idAI2"}; enemy.name="imp";
    enemy.encounter="Arena"; enemy.encounterId=3; enemy.encounterGeneration=4;
    enemy.positionKnown=enemy.maximumKnown=true; enemy.health=100; enemy.maximum=200; enemy.top={100,100,100};
    snapshot.barEnemies.push_back(enemy);
    GameCamera camera{}; camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;

    auto fresh=[](const InvisibleEnemyRow&,Vec3& point) { point={1,4,2}; return true; };
    auto geometry=buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,fresh);
    expect(geometry.bars==1&&!geometry.triangles.empty(),"health panel uses filled world triangles");
    expect(geometry.triangles.front().position.x>.5f&&geometry.triangles.front().position.x<1.f&&geometry.triangles.front().position.y==4,"bar uses refreshed pose instead of cached position");
    snapshot.barEnemies.push_back(enemy);
    expect(buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,fresh).bars==1,"enemy shared by managers has one bar");
    expect(!buildEnemyHealthBars(camera,snapshot,level,1251,40,1080,fresh).bars,"stale bars expire after250ms");
    auto other=level; other.world++;
    expect(!buildEnemyHealthBars(camera,snapshot,other,1000,40,1080,fresh).bars,"foreign level bars never draw");
    expect(!buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,[](const auto&,Vec3&) { return false; }).bars,"failed identity/pose validation hides bar");
    expect(!buildEnemyHealthBars(camera,snapshot,level,1000,1,1080,fresh).bars,"distant bars are culled");
    snapshot.barEnemies.resize(1);
    for(double health:{0.,-1.,std::numeric_limits<double>::quiet_NaN()}) {
        snapshot.barEnemies[0].health=health;expect(!buildEnemyHealthBars(camera,snapshot,level,1000,40,1080,fresh).bars,"dead/invalid health never renders");
    }
    snapshot.barEnemies[0].health=100;
    expect(cleanNumberLabel("100%\t^1imp")=="100???1imp","engine formatting characters are escaped");
    snapshot.enemies=snapshot.barEnemies;
    snapshot.syncAction=1;snapshot.syncTarget="1:2";snapshot.syncAnimations={"cut_front","cut_back"};snapshot.syncAutomaticAnimation="cut_back";
    snapshot.syncHints={{"cut_front","Hell Knight","No specific body part"},{"cut_back","Hell Knight","No specific body part"}};
    snapshot.timers.push_back({"flame_belch","Equipment","Flame belch","cooldown","",1.25,true,20});
    snapshot.timers.push_back({"sticky_bomb","Weapon mods","Sticky Bomb","cooldown","",.25,true,1.5});
    snapshot.timers.push_back({"sticky_charge_1","sticky_charges","Charge 1","cooldown","",.25,true,1.5});
    snapshot.timers.push_back({"sticky_charge_2","sticky_charges","Charge 2","cooldown","",1.75,true,3});
    auto timerLines=timerHudLines(snapshot.timers);
    expect(timerLines.size()==3&&timerLines[2].find("[1: 0.25 / 1.50 s]   [2: 1.75 / 3.00 s]")!=std::string::npos,"per-charge countdowns sit to the right of Sticky Bomb without duplicate child rows");
    for(unsigned index=0;index<30;++index) snapshot.timers.push_back({"test"+std::to_string(index),"Equipment","Test","cooldown","",1,true,10});
    expect(timerHudLines(snapshot.timers).size()==33,"timer HUD has no fixed twenty-row truncation");
    wchar_t temp[MAX_PATH]{},path[MAX_PATH]{};expect(GetTempPathW(MAX_PATH,temp)&&GetTempFileNameW(temp,L"num",0,path),"temporary test path");
    DeleteFileW(path);expect(writeInvisibleNumbers(snapshot,path),"actual numbers protocol writes atomically");
    auto file=std::filesystem::path(path)/(L"invisible-numbers-"+std::to_wstring(snapshot.process)+L".tsv");
    std::ifstream input(file,std::ios::binary); std::string protocol((std::istreambuf_iterator<char>(input)),{}); input.close();
    expect(protocol.find("#LABPLUS_NUMBERS_V1\t")==0&&protocol.find("\t100\t200\n")!=std::string::npos&&protocol.find("\tcooldown\t1.25\t\t20\n")!=std::string::npos,"native protocol carries exact HP and invariant countdown/maximum values");
    expect(protocol.find("P\t1\t1:2\tcut_back\nA\tcut_front\nA\tcut_back\n")!=std::string::npos,"native worker protocol carries resolved automatic choice, animation choices and target identity");
    expect(protocol.find("H\tcut_front\tHell Knight\tNo specific body part\n")!=std::string::npos,"per-enemy animation guidance is published separately from its stable selection key");
    DeleteFileW(file.c_str());RemoveDirectoryW(path);
    puts("PASS: enemy bar depth geometry, refreshed pose, identity/age/level gates, deduplication and numbers protocol.");
}
