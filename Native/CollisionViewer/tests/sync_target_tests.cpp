#include "../src/vulkan_layer.cpp"
#include "../src/sync_distance_map.h"
#include <limits>
static void requireTarget(bool pass,const char* message) {if(!pass){fprintf(stderr,"FAIL: %s\n",message);exit(1);}}
template<class T> static void targetPut(std::vector<unsigned char>& bytes,size_t offset,const T& value) {memcpy(bytes.data()+offset,&value,sizeof(value));}
static uintptr_t targetAddress(std::vector<unsigned char>& bytes) {return reinterpret_cast<uintptr_t>(bytes.data());}
#include "sync_enemy_display_checks.h"
#include "sync_limb_position_checks.h"
#include "sync_authored_preview_checks.h"
#include "sync_auto_retention_checks.h"
#include "sync_limb_multiview_checks.h"
#include "sync_zombie_soldier_checks.h"
#include "sync_body_availability_checks.h"
#include "sync_body_colour_checks.h"
#include "sync_body_sector_checks.h"
#include "sync_distance_checks.h"
#include "sync_body_all_zones_checks.h"
#include "sync_carcass_nearest_checks.h"
#include "sync_all_nearest_checks.h"
struct HitTargetFixture {
    std::vector<unsigned char> actor=std::vector<unsigned char>(0x6500),physics=std::vector<unsigned char>(0x250),wrapper=std::vector<unsigned char>(0x58),
        root=std::vector<unsigned char>(112),child=std::vector<unsigned char>(112),leaf=std::vector<unsigned char>(64),materials=std::vector<unsigned char>(176),group=std::vector<unsigned char>(120),targeting=std::vector<unsigned char>(208);
    EntityReference ref;
    explicit HitTargetFixture(uintptr_t base=0x10000000,uintptr_t map=0x20000) {
        ref={targetAddress(actor),map,0,1,1,"idAI2"};
        targetPut(actor,0xbd0,targetAddress(targeting));targetPut(targeting,0xc4,uint32_t(1));
        targetPut(actor,0x1a28,targetAddress(wrapper));targetPut(actor,0x1a38,targetAddress(physics));
        targetPut(physics,0x70,map);targetPut(physics,0x238,targetAddress(wrapper));targetPut(physics,0x240,int64_t(10));
        float axis[9]{1,0,0,0,1,0,0,0,1};memcpy(physics.data()+0x8c,axis,sizeof(axis));
        targetPut(physics,0xb0,Vec3{0,4,0});float bounds[6]{-1,3,-1,1,5,1};memcpy(physics.data()+0xe0,bounds,24);
        targetPut(wrapper,0,targetAddress(root));targetPut(wrapper,8,SyncPreviewList{targetAddress(materials),2,2,0});
        root[0x22]=13;targetPut(root,0x50,targetAddress(child));targetPut(root,0x58,uint32_t(1));
        targetPut(child,0x50,targetAddress(leaf));float scale[3]{1,1,1};memcpy(child.data()+0x40,scale,12);
        targetPut(leaf,0,base+0x2f265e8);leaf[0x22]=2;targetPut(leaf,0x28,1.f);targetPut(leaf,0x30,targetAddress(materials)+88);
        targetPut(materials,88+8,uint32_t(0x40080));targetPut(materials,88+0x50,targetAddress(group));targetPut(group,8,reinterpret_cast<uintptr_t>("head"));
    }
};
struct TargetCaptureFixture {
    unsigned char* image=nullptr;uintptr_t base=0;
    std::vector<unsigned char> system=std::vector<unsigned char>(128),map=std::vector<unsigned char>(0x61cd0),game=std::vector<unsigned char>(0x136288),
        player=std::vector<unsigned char>(0x7710),view=std::vector<unsigned char>(0x178),info=std::vector<unsigned char>(0x200),resolved=std::vector<unsigned char>(16),entries=std::vector<unsigned char>(64);
    std::array<HitTargetFixture,2> enemies;LevelIdentity level;
    TargetCaptureFixture() {
        image=static_cast<unsigned char*>(VirtualAlloc(nullptr,kProfile11.imageSize,MEM_RESERVE,PAGE_READWRITE));requireTarget(image!=nullptr,"reserve fixture image");base=reinterpret_cast<uintptr_t>(image);
        auto commit=[&](size_t at){requireTarget(VirtualAlloc(image+(at&~size_t(4095)),4096,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit fixture page");};
        for(size_t at:{size_t(0x1000),size_t(0x2000),size_t(0x3000),size_t(0x6128718),size_t(kProfile11.gameSystemRva),size_t(0x1927801),size_t(0x1db9d60),size_t(0x18db396)})commit(at);
        const unsigned char manager[]{0x48,0x8b,0x0d,0x10,0x0f,0x80,0x04},los[]{0x0f,0xb6,0x81,0xf8,0,0,0,0x83,0xe0,1,0xc3};
        memcpy(image+0x1927801,manager,sizeof(manager));memcpy(image+0x1db9d60,los,sizeof(los));
        const unsigned char chainsawBypass[]{0x74,0x52,0x83,0xb8,0x9c,0,0,0,0x10,0x74,0x49};memcpy(image+0x18db396,chainsawBypass,sizeof(chainsawBypass));
        const auto systemAddress=targetAddress(system),gameAddress=targetAddress(game);memcpy(image+kProfile11.gameSystemRva,&systemAddress,8);memcpy(image+0x6128718,&gameAddress,8);
        level={targetAddress(map),0x20000,"test/arena"};targetPut(system,0x50,level.map);targetPut(map,0x1ad0,targetAddress(player));
        targetPut(player,0,base+kProfile11.playerVtableRva);targetPut(map,0x61cb0,SyncPreviewList{targetAddress(entries),2,2,0});
        targetPut(game,0x136180,SyncPreviewList{targetAddress(view),1,1,0});targetPut(view,0xc8,targetAddress(player));
        targetPut(view,0x60,SyncPreviewList{targetAddress(info),2,2,0});targetPut(view,0x140,SyncPreviewList{targetAddress(resolved),2,2,0});
        const auto getter=base+0x2000,name=reinterpret_cast<uintptr_t>("idAI2");memcpy(image+0x1010,&getter,8);memcpy(image+0x3000,&name,8);
        const unsigned char prefix[]{0x48,0x8d,0x05};memcpy(image+0x2000,prefix,3);int32_t relative=0x3000-(0x2000+7);memcpy(image+0x2003,&relative,4);image[0x2007]=0xc3;
        for(unsigned i=0;i<2;++i) {
            auto& enemy=enemies[i];enemy.ref.map=level.map;targetPut(enemy.actor,0,base+0x1000);targetPut(enemy.physics,0x70,level.map);targetPut(enemy.leaf,0,base+0x2f265e8);
            targetPut(enemy.physics,0xb0,Vec3{0,4+float(i)*4,0});float bounds[6]{-1,3+float(i)*4,-1,1,5+float(i)*4,1};memcpy(enemy.physics.data()+0xe0,bounds,24);
            targetPut(resolved,i*8,targetAddress(enemy.actor));info[i*0x100+0xf8]=0x41;
            targetPut(info,i*0x100+0xd0,uint32_t(i+1));
            targetPut(entries,i*32,uint32_t(i+1));targetPut(entries,i*32+4,uint32_t(1));targetPut(entries,i*32+8,targetAddress(enemy.actor));
        }
    }
    ~TargetCaptureFixture(){if(image)VirtualFree(image,0,MEM_RELEASE);}
};
#include "sync_retained_capture_checks.h"
static void checkSyncPlayerContext(TargetCaptureFixture& scene) {
    {std::vector<unsigned char> rows(48),list(24);targetPut(rows,0,uint64_t(2));targetPut(rows,12,9.f);targetPut(rows,24,uint64_t(6));targetPut(rows,36,12.f);
        targetPut(list,0,SyncPreviewList{targetAddress(rows),2,2,0});float maximum=6;bool matched=false;
        requireTarget(syncRangeMonsterLimit(WallMemory{},targetAddress(list),8,maximum,matched)&&!matched&&maximum==6,"a monster without an override retains native base lunge distance instead of losing GK support");
        requireTarget(syncRangeMonsterLimit(WallMemory{},targetAddress(list),2,maximum,matched)&&matched&&maximum==9,"native monster range uses the first matching row even when later masks also match");}
    const WallMemory memory;std::vector<unsigned char> bodyPlayer(0xc600);const auto player=targetAddress(bodyPlayer),physics=player+0x7d28;
    targetPut(bodyPlayer,0x1d0,physics);targetPut(bodyPlayer,0x7d28+0x14,uint32_t(5));targetPut(bodyPlayer,0x7d28+0x70,scene.level.map);
    const Vec3 origin{3,4,5};targetPut(bodyPlayer,0x7d28+0xb0,origin);const float axis[9]{1,0,0,0,1,0,0,0,1};memcpy(bodyPlayer.data()+0x7d28+0x8c,axis,36);
    float local[6]{-.395f,-.395f,0,.395f,.395f,1.79f},world[6]{2.605f,3.605f,5,3.395f,4.395f,6.79f};
    memcpy(bodyPlayer.data()+0x7d28+0xc8,local,24);memcpy(bodyPlayer.data()+0x7d28+0xe0,world,24);
    auto footprint=readSyncPlayerFootprint(memory,player,scene.level.map);
    requireTarget(footprint.known&&fabsf(footprint.minimum.x+.395f)<1e-6f&&fabsf(footprint.maximum.y-.395f)<1e-6f,"player footprint comes from coherent native collider bounds, not an invented fixed radius");
    local[0]=local[1]=-.28f;local[3]=local[4]=.28f;local[5]=1.2f;world[0]=2.72f;world[1]=3.72f;world[3]=3.28f;world[4]=4.28f;world[5]=6.2f;
    memcpy(bodyPlayer.data()+0x7d28+0xc8,local,24);memcpy(bodyPlayer.data()+0x7d28+0xe0,world,24);targetPut(bodyPlayer,0x7d28+0x3cc0,int(1));
    footprint=readSyncPlayerFootprint(memory,player,scene.level.map);requireTarget(footprint.known&&fabsf(footprint.maximum.x-.28f)<1e-6f,"shape and stance changes update the footprint bounds");
    targetPut(bodyPlayer,0x7d28+0xe0,99.f);requireTarget(!readSyncPlayerFootprint(memory,player,scene.level.map).known,"inconsistent local/world bounds do not authorize a clearance colour");
    for(size_t at:{size_t(0x5d721d),size_t(0x61a9db0)})requireTarget(VirtualAlloc(scene.image+(at&~size_t(4095)),4096,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit context code/cvar fixture pages");
    const unsigned char groupCode[]{0x48,0x63,0x44,0xd1,0x08};memcpy(scene.image+0x5d721d,groupCode,5);
    std::vector<unsigned char> historyCvar(64),countCvar(64),randomCvar(64),focusPlayer(0x16000),animator(8),model(0x300),groupPointers(16),head(0x78),chest(0x78),jointIds(4),historyIds(16);
    auto cvar=[&](size_t at,std::vector<unsigned char>& object,const char* name,int value){const auto address=targetAddress(object);memcpy(scene.image+at,&address,8);targetPut(object,40,reinterpret_cast<uintptr_t>(name));targetPut(object,8,value);};
    cvar(0x61a9db0,historyCvar,"sync_UseFocusHistory",0);cvar(0x61a9d30,countCvar,"player_focusMaxHistory",4);cvar(0x61a9cb0,randomCvar,"sync_semiRandom",0);
    HitTargetFixture victim(scene.base,scene.level.map);targetPut(victim.actor,0x1a20,targetAddress(animator));targetPut(animator,0,targetAddress(model));
    targetPut(model,0xe0,SyncPreviewList{targetAddress(groupPointers),2,2,0});targetPut(groupPointers,0,targetAddress(head));targetPut(groupPointers,8,targetAddress(chest));
    targetPut(head,8,reinterpret_cast<uintptr_t>("head"));targetPut(chest,8,reinterpret_cast<uintptr_t>("chest"));targetPut(jointIds,0,short(37));targetPut(jointIds,2,short(31));
    targetPut(head,0x10,SyncPreviewList{targetAddress(jointIds),1,1,0});targetPut(chest,0x10,SyncPreviewList{targetAddress(jointIds),2,2,0});
    targetPut(focusPlayer,0x15c2c,int(4));targetPut(focusPlayer,0x15c5c,int(1));targetPut(focusPlayer,0x15c60,int(37));targetPut(focusPlayer,0x15c64,victim.ref.id);
    auto focus=readSyncJointFocus(memory,scene.base,targetAddress(focusPlayer),victim.ref);
    requireTarget(focus.known&&focus.group=="head","cached native focus contact maps the joint through the first matching native damage group");
    std::vector<SyncHitSphere> mapped(4);mapped[0].jointIndex=37;mapped[1].jointIndex=31;mapped[2].jointIndex=99;mapped[3].jointIndex=-1;
    for(auto& s:mapped)s.group="misleading_material_label";
    requireTarget(readSyncSphereFocusGroups(memory,scene.base,victim.ref,mapped)&&mapped[0].focusKnown&&mapped[0].focusGroup=="head"&&mapped[1].focusGroup=="chest"&&mapped[2].focusKnown&&mapped[2].focusGroup.empty()&&mapped[3].focusKnown&&mapped[3].focusGroup.empty(),
        "visual group lookup uses the material joint ID and first native type-0 membership, including an unmapped joint, instead of the material label");
    targetPut(animator,0,uintptr_t(0));requireTarget(!readSyncSphereFocusGroups(memory,scene.base,victim.ref,mapped)&&!mapped[0].focusKnown,"a missing model clears previously mapped visual groups");targetPut(animator,0,targetAddress(model));
    targetPut(focusPlayer,0x15c64,uint32_t(99));requireTarget(!readSyncJointFocus(memory,scene.base,targetAddress(focusPlayer),victim.ref).known,"another victim's cached contact cannot supply focus for the selected enemy");targetPut(focusPlayer,0x15c64,victim.ref.id);
    targetPut(randomCvar,8,int(1));requireTarget(!readSyncJointFocus(memory,scene.base,targetAddress(focusPlayer),victim.ref).known,"unsupported native fallback cannot be replaced with the first sphere hit");targetPut(randomCvar,8,int(0));
    targetPut(historyCvar,8,int(1));const int entries[4]{31,37,37,37};memcpy(historyIds.data(),entries,16);targetPut(focusPlayer,0x7768,SyncPreviewList{targetAddress(historyIds),4,4,0});
    focus=readSyncJointFocus(memory,scene.base,targetAddress(focusPlayer),victim.ref);requireTarget(focus.known&&focus.group=="head","enabled native history mode uses the audited history winner and group lookup");
    const uintptr_t zero=0;for(size_t at:{size_t(0x61a9db0),size_t(0x61a9d30),size_t(0x61a9cb0)})memcpy(scene.image+at,&zero,8);
}
static void checkAimPartHighlights() {
    GameCamera camera{};camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    const float box[]{-1.5f,3.f,-1.2f,1.5f,5.5f,1.8f};SyncAimFrame aim;aim.known=true;memcpy(aim.bounds,box,24);
    std::vector<SyncHitSphere> parts;
    auto add=[&](Vec3 center,float size,const char* group){SyncHitSphere s;s.center=center;s.radius=size;s.group="not_the_focus_label";s.focusKnown=true;s.focusGroup=group;parts.push_back(s);};
    add({0,4,1.15f},.42f,"head");add({0,4,0},.62f,"chest");add({-1.05f,4,.2f},.3f,"left_arm");add({.7f,4,-.8f},.3f,"right_leg");
    SyncAimRequirement rule;rule.damageGroups={"head"};
    auto verify=[&](const SyncAimPartMesh& mesh,const SyncAimRequirement& required){requireTarget(mesh.known&&!mesh.fill.empty()&&!mesh.outline.empty(),"a visible required body part produces a filled mask and boundary on the target box");
        requireTarget(mesh.fill.size()<=39*51*6&&mesh.outline.size()<mesh.fill.size(),"body masks have bounded geometry and no internal grid outlines");
        // Independent dense rays inside every rendered triangle must hit an
        // allowed group first. This detects bridges, occlusion and bad clipping.
        for(size_t i=0;i<mesh.fill.size();i+=3)for(unsigned a=0;a<=4;++a)for(unsigned b=0;b<=4-a;++b){
            const auto p=motionAdd(motionScale(mesh.fill[i],float(a)/4),motionAdd(motionScale(mesh.fill[i+1],float(b)/4),motionScale(mesh.fill[i+2],float(4-a-b)/4)));
            auto ray=subtract(p,{camera.position[0],camera.position[1],camera.position[2]});ray=motionScale(ray,1/sqrtf(dot(ray,ray)));float nearest=FLT_MAX;const SyncHitSphere* first=nullptr;
            for(const auto& sphere:parts)if(syncSphereHit({camera.position[0],camera.position[1],camera.position[2]},ray,sphere,nearest))first=&sphere;
            requireTarget(first&&std::find(required.damageGroups.begin(),required.damageGroups.end(),first->focusGroup)!=required.damageGroups.end(),"every highlighted interior ray first reaches a required native joint group");
            double x,y,w;requireTarget(syncAimProject(camera,p,x,y,w),"mask vertices project");if(required.quadrants==1)requireTarget(x<0.00001&&y>=.06-0.00001,"anatomical mask intersects the native physics-centre quadrant");}
    };
    auto head=syncBuildAimPartMesh(camera,box,aim,&rule,parts);verify(head,rule);
    double lowHead=DBL_MAX;for(auto p:head.fill){double x,y,w;syncAimProject(camera,p,x,y,w);lowHead=std::min(lowHead,y);}requireTarget(lowHead>.17,"head guidance does not fill the whole upper half or unrestricted box");
    for(const char* group:{"left_arm","right_leg","chest"}){rule.damageGroups={group};verify(syncBuildAimPartMesh(camera,box,aim,&rule,parts),rule);}
    rule.damageGroups={"head","left_arm"};verify(syncBuildAimPartMesh(camera,box,aim,&rule,parts),rule);
    rule.damageGroups={"head"};rule.quadrants=1;aim.bounds[2]+=.3f;aim.bounds[5]+=.3f;verify(syncBuildAimPartMesh(camera,box,aim,&rule,parts),rule);
    rule.quadrants=0;const auto before=syncBuildAimPartMesh(camera,box,aim,&rule,parts);parts[0].center.x=.6f;const auto after=syncBuildAimPartMesh(camera,box,aim,&rule,parts);verify(after,rule);
    double oldX=0,newX=0;for(auto p:before.fill){double x,y,w;syncAimProject(camera,p,x,y,w);oldX+=x;}for(auto p:after.fill){double x,y,w;syncAimProject(camera,p,x,y,w);newX+=x;}
    requireTarget(newX/after.fill.size()>oldX/before.fill.size()+.1,"highlight follows current animated part pose instead of staying on a fixed guessed box half");
    parts[0].center={0,4,0};parts[0].radius=.3f;auto hidden=syncBuildAimPartMesh(camera,box,aim,&rule,parts);requireTarget(hidden.known&&hidden.fill.empty(),"head behind the chest is not highlighted through an unrelated body part");
    parts[0].center={0,3.15f,0};verify(syncBuildAimPartMesh(camera,box,aim,&rule,parts),rule);
    parts[1].focusKnown=false;requireTarget(!syncBuildAimPartMesh(camera,box,aim,&rule,parts).known,"unknown group membership hides anatomical guidance rather than inventing a target");parts[1].focusKnown=true;
    rule.damageGroups={"absent"};const auto absent=syncBuildAimPartMesh(camera,box,aim,&rule,parts);requireTarget(absent.known&&absent.fill.empty(),"a missing requested part never highlights the entire box");
    rule.damageGroups.clear();requireTarget(!syncBuildAimPartMesh(camera,box,aim,&rule,parts).required,"unrestricted anatomy leaves existing quadrant-only behaviour intact");
    rule.damageGroups={"head","chest","left_arm","right_leg"};
    requireTarget(syncBodyAimKind(&rule,parts)==SyncBodyAimKind::AllCurrentGroups&&!syncBuildAimPartMesh(camera,box,aim,&rule,parts).required,
        "a rule accepting every current hit group is not presented as a special limb aiming requirement");
    parts[1].focusKnown=false;requireTarget(syncBodyAimKind(&rule,parts)==SyncBodyAimKind::Unknown,"partial model mappings cannot be declared unrestricted anatomy");parts[1].focusKnown=true;
    rule.damageGroups={"head"};std::vector<Vertex> fill;appendSyncAimBox(camera,box,aim,&rule,fill,false);requireTarget(fill.empty(),"joint-group requirements cannot leave a misleading full-quadrant fill beneath the anatomical mask");
}
static void checkNativeChoiceAudit() {
    // Read-only Hell Knight 1.1 snapshot. These are native selector sites,
    // not guessed front/back points or a replay of the reported execution.
    SyncTargetFrame knight;knight.action=1;knight.selectionKnown=true;knight.candidatesReady=true;knight.selectionTolerance=.381000012f;
    knight.ranges.playerOrigin={-3.72072434f,-85.1501389f,20.6000042f};
    knight.chainsaw={"cut_front_01","cut_back"};knight.selectedAnimation="cut_front_01";
    knight.nativeChoices={{"cut_front_01",1,1.25f,{-2.95662451f,-82.4016724f,20.6000023f},true},{"cut_back",1,1.25f,{-4.99406385f,-82.40168f,20.6000042f},true}};
    const auto possible=syncNativeSelection(knight);
    requireTarget(possible.known&&possible.possible==std::vector<std::string>({"cut_back","cut_front_01"}),"observed Hell Knight sites retain BOTH valid cuts in the native random band even outside a unique clearance area");
    auto messages=syncPositionGuideLines(knight);requireTarget(std::find(messages.begin(),messages.end(),"Selected: Front 01")!=messages.end(),"compact HUD keeps the selected animation without verbose candidate diagnostics");
    knight.nativeChoices[1].priority=2;auto ranked=syncNativeSelection(knight);
    requireTarget(ranked.known&&ranked.possible==std::vector<std::string>{"cut_back"},"native priority outranks which start pose is nearer");
    knight.pendingChainsaw=true;requireTarget(!syncNativeSelection(knight).known,"pending engine checks cannot claim a sole selector result");knight.pendingChainsaw=false;
    knight.nativeChoices[1].known=false;requireTarget(!syncNativeSelection(knight).known,"missing data for a ready competing candidate makes the choice set unknown");knight.nativeChoices[1].known=true;
    knight.nativeChoices.pop_back();requireTarget(!syncNativeSelection(knight).known,"an eligible name missing from selector rows cannot disappear from competition");
    knight.action=0;knight.gloryKills={"kill_near","kill_far"};knight.nativeChoices={{"kill_near",0,0,{knight.ranges.playerOrigin.x+1,knight.ranges.playerOrigin.y,knight.ranges.playerOrigin.z},true},{"kill_far",0,0,{knight.ranges.playerOrigin.x+2,knight.ranges.playerOrigin.y,knight.ranges.playerOrigin.z},true}};
    requireTarget(syncNativeSelection(knight).possible==std::vector<std::string>{"kill_near"},"GK uses the same audited current priority/distance selection as Chainsaw");
    knight.nativeChoices[1].priority=1;requireTarget(syncNativeSelection(knight).possible==std::vector<std::string>{"kill_far"},"higher-priority GK wins over the nearest lower-priority pose");
    knight.selectionKnown=false;requireTarget(!syncNativeSelection(knight).known,"unsupported selector transform branch stays unknown");
}
static void checkEnemyRuleInventory() {
    std::ifstream fixture("Native/CollisionViewer/tests/fixtures/sync-enemy-rules-11.tsv");requireTarget(bool(fixture),"per-enemy authored inventory fixture is present");
    std::unordered_set<std::string> types,partTypes;size_t gk=0,cs=0,hkCuts=0;bool knightHead=false,zombieBack=false;
    std::string line;while(std::getline(fixture,line)){if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);std::string enemy,state,action,groups;uint32_t quadrant=0;float ranges[4],pitch,yaw;int centric[5];
        fields>>enemy>>state>>action>>quadrant>>groups;for(auto& v:ranges)fields>>v;fields>>pitch>>yaw;for(auto& v:centric)fields>>v;
        requireTarget(!fields.fail(),"enemy rule inventory row parses");types.insert(enemy);
        if(action=="GK"){++gk;if(groups!="-")partTypes.insert(enemy);if(enemy=="hellknight")requireTarget(groups=="-","Hell Knight's ordinary GK names never imply anatomical targeting");}
        else {requireTarget(action=="CS","known inventory action");++cs;}
        if(enemy=="hellknight"&&state=="kill_front_head"){knightHead=true;requireTarget(quadrant==3&&groups=="-","Hell Knight front_head is an upper-quadrant requirement, not native head focus");}
        if(enemy=="zombie_tier3"&&state=="kill_back_head"){zombieBack=true;requireTarget(quadrant==3&&groups=="-","Zombie tier3 back_head does not inherit front_head's anatomical requirement");}
        if(enemy=="hellknight"&&action=="CS"){++hkCuts;requireTarget(groups.find("left_lower_leg")!=std::string::npos&&groups.find("head")!=std::string::npos&&groups.find("right_upper_arm")!=std::string::npos,"both Hell Knight cuts accept a broad body-group template");}
    }
    requireTarget(types.size()==25&&gk==188&&cs==36&&partTypes.size()==8&&hkCuts==2&&knightHead&&zombieBack,
        "audited inventory separates 25 declaration groups, 188 ordinary/boss kill states, 36 cut states and eight families with group-dependent GK");
    requireTarget(partTypes.count("imp")&&partTypes.count("carcass")&&partTypes.count("gargoyle")&&partTypes.count("mancubus_fire")&&partTypes.count("prowler")&&partTypes.count("soldier_blaster")&&partTypes.count("zombie_tier1")&&partTypes.count("zombie_tier3"),"part-dependent GK families are enumerated explicitly from authored rules");
}
static void checkZombieAutomaticSelection() {
    SyncTargetFrame zombie;zombie.level={0x10000,0x20000,"fixture/zombie_tier3"};zombie.entity={0x30000,0x10000,0x40000,1,1,"idAI2"};zombie.regionState="ready";zombie.ranges.known=true;
    std::ifstream input("Native/CollisionViewer/tests/fixtures/sync-zombie-tier3-11.tsv");requireTarget(bool(input),"actual zombie tier 3 frame fixture is present");
    std::string row;while(std::getline(input,row)){if(row.empty()||row[0]=='#')continue;std::istringstream fields(row);char kind=0;fields>>kind;
        auto vec=[&](Vec3& p){fields>>p.x>>p.y>>p.z;};
        if(kind=='P')vec(zombie.ranges.playerOrigin);else if(kind=='E')vec(zombie.ranges.eyeOffset);else if(kind=='A')vec(zombie.ranges.aimOrigin);else if(kind=='V')vec(zombie.ranges.viewTargetOffset);
        else if(kind=='M')fields>>zombie.ranges.modelViewOffset>>zombie.incomingYaw>>zombie.ranges.gloryKillTarget>>zombie.ranges.chainsawTarget>>zombie.ranges.lunge>>zombie.ranges.modifier>>zombie.ranges.ignoreQuickLunge;
        else if(kind=='T'){vec(zombie.facing.origin);for(auto& f:zombie.facing.axis)fields>>f;}
        else if(kind=='B'){for(auto& f:zombie.shotBounds)fields>>f;fields>>zombie.playerFootprint.minimum.x>>zombie.playerFootprint.minimum.y>>zombie.playerFootprint.maximum.x>>zombie.playerFootprint.maximum.y>>zombie.playerFootprint.known;}
        else if(kind=='Q'){fields>>zombie.aim.known>>zombie.aim.quadrant>>zombie.jointFocus.known>>zombie.jointFocus.group;if(zombie.jointFocus.group=="-")zombie.jointFocus.group.clear();}
        else if(kind=='R'){SyncFacingRegion r;size_t groups=0;fields>>r.name;vec(r.incoming);vec(r.start);fields>>r.halfYaw>>r.minXY>>r.maxXY>>r.minZ>>r.maxZ>>r.checkLunge>>r.flatLunge>>r.aim.quadrants>>r.aim.known>>groups;
            requireTarget(groups<64,"bounded native focus group fixture");for(size_t i=0;i<groups;++i){std::string group;fields>>group;r.aim.damageGroups.push_back(group);}zombie.regions.push_back(std::move(r));}
        else requireTarget(false,"known native zombie fixture record");requireTarget(!fields.fail(),"native zombie fixture field parsing");}
    requireTarget(zombie.regions.size()==13&&zombie.ranges.playerOrigin.z==zombie.facing.origin.z,"fixture reproduces thirteen zombie GK variants at grounded foot height");
    auto oldDefault=zombie;oldDefault.selectedAnimation=zombie.regions.front().name;
    requireTarget(oldDefault.selectedAnimation=="kill_above_back"&&syncBuildReferenceGuide(oldDefault).pieces.empty(),"alphabetical first available reproduces the reported invisible zombie GK zone");
    SyncAutomaticSelection automatic;automatic.update(zombie);
    requireTarget(zombie.selectedAnimation.empty()&&zombie.automaticResolved&&zombie.automaticAnimation=="kill_front_chest","a recorded position outside exclusive areas retains a useful ground fallback instead of selecting an aerial animation");
    const auto guide=syncBuildReferenceGuide(zombie);const auto mesh=syncBuildSectorMesh(guide,zombie.shotBounds);
    requireTarget(guide.known&&!guide.pieces.empty()&&mesh.known&&!mesh.fill.empty()&&!mesh.outline.empty(),"actual zombie tier 3 parameters now produce filled GK geometry and an outline");
    const auto retained=zombie.automaticAnimation;auto moved=zombie;moved.ranges.playerOrigin.x+=3;automatic.update(moved);
    requireTarget(moved.automaticAnimation==retained,"automatic selection remains stable while an existing reference area stays available");
    auto lowerAim=zombie;lowerAim.aim.quadrant=12;automatic.update(lowerAim);
    requireTarget(!lowerAim.automaticAnimation.empty()&&lowerAim.automaticAnimation!=retained&&!syncBuildReferenceGuide(lowerAim).pieces.empty(),"changing native aim eligibility replaces an unusable automatic selection with a valid area");
    auto manual=zombie;manual.selectedAnimation="kill_above_back";automatic.update(manual);
    requireTarget(!manual.automaticResolved&&syncSelectedAnimation(manual)==manual.selectedAnimation&&syncBuildReferenceGuide(manual).pieces.empty(),"explicit aerial selection is preserved even when unavailable on the ground");
    auto missing=zombie;missing.selectedAnimation="not_on_this_enemy";automatic.update(missing);
    requireTarget(syncSelectedAnimation(missing)=="not_on_this_enemy"&&!syncBuildReferenceGuide(missing).known,"manual animation absent from the new enemy is never silently replaced");
    auto unknown=zombie;unknown.ranges.known=false;automatic.update(unknown);
    requireTarget(unknown.automaticResolved&&unknown.automaticAnimation.empty()&&!syncBuildReferenceGuide(unknown).known,"automatic mode does not invent a fallback when no verified area can be built");
    auto next=zombie;next.entity.id++;next.entity.generation++;next.entity.address+=0x100;next.regions={zombie.regions[4]};next.regions[0].name="next_enemy_ground";automatic.update(next);
    requireTarget(next.automaticAnimation=="next_enemy_ground","automatic state is reacquired for a different target identity");
    auto saw=zombie;saw.action=1;saw.hasChainsaw=true;saw.chainsawAnimations={"only_saw"};saw.chainsawRules={{"only_saw",{},true}};automatic.update(saw);
    requireTarget(saw.automaticAnimation=="only_saw","automatic Chainsaw mode resolves its own native variants independently of GK");
    automatic.update(zombie);requireTarget(!zombie.automaticAnimation.empty()&&zombie.automaticAnimation!="only_saw","returning to GK cannot reuse a Chainsaw choice");
}
static void checkRecordedAimParts() {
    GameCamera camera{};float bounds[6]{};SyncAimFrame aim;std::vector<SyncHitSphere> spheres;std::vector<std::pair<std::string,SyncAimRequirement>> rules;
    std::ifstream fixture("Native/CollisionViewer/tests/fixtures/sync-aim-parts-zombie-11.tsv");requireTarget(bool(fixture),"actual retail 1.1 body-pose and native group fixture is present");
    std::string line;while(std::getline(fixture,line)){if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);char kind;fields>>kind;
        if(kind=='C'){for(auto& f:camera.matrix)fields>>f;for(auto& f:camera.position)fields>>f;}
        else if(kind=='B')for(auto& f:bounds)fields>>f;
        else if(kind=='Q'){fields>>aim.known;for(auto& f:aim.bounds)fields>>f;}
        else if(kind=='S'){SyncHitSphere s;fields>>s.center.x>>s.center.y>>s.center.z>>s.radius>>s.jointIndex>>s.focusKnown>>s.focusGroup>>s.group;if(s.focusGroup=="-")s.focusGroup.clear();spheres.push_back(std::move(s));}
        else if(kind=='R'){std::string name;SyncAimRequirement r;size_t n=0;fields>>name>>r.quadrants>>r.known>>n;requireTarget(n<32,"bounded native fixture group count");while(n--){std::string g;fields>>g;r.damageGroups.push_back(g);}rules.emplace_back(name,std::move(r));}
        else requireTarget(false,"known body-pose fixture record");requireTarget(!fields.fail(),"body-pose fixture parses");}
    requireTarget(spheres.size()==35&&rules.size()==4,"actual zombie frame has 35 current shot spheres and four group-dependent GK requirements");
    std::ofstream meshFile("build/sync-aim-parts-v13-mesh.tsv");meshFile.precision(9);meshFile<<"C";for(float f:camera.matrix)meshFile<<'\t'<<f;meshFile<<'\n';
    const auto started=std::chrono::steady_clock::now();
    for(const auto& item:rules){const auto mesh=syncBuildAimPartMesh(camera,bounds,aim,&item.second,spheres);
        requireTarget(mesh.known&&!mesh.fill.empty()&&!mesh.outline.empty(),"real zombie head, chest and both arms each produce visible aim patches from native data");
        std::vector<Vertex> wire;appendSyncAimBox(camera,bounds,aim,&item.second,wire,true);requireTarget(wire.size()==24,"anatomical overlays retain one square target box and omit coarse quadrant cross/fill");
        for(const auto& v:wire)meshFile<<"W\t"<<item.first<<'\t'<<v.position.x<<'\t'<<v.position.y<<'\t'<<v.position.z<<'\n';
        for(auto p:mesh.fill)meshFile<<"F\t"<<item.first<<'\t'<<p.x<<'\t'<<p.y<<'\t'<<p.z<<'\n';
        for(auto p:mesh.outline)meshFile<<"L\t"<<item.first<<'\t'<<p.x<<'\t'<<p.y<<'\t'<<p.z<<'\n';
        for(size_t i=0;i<mesh.fill.size();i+=3)for(unsigned a=0;a<=4;++a)for(unsigned b=0;b<=4-a;++b){
            const auto p=motionAdd(motionScale(mesh.fill[i],float(a)/4),motionAdd(motionScale(mesh.fill[i+1],float(b)/4),motionScale(mesh.fill[i+2],float(4-a-b)/4)));
            const Vec3 eye{camera.position[0],camera.position[1],camera.position[2]};auto ray=subtract(p,eye);ray=motionScale(ray,1/sqrtf(dot(ray,ray)));float distance=FLT_MAX;const SyncHitSphere* first=nullptr;
            for(const auto& sphere:spheres)if(syncSphereHit(eye,ray,sphere,distance))first=&sphere;
            requireTarget(first&&first->focusKnown&&std::find(item.second.damageGroups.begin(),item.second.damageGroups.end(),first->focusGroup)!=item.second.damageGroups.end(),"every sampled ray across actual native-pose mask triangles reaches an allowed part first");}
        if(item.second.quadrants==3){const auto* b=aim.bounds;double centerX,centerY,w;requireTarget(syncAimProject(camera,{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},centerX,centerY,w),"native fixture centre projects");
            for(auto p:mesh.fill){double x,y,z;requireTarget(syncAimProject(camera,p,x,y,z)&&y>centerY+1e-6,"top-only body aim masks exclude the shared native centre-line band");}}
    }
    const auto allFour=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
    auto shifted=camera;shifted.position[0]+=100;for(unsigned row:{0u,4u,8u,12u})shifted.matrix[row+3]-=100*shifted.matrix[row];auto moved=spheres;for(auto& s:moved)s.center.x+=100;
    auto shiftedAim=aim;shiftedAim.bounds[0]+=100;shiftedAim.bounds[3]+=100;float movedBounds[6];memcpy(movedBounds,bounds,24);movedBounds[0]+=100;movedBounds[3]+=100;
    requireTarget(!syncBuildAimPartMesh(shifted,movedBounds,shiftedAim,&rules[0].second,moved).fill.empty(),"real pose guidance survives world translation without losing its group mapping");
    auto manual=SyncTargetFrame{};manual.selectedAnimation=rules[0].first;manual.regions.push_back({});manual.regions[0].name=rules[0].first;manual.regions[0].aim=rules[0].second;
    manual.aimPartMesh=std::make_shared<const SyncAimPartMesh>(syncBuildAimPartMesh(camera,bounds,aim,&rules[0].second,spheres));
    const auto hud=syncPositionGuideLines(manual);requireTarget(std::find(hud.begin(),hud.end(),"Aim at a highlighted body area")!=hud.end(),"HUD explains the body aim patch even while standing geometry is unavailable");
    printf("Actual zombie four aim masks + dense ray checks + QA vertex export %.3f ms (offline CPU, not FPS).\n",allFour);
}
#include "sync_patch_checks.h"
int main(int argc,char** argv) {
    checkSyncPatchLayouts();
    setvbuf(stdout,nullptr,_IONBF,0);
    checkCarcassNativeTrace();
    checkCarcassBodyThresholds();
    checkCarcassWinnerColours();
    if(argc==2&&!strcmp(argv[1],"--carcass-only"))return 0;
    checkOrdinaryChainsawTrace();
    checkNearestBranchesAndGuides();
    checkCompactFinisherPresentation();
    checkAllEnemyBodyZones(false,true);
    requireTarget(nearestFamilyViews>8000&&nearestContexts>100000&&nearestTriangles>100000&&nearestRegionViews>500,"all-family nearest replay covers both actions, every supported region and rendered winning colours");
    printf("PASS: generic nearest selection: 25 families, %zu views, %zu group/quadrant checks, %zu sector views, %zu triangle colours.\n",nearestFamilyViews,nearestContexts,nearestRegionViews,nearestTriangles);
    if(argc==2&&!strcmp(argv[1],"--nearest-only"))return 0;
    checkDistanceSelection();
    checkDistanceBodyFilter();
    checkRecordedDistanceMaps();
    if(argc==2&&!strcmp(argv[1],"--distance-only")){checkAllEnemyBodyZones(true);return 0;}
    checkCapturedCarcassLowerTorso();
    checkAllEnemyBodyZones();
    checkBodyPaletteByOccupiedSector();
    checkBodyColourScopes();
    checkCapturedBodyColours();
    checkAutomaticBodyAvailability();
    checkCapturedBodyQuadrants();
    checkRecordedAutomaticBodyZones();
    checkLimbUnassignedAndNearPlane();
    checkAllCapturedLimbViews();
    checkCapturedLowerAimViews();
    checkCapturedZombieAndSoldier();
    checkRetainedCaptureLifecycle();
    checkAutomaticEnteredArea();
    checkRetainedReferenceGeometry();
    checkAuthoredBeforeGkState();
    checkLimbStandingReference();
    checkNearFarAndAimScopedExclusions();
    checkEveryEnemyDisplay();
    checkDisplaySwitchesAndUnknowns();
    checkRecordedHellKnightDisplay();
    checkRecordedChainsawClearanceBand();
    checkEnemyRuleInventory();
    checkNativeChoiceAudit();
    checkAimPartHighlights();
    checkRecordedAimParts();
    checkZombieAutomaticSelection();
    const bool wasEnabled=enabled;enabled=false;showSyncTarget=true;
    requireTarget(nativeOverlayNeeded(),"enemy zones request rendering independently of Collision Viewer");
    showSyncTarget=false;requireTarget(!nativeOverlayNeeded(),"disabled displays do not keep the overlay active");enabled=wasEnabled;
    const WallMemory memory;float distance=50;SyncHitSphere sphere{{0,4,0},1,"head"};
    requireTarget(syncSphereHit({0,0,0},{0,1,0},sphere,distance)&&fabsf(distance-3)<1e-5f,"ray hits actual sphere surface");
    distance=50;requireTarget(!syncSphereHit({.9f,0,.9f},{0,1,0},sphere,distance),"inside bounding box is not a hitbox hit");
    distance=50;requireTarget(syncSphereHit({0,4,0},{0,1,0},sphere,distance)&&distance==1,"camera inside sphere selects forward exit");
    sphere.radius=-1;distance=50;requireTarget(!syncSphereHit({0,0,0},{0,1,0},sphere,distance),"negative radius rejected");
    sphere.radius=std::numeric_limits<float>::quiet_NaN();requireTarget(!syncSphereHit({0,0,0},{0,1,0},sphere,distance),"NaN sphere rejected");
    const int gk[]{2,0},chainsaw[]{1,16},any[]{17,0},strict[]{16,2};
    requireTarget(syncInputMatches(gk,2)&&!syncInputMatches(gk,16),"GK input is independent of Chainsaw");
    requireTarget(syncInputMatches(chainsaw,16)&&!syncInputMatches(chainsaw,2),"Chainsaw input uses real input filter");
    requireTarget(syncInputMatches(any,2)&&!syncInputMatches(any,0)&&!syncInputMatches(strict,2),"wildcard and strict Chainsaw branch match audited semantics");
    HitTargetFixture hit;std::vector<SyncHitSphere> spheres;
    requireTarget(readSyncHitSpheres(memory,0x10000000,hit.ref,spheres)&&spheres.size()==1&&spheres[0].group=="head"&&spheres[0].center.y==4,"real compound material maps to hit group");
    float turn[9]{0,1,0,-1,0,0,0,0,1};memcpy(hit.physics.data()+0x8c,turn,36);targetPut(hit.child,0x30,Vec3{1,0,0});
    requireTarget(readSyncHitSpheres(memory,0x10000000,hit.ref,spheres)&&spheres[0].center.x==0&&spheres[0].center.y==5,"animated local sphere follows native body basis");
    targetPut(hit.child,0x44,2.f);requireTarget(!readSyncHitSpheres(memory,0x10000000,hit.ref,spheres),"nonuniform shape is not silently expanded");targetPut(hit.child,0x44,1.f);
    targetPut(hit.child,0x44,std::numeric_limits<float>::quiet_NaN());requireTarget(!readSyncHitSpheres(memory,0x10000000,hit.ref,spheres),"NaN in a secondary scale axis cannot pass uniformity checks");targetPut(hit.child,0x44,1.f);
    hit.leaf[0x22]=3;requireTarget(!readSyncHitSpheres(memory,0x10000000,hit.ref,spheres),"unsupported shape kind never becomes a sphere");hit.leaf[0x22]=2;
    targetPut(hit.materials,96,uint32_t(0x40000080));requireTarget(!readSyncHitSpheres(memory,0x10000000,hit.ref,spheres),"NOCOLLISION hit group excluded");targetPut(hit.materials,96,uint32_t(0x40080));
    hit.actor[0x6488]=1;requireTarget(!readSyncHitSpheres(memory,0x10000000,hit.ref,spheres),"dead retained actor cannot produce geometry");hit.actor[0x6488]=0;
    targetPut(hit.physics,0x70,uintptr_t(0x30000));requireTarget(!readSyncHitSpheres(memory,0x10000000,hit.ref,spheres),"foreign map physics rejected");
    TargetCaptureFixture scene;checkOrdinaryPriorityReader(scene.image,scene.base);checkSyncPlayerContext(scene);SyncTargetReader reader{0};GameCamera camera{};camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    {AuthoredPreviewFixture assigned("tyrant");auto lookup=assigned.lookup();const auto at=targetAddress(scene.enemies[0].actor)+0x4db0;
        memcpy(reinterpret_cast<void*>(at+0x608),reinterpret_cast<void*>(assigned.source+0x608),24);
        memcpy(reinterpret_cast<void*>(at+0x1b0),reinterpret_cast<void*>(assigned.source+0x1b0),24);
        const auto beforeState=reader.capture(memory,scene.base,kProfile11,scene.level,camera,50,&lookup);
        requireTarget(beforeState.entity&&beforeState.gloryKills.empty()&&beforeState.nativeChoices.empty()&&syncAnimationChoices(beforeState).size()==assigned.gkCount,
            "full capture publishes Tyrant's assigned animation choices with an empty native GK candidate pool");
        requireTarget(!beforeState.selectionKnown,"pre-state authoring never confirms a current executable animation");
        memset(reinterpret_cast<void*>(at+0x608),0,24);memset(reinterpret_cast<void*>(at+0x1b0),0,24);}
    auto capture=[&]{return reader.capture(memory,scene.base,kProfile11,scene.level,camera);};
    auto value=capture();requireTarget(value.phase=="ready"&&value.entity.address==targetAddress(scene.enemies[0].actor)&&value.distance==3,"only nearest aimed living actor selected");
    requireTarget(!reader.capture(memory,scene.base,kProfile11,scene.level,camera,2).entity,"configured view distance also limits enemy zones");
    targetPut(scene.enemies[0].targeting,0xc4,uint32_t(2));value=capture();requireTarget(!value.entity,"friendly AI is not an enemy and blocks selecting one through its hit geometry");targetPut(scene.enemies[0].targeting,0xc4,uint32_t(1));
    scene.enemies[0].actor[0x6488]=1;value=capture();requireTarget(value.entity.address==targetAddress(scene.enemies[1].actor),"dead nearest actor does not survive in cached selection");scene.enemies[0].actor[0x6488]=0;
    camera.position[0]=.9f;camera.position[2]=.9f;value=capture();requireTarget(!value.entity,"ray in broad bounds but outside spheres has no aimed target");camera.position[0]=camera.position[2]=0;
    scene.info[0xf8]=0x40;scene.info[0x1f8]=0x40;value=capture();requireTarget(!value.entity,"engine line-of-sight failure excludes actors");scene.info[0xf8]=0x41;scene.info[0x1f8]=0x41;
    scene.enemies[0].leaf[0x22]=3;value=capture();requireTarget(value.phase!="ready","unknown foreground geometry cannot expose a farther target");scene.enemies[0].leaf[0x22]=2;
    std::vector<unsigned char> candidate(0x160),decl(0x398),pointers(8);
    targetPut(pointers,0,targetAddress(candidate));targetPut(scene.player,0x70e8+0x178,SyncPreviewList{targetAddress(pointers),1,1,0});
    targetPut(candidate,0x140,targetAddress(scene.enemies[0].actor));targetPut(candidate,0x138,uint32_t(1));targetPut(candidate,0x40,targetAddress(decl));candidate[0x48]=1;
    targetPut(candidate,0xe8,reinterpret_cast<uintptr_t>("kill_front_lower"));targetPut(decl,0x98,int32_t(2));
    value=capture();requireTarget(value.candidatesReady&&value.gloryKills==std::vector<std::string>{"kill_front_lower"}&&value.chainsaw.empty(),"native animation candidate belongs to selected actor and correct action");
    targetPut(decl,0x98,int32_t(1));targetPut(decl,0x9c,int32_t(16));targetPut(decl,0x310,uint32_t(4));
    value=capture();requireTarget(value.hasChainsaw&&value.chainsawRules.size()==1&&value.chainsawRules[0].quick&&value.chainsawRules[0].aim.known&&value.chainsawRules[0].aim.quadrants==4,"native Chainsaw authoring retains its own QUICK flag and focal quadrant requirements");
    targetPut(decl,0x98,int32_t(2));targetPut(decl,0x9c,int32_t(0));targetPut(decl,0x310,uint32_t(0));
    candidate[0x110]=1;value=capture();requireTarget(value.gloryKills.empty()&&value.pendingGloryKill&&!value.pendingChainsaw,"pending GK trace is not shown as ready and is tracked independently from Chainsaw");candidate[0x110]=0;
    decl[0x314]=1;value=capture();requireTarget(value.gloryKills.empty(),"Berserk requirement is not mislabeled as ordinary GK");decl[0x314]=0;
    targetPut(candidate,0x140,targetAddress(scene.enemies[1].actor));value=capture();requireTarget(value.gloryKills.empty(),"another enemy's animation cannot leak into HUD");
    targetPut(scene.entries,0,uint32_t(99));value=capture();requireTarget(value.entity.address!=targetAddress(scene.enemies[0].actor),"old GameView token cannot select a reused actor slot");
    targetPut(scene.info,0xd0,uint32_t(99));value=capture();requireTarget(value.entity.id==99,"entity slot reuse invalidates old generation and reacquires identity");
    targetPut(scene.view,0xc8,uintptr_t(0));value=capture();requireTarget(!value.entity,"foreign view owner clears target");
    // Authoring data, not names, determines the facing direction.
    float instigatorPose[12]{1,0,0,0,0,1,0,0,0,0,1,0};
    float targetPose[12]{-1,0,0,3.8f,0,-1,0,0,0,0,1,2.4f};Vec3 incoming{};
    requireTarget(syncFacingIncoming(instigatorPose,targetPose,incoming)&&incoming.x==1&&incoming.y==0,"front direction follows native attach matrices");
    float leftPose[12]{0,-1,0,3.8f,1,0,0,0,0,0,1,2.4f};
    requireTarget(syncFacingIncoming(instigatorPose,leftPose,incoming)&&incoming.x==0&&incoming.y==1,"side direction follows authored rotation");
    std::vector<unsigned char> nativeDecl(0x398),jointCache(0x3020),node(0xb0);
    nativeDecl[0x1d8]=1;targetPut(nativeDecl,0x264,90.f);targetPut(nativeDecl,0x268,60.f);
    const float ranges[4]{0,6,0,9};memcpy(nativeDecl.data()+0x254,ranges,16);jointCache[0x301c]=1;
    memcpy(jointCache.data()+48,instigatorPose,48);memcpy(jointCache.data()+96,targetPose,48);
    SyncFacingRegion region;
    auto readRegion=[&]{return readSyncFacingRegion(memory,targetAddress(nativeDecl),targetAddress(node),targetAddress(jointCache),1,2,"unrelated_native_name",region);};
    requireTarget(readRegion()&&region.name=="unrelated_native_name"&&region.incoming.x==1&&fabsf(region.halfYaw-syncFacingPi/3)<1e-6f,"region comes from cached pose and declaration yaw, not front/back words");
    nativeDecl[0x108]=1;requireTarget(!readRegion(),"instigator-centric alignment is not turned into a fixed enemy sector");nativeDecl[0x108]=0;
    targetPut(nativeDecl,0x264,30.f);requireTarget(!readRegion(),"unrepresented pitch restrictions are not silently drawn as full height");targetPut(nativeDecl,0x264,90.f);
    jointCache[0x301c]=0;requireTarget(!readRegion(),"unready native joint cache cannot draw a guessed sector");jointCache[0x301c]=1;
    targetPut(jointCache,0x3018,int32_t(1));requireTarget(!readRegion(),"only authored initial frame is used");targetPut(jointCache,0x3018,int32_t(0));
    targetPut(nativeDecl,0x268,std::numeric_limits<float>::quiet_NaN());requireTarget(!readRegion(),"non-finite native yaw rejected");targetPut(nativeDecl,0x268,60.f);
    requireTarget(readRegion(),"valid authoring recovered");
    {std::ifstream fixture("Native/CollisionViewer/tests/fixtures/sync-demon-authoring-11.tsv");requireTarget(bool(fixture),"real retail 1.1 demon metadata fixture is present");
        std::string row;size_t cases=0;std::unordered_set<std::string> types;
        while(std::getline(fixture,row)){if(row.empty()||row[0]=='#')continue;std::istringstream fields(row);std::string name;float pitch,yaw,values[4];uint32_t mask;
            requireTarget(bool(fields>>name>>pitch>>yaw>>values[0]>>values[1]>>values[2]>>values[3]>>mask),"demon declaration fixture parses");
            targetPut(nativeDecl,0x264,pitch);targetPut(nativeDecl,0x268,yaw);targetPut(nativeDecl,0x310,mask);memcpy(nativeDecl.data()+0x254,values,16);
            if(!readRegion())fprintf(stderr,"Unsupported real declaration: %s\n",name.c_str());requireTarget(readRegion(),"real ordinary demon validation metadata is supported with flat authored attach matrices");
            requireTarget(region.minZ==values[2]&&region.maxXY==values[1]&&fabsf(region.halfYaw-fabsf(yaw)*syncFacingPi/180)<1e-6f,"per-demon height, distance and angle survive parsing without Cacodemon defaults");
            ++cases;const auto start=name.find('/',name.find('/')+1)+1;types.insert(name.substr(start,name.find('/',start)-start));}
        requireTarget(cases==192&&types.size()==24,"192 real ordinary target-centric declarations cover 24 demon types; authored directions are not inferred from names");}
    targetPut(nativeDecl,0x264,90.f);targetPut(nativeDecl,0x268,60.f);targetPut(nativeDecl,0x310,uint32_t(0));memcpy(nativeDecl.data()+0x254,ranges,16);requireTarget(readRegion(),"restore original authoring fixture");
    targetPut(nativeDecl,0x310,uint32_t(2));auto aimRule=readSyncAimRequirement(memory,targetAddress(nativeDecl));
    requireTarget(aimRule.known&&aimRule.quadrants==2&&syncAimMatches(aimRule,2)&&!syncAimMatches(aimRule,1),"native focalScreenQuadrants are read independently from animation names and facing");
    std::vector<unsigned char> groupNames(8);targetPut(groupNames,0,reinterpret_cast<uintptr_t>("head"));
    targetPut(nativeDecl,0x2a0,SyncPreviewList{targetAddress(groupNames),1,1,0});aimRule=readSyncAimRequirement(memory,targetAddress(nativeDecl));
    requireTarget(aimRule.known&&aimRule.damageGroups==std::vector<std::string>{"head"},"native joint-focus requirements are retained instead of replaced by guessed hit-box halves");
    targetPut(nativeDecl,0x2a0,SyncPreviewList{});targetPut(nativeDecl,0x310,uint32_t(16));requireTarget(!readSyncAimRequirement(memory,targetAddress(nativeDecl)).known,"unknown quadrant bits fail closed");targetPut(nativeDecl,0x310,uint32_t(0));
    GameCamera aimCamera{};aimCamera.matrix[0]=aimCamera.matrix[6]=aimCamera.matrix[13]=1;
    for(auto example:std::array<std::array<float,3>,9>{{{{1,-1,1}},{{-1,-1,2}},{{1,1,4}},{{-1,1,8}},{{0,-1,3}},{{0,1,12}},{{1,0,5}},{{-1,0,10}},{{0,0,15}}}}){
        const float bounds[]{example[0]-.5f,4.5f,example[1]-.5f,example[0]+.5f,5.5f,example[1]+.5f};
        requireTarget(syncAimQuadrant(aimCamera,bounds)==uint32_t(example[2]),"aim quadrants follow the native projected centre and preserve all shared-axis bits");}
    float aimBounds[]{-1,3,-1,1,5,1};SyncAimFrame aimFrame;aimFrame.known=true;memcpy(aimFrame.bounds,aimBounds,24);aimFrame.quadrant=15;
    aimFrame.bounds[0]+=.25f;aimFrame.bounds[3]+=.25f;aimFrame.bounds[2]-=.25f;aimFrame.bounds[5]-=.25f;
    SyncAimRequirement topLeft;topLeft.quadrants=1;std::vector<Vertex> aimBoxLines,aimBoxFill;
    appendSyncAimBox(aimCamera,aimBounds,aimFrame,&topLeft,aimBoxLines,true);appendSyncAimBox(aimCamera,aimBounds,aimFrame,&topLeft,aimBoxFill,false);
    requireTarget(aimBoxLines.size()==28&&aimBoxFill.size()==6,"one target wire box plus native centre guides and one requested aim quadrant are drawn");
    for(size_t i=0;i<24;++i){const auto p=aimBoxLines[i].position;requireTarget(p.x>=-1&&p.x<=1&&p.y>=3&&p.y<=5&&p.z>=-1&&p.z<=1,"wire box follows the supplied shot bounds");}
    for(const auto& vertex:aimBoxFill){double x,y,w;requireTarget(syncAimProject(aimCamera,vertex.position,x,y,w)&&x<=.0625+1e-6&&y>=-.0625-1e-6,"highlighted tile follows native physics centre, not the different shot-box centre");}
    auto anyAim=topLeft;anyAim.quadrants=0x7fffffff;aimBoxFill.clear();appendSyncAimBox(aimCamera,aimBounds,aimFrame,&anyAim,aimBoxFill,false);
    requireTarget(aimBoxFill.empty(),"unrestricted animations do not invent body subdivisions");
    std::vector<unsigned char> aimPhysics(0x200);
    const auto aimPhysicsPointer=targetAddress(aimPhysics),actorVtable=scene.base+0x1000,physicsGetter=scene.base+0x2500,boundsGetter=scene.base+0x2520;
    memcpy(scene.image+0x1e28,&physicsGetter,8);memcpy(scene.image+0x1038,&boundsGetter,8);
    const unsigned char physicsCode[]{0x48,0x8b,0x81,0xd0,1,0,0,0xc3},boundsCode[]{0x48,0x8d,0x81,0xe0,0,0,0,0xc3};
    memcpy(scene.image+0x2500,physicsCode,8);memcpy(scene.image+0x2520,boundsCode,8);targetPut(scene.enemies[0].actor,0x1d0,aimPhysicsPointer);
    targetPut(aimPhysics,0,actorVtable);targetPut(aimPhysics,0x70,scene.level.map);memcpy(aimPhysics.data()+0xe0,aimBounds,24);
    auto readAim=readSyncAimFrame(memory,scene.base,scene.enemies[0].ref,aimCamera);
    requireTarget(readAim.known&&readAim.quadrant==15,"native aim reference follows guarded target GetPhysics and absolute-bounds getters");
    scene.image[0x2520]=0x90;requireTarget(!readSyncAimFrame(memory,scene.base,scene.enemies[0].ref,aimCamera).known,"unknown bounds getter cannot produce guessed quadrant data");memcpy(scene.image+0x2520,boundsCode,8);
    requireTarget(fabsf(region.start.x-3.8f)<1e-5f&&fabsf(region.start.z+2.4f)<1e-5f,"native initial player position is expressed in target coordinates");
    SyncTargetFrame geometry;geometry.playerFootprint={true,{-.395f,-.395f,0},{.395f,.395f,0}};geometry.regionState="ready";geometry.regions={region};geometry.regions[0].checkLunge=true;
    geometry.facing={{1,0,0,0,1,0,0,0,1},{0,0,0}};geometry.incomingYaw=0;
    const float geometryBox[6]{-.5f,-.5f,-.5f,.5f,.5f,.5f};memcpy(geometry.shotBounds,geometryBox,24);
    geometry.ranges.known=true;geometry.ranges.gloryKillTarget=9;geometry.ranges.chainsawTarget=10;geometry.ranges.lunge=9;
    geometry.ranges.ignoreQuickLunge=true;geometry.ranges.eyeOffset={0,0,1.6f};geometry.hasChainsaw=true;geometry.chainsawAnimations={"cut_back","cut_front"};geometry.chainsawRules={{"cut_back",{},true},{"cut_front",{},true}};
    SyncRangeVolume volume;requireTarget(syncRangeBuild(&geometry.regions[0],geometry.facing,geometry.ranges,false,0,volume),"native cylinder, view radius, body lunge and facing planes form a bounded intersection");
    Vec3 boundary{};requireTarget(syncRangeRay(volume,{1,0,0},boundary)&&boundary.x>8&&boundary.x<9,"GK preview extends to the native distance instead of fitting the body");
    requireTarget(!syncRangeContains(volume,motionAdd(boundary,{.1f,0,0}))&&syncRangeContains(volume,motionAdd(boundary,{-.1f,0,0})),"range boundary rejects positions beyond the native cap");
    requireTarget(!syncRangeContains(volume,{-1,0,0}),"front-facing positions are not extended behind the enemy");
    auto aimed=geometry.ranges;aimed.aimOrigin={0,2,0};
    requireTarget(syncRangeBuild(&geometry.regions[0],geometry.facing,aimed,false,0,volume)&&syncRangeContains(volume,{2,2,-1.6f})&&!syncRangeContains(volume,{1,0,-1.6f}),"facing boundary follows the current aim point rather than a fixed body box");
    aimed.modelViewOffset=syncFacingPi/2;
    requireTarget(syncRangeBuild(&geometry.regions[0],geometry.facing,aimed,false,0,volume)&&syncRangeContains(volume,{0,1,-1.6f})&&!syncRangeContains(volume,{1,2,-1.6f}),"native model-to-view offset is included in the spatial facing condition");
    requireTarget(syncRangeBuild(&geometry.regions[0],geometry.facing,geometry.ranges,false,0,volume),"restore native unshifted intersection");
    geometry.ranges.playerOrigin={2,0,-1.6f};
    auto section=syncGuideSection(volume,geometry.ranges.playerOrigin.z);
    requireTarget(section.size()>=3,"guide has a horizontal foot-height section");
    for(auto point:section)requireTarget(point.z==geometry.ranges.playerOrigin.z&&syncRangeContains(volume,point,-.0001),"foot contour remains inside all native constraints");
    auto status=syncGuideStatus(volume,{12,0,-1.6f},false);
    requireTarget(!status.inside&&status.cameraRange&&status.startRange&&status.lunge&&!status.direction&&!status.height,"distance diagnostics identify the failed native constraints");
    status=syncGuideStatus(volume,{-2,0,-1.6f},false);requireTarget(status.direction&&!status.cameraRange,"wrong side is distinguished from out of range");
    status=syncGuideStatus(volume,{2,0,10},false);requireTarget(status.height,"height violation is distinguished from facing");
    Vec3 destination{};requireTarget(syncGuideDestination(volume,{-2,3,12},destination)&&syncRangeContains(volume,destination,-.0001),"movement marker satisfies the full intersected volume");
    requireTarget(syncGuideSection(volume,20).empty(),"height mismatch does not draw a false ground footprint");
    requireTarget(syncGuideStatus(volume,geometry.ranges.playerOrigin,false).inside,"player feet rather than camera position determine area membership");
    auto boundaryPlayer=geometry;boundaryPlayer.ranges.playerOrigin={1.72073f,2.45746f,-1.6f};boundaryPlayer.candidatesReady=true;boundaryPlayer.gloryKills={region.name};
    const auto boundaryGuide=syncBuildReferenceGuide(boundaryPlayer);
    requireTarget(syncRangeContains(volume,boundaryPlayer.ranges.playerOrigin)&&!syncExclusiveContains(boundaryGuide,boundaryPlayer.ranges.playerOrigin),"a centre inside the angular range is insufficient when the real player footprint crosses its boundary");
    requireTarget(syncGuideFeedback(boundaryPlayer,volume,region.name).tone==SyncSectorTone::Waiting,"player-footprint contact cannot turn a near-boundary centre green");
    auto noCollider=boundaryPlayer;noCollider.playerFootprint.known=false;requireTarget(!syncBuildReferenceGuide(noCollider).known,"unknown player bounds never substitute an arbitrary capsule radius");
    requireTarget(syncSectorColor(SyncSectorTone::Reference,false)[0]==.65f&&syncSectorColor(SyncSectorTone::Reference,false)[1]==.7f,"outside reference uses neutral grey rather than misleading cyan");
    float approachNear=0,approachFar=0;
    requireTarget(syncGuideApproach(volume,geometry.ranges.playerOrigin,approachNear,approachFar)&&fabsf(approachFar-sqrtf(81-1.6f*1.6f))<.001f&&approachFar<9,"selected approach range includes body lunge, not only the larger camera cap");
    requireTarget(!syncGuideApproach(volume,{-2,0,-1.6f},approachNear,approachFar)&&!syncGuideApproach(volume,{2,0,20},approachNear,approachFar),"wrong side and impossible height have no fabricated usable distance");
    auto duplicate=geometry.regions[0];duplicate.name="other_animation";duplicate.incoming={-1,0,0};geometry.regions.push_back(duplicate);
    std::string selected;geometry.selectedAnimation=geometry.regions[0].name;SyncRangeVolume selectedVolume;
    requireTarget(syncSelectedVolume(geometry,selectedVolume,selected)&&selected==geometry.regions[0].name,"explicit animation resolves only its own native volume");
    std::vector<Vertex> oneArea;appendSyncTargetGeometry(camera,geometry,oneArea,true);
    geometry.regions.pop_back();std::vector<Vertex> withoutOthers;appendSyncTargetGeometry(camera,geometry,withoutOthers,true);
    requireTarget(!oneArea.empty()&&!withoutOthers.empty(),"only the selected area remains drawable after subtracting a non-overlapping variant");
    geometry.regions.push_back(duplicate);
    geometry.selectedAnimation="not_on_this_enemy";requireTarget(!syncSelectedVolume(geometry,selectedVolume,selected),"missing selection never silently substitutes another animation");
    geometry.selectedAnimation.clear();geometry.regions.pop_back();
    geometry.regionState="ready";geometry.candidatesReady=true;geometry.gloryKills={geometry.regions[0].name};
    requireTarget(syncSelectedVolume(geometry,selectedVolume,selected)&&syncGuideFeedback(geometry,selectedVolume,selected).selectedOnlyObserved,"green requires the selected animation to be the sole observed candidate without overlap");
    geometry.gloryKills={"another_native_candidate"};requireTarget(!syncGuideFeedback(geometry,selectedVolume,selected).selectedOnlyObserved,"inside geometry with another engine candidate cannot turn green");
    geometry.gloryKills={geometry.regions[0].name};geometry.pendingGloryKill=true;
    requireTarget(!syncGuideFeedback(geometry,selectedVolume,selected).selectedOnlyObserved,"pending native trace prevents green even when one name remains");geometry.pendingGloryKill=false;
    geometry.candidatesReady=false;requireTarget(!syncGuideFeedback(geometry,selectedVolume,selected).selectedOnlyObserved,"incoherent native candidates cannot turn green");geometry.candidatesReady=true;
    geometry.regionState="partial facing preview";requireTarget(!syncGuideFeedback(geometry,selectedVolume,selected).selectedOnlyObserved,"unknown additional geometry prevents green");geometry.regionState="ready";
    const auto anchoredGuide=syncBuildReferenceGuide(geometry);const auto anchoredMesh=syncBuildSectorMesh(anchoredGuide,geometry.shotBounds);
    requireTarget(anchoredGuide.known&&!anchoredMesh.fill.empty(),"enemy-anchored reference has a nonempty sector");
    for(unsigned aimCase=0;aimCase<12;++aimCase){auto changedAim=geometry;
        changedAim.ranges.aimOrigin={float(aimCase)*.15f-1,2-float(aimCase)*.23f,float(aimCase)*.1f};
        changedAim.ranges.modelViewOffset=(float(aimCase)-6)*.2f;changedAim.incomingYaw=float(aimCase)*.3f;
        changedAim.ranges.eyeOffset.x=.15f*float(aimCase);changedAim.ranges.eyeOffset.y=-.08f*float(aimCase);
        const auto guide=syncBuildReferenceGuide(changedAim);const auto mesh=syncBuildSectorMesh(guide,changedAim.shotBounds);
        requireTarget(mesh.known&&mesh.fill.size()==anchoredMesh.fill.size()&&mesh.outline.size()==anchoredMesh.outline.size()&&
            !memcmp(mesh.fill.data(),anchoredMesh.fill.data(),mesh.fill.size()*sizeof(Vec3))&&!memcmp(mesh.outline.data(),anchoredMesh.outline.data(),mesh.outline.size()*sizeof(Vec3)),
            "moving aim point, model/view yaw and lateral camera offset cannot drag or rotate the reference mesh");
        requireTarget(changedAim.ranges.modelViewOffset==(float(aimCase)-6)*.2f&&changedAim.ranges.aimOrigin.y==2-float(aimCase)*.23f,"reference construction preserves the actual sampled native aim data");}
    auto wrongFacing=geometry;wrongFacing.ranges.modelViewOffset=syncFacingPi/2;wrongFacing.incomingYaw=syncFacingPi/2;
    requireTarget(syncExclusiveContains(anchoredGuide,wrongFacing.ranges.playerOrigin),"fixed reference can contain the player while actual facing differs");
    const auto facingFeedback=syncGuideFeedback(wrongFacing,anchoredGuide.selected,selected);
    requireTarget(!facingFeedback.selectedOnlyObserved&&facingFeedback.detail.find("current aim / body facing")!=std::string::npos,
        "standing in the fixed sector never confirms an animation when the true native facing volume rejects it");
    auto movedEnemy=geometry;const Vec3 translation{12,-7,3};movedEnemy.facing.origin=translation;
    movedEnemy.ranges.playerOrigin=motionAdd(geometry.ranges.playerOrigin,translation);movedEnemy.ranges.aimOrigin={999,888,0};
    const float translationComponents[3]{translation.x,translation.y,translation.z};
    for(unsigned i=0;i<3;++i){movedEnemy.shotBounds[i]+=translationComponents[i];movedEnemy.shotBounds[i+3]+=translationComponents[i];}
    const auto movedReference=syncBuildReferenceGuide(movedEnemy);
    for(float x=-8;x<=8;x+=.25f)for(float y=-8;y<=8;y+=.25f){const Vec3 p{x,y,geometry.ranges.playerOrigin.z};
        requireTarget(syncExclusiveContains(anchoredGuide,p)==syncExclusiveContains(movedReference,motionAdd(p,translation)),"reference area translates with the enemy instead of retaining a camera/world anchor");}
    auto turnedEnemy=geometry;const float ninety[9]{0,1,0,-1,0,0,0,0,1};memcpy(turnedEnemy.facing.axis,ninety,36);
    const auto turnedReference=syncBuildReferenceGuide(turnedEnemy);
    requireTarget(syncExclusiveContains(anchoredGuide,{3,0,-1.6f})&&syncExclusiveContains(turnedReference,{0,3,-1.6f})&&!syncExclusiveContains(turnedReference,{3,0,-1.6f}),"reference sector turns with the enemy native facing axis");
    auto actualOverlap=geometry;auto quarterCompetitor=geometry.regions[0];quarterCompetitor.name="native_quarter_turn";quarterCompetitor.incoming={0,1,0};
    actualOverlap.regions.push_back(quarterCompetitor);actualOverlap.ranges.modelViewOffset=syncFacingPi/4;actualOverlap.incomingYaw=syncFacingPi/4;const auto overlapReference=syncBuildReferenceGuide(actualOverlap);
    requireTarget(syncExclusiveContains(overlapReference,actualOverlap.ranges.playerOrigin),"centre-facing reference removes its own competing sectors");
    const auto overlapFeedback=syncGuideFeedback(actualOverlap,overlapReference.selected,selected);
    requireTarget(!overlapFeedback.selectedOnlyObserved&&overlapFeedback.otherGeometry==std::vector<std::string>{quarterCompetitor.name},"true current-aim competitor prevents green even if the fixed reference is exclusive and candidate publication still contains one name");
    auto shiftedCamera=geometry;shiftedCamera.ranges.playerOrigin={8.2f,0,-1.6f};shiftedCamera.ranges.eyeOffset.x=2;
    const auto cameraReference=syncBuildReferenceGuide(shiftedCamera);
    requireTarget(syncExclusiveContains(cameraReference,shiftedCamera.ranges.playerOrigin)&&!syncGuideFeedback(shiftedCamera,cameraReference.selected,selected).selectedOnlyObserved,"current camera range is enforced separately from the fixed reference sector");
    const auto cameraLines=syncPositionGuideLines(shiftedCamera);
    requireTarget(std::any_of(cameraLines.begin(),cameraLines.end(),[](const auto& line){return line.find("Distance: 10.20 m / 9.00 m")!=std::string::npos;}),"HUD range reports the actual camera metric rather than the anchored reference metric");
    // Recorded v4 failure: victim 16796944, Back upper selected, both native
    // candidates valid before actual Right upper. Authoring starts and pose
    // are copied from the 2026-11-06 read-only demonstration, not equal wedges.
    auto recorded=geometry;recorded.selectedAnimation="kill_back_upper";
    recorded.facing={{.9999997616f,-.0006902913f,0,.0006902913f,.9999997616f,0,0,0,1},{-1.5275612f,-346.13031f,33.43732f}};
    recorded.ranges.playerOrigin={-4.6996074f,-349.526947f,32.2000046f};recorded.ranges.eyeOffset={0,0,1.65732f};
    recorded.ranges.aimOrigin={-1.9728f,-347.467f,31.5446f};recorded.ranges.modelViewOffset=-.00364327f;recorded.incomingYaw=-2.4982f;
    auto back=recorded.regions[0];back.name="kill_back_upper";back.start={-2.99508667f,1.04836845f,-2.43659102f};back.incoming={-1,0,0};
    auto side=back;side.name="kill_right_upper";side.start={.53384221f,-3.09451929f,-2.39830056f};side.incoming={.09306753f,-.99565983f,0};
    recorded.regions={back,side};recorded.gloryKills={back.name,side.name};
    requireTarget(syncSelectedVolume(recorded,selectedVolume,selected)&&syncRangeContains(selectedVolume,recorded.ranges.playerOrigin),"recorded Back upper position still satisfies the unchanged native geometry");
    auto feedback=syncGuideFeedback(recorded,selectedVolume,selected);
    requireTarget(!feedback.selectedOnlyObserved&&feedback.otherGeometry==std::vector<std::string>{side.name}&&feedback.detail.find("Right upper")!=std::string::npos,"recorded false-green position is amber and identifies the competing side animation");
    std::vector<Vertex> amber;appendSyncPositionGuide(selectedVolume,recorded.ranges.playerOrigin,amber,false,feedback.selectedOnlyObserved);
    requireTarget(!amber.empty()&&amber[0].color[0]==1&&amber[0].color[1]==.65f,"overlap renders amber instead of green");
    auto left=back;left.name="kill_left_upper";left.start={.41682535f,3.8018918f,-2.4578545f};left.incoming={0,1,0};
    auto front=back;front.name="kill_front_lower";front.start={3.8482535f,0,-2.4192219f};front.incoming={1,0,0};
    recorded.regions={back,side,left,front};
    const auto exclusive=syncBuildExclusiveGuide(recorded);
    if(!exclusive.known)fprintf(stderr,"Exclusive fixture: %s\n",exclusive.detail.c_str());
    requireTarget(exclusive.known&&!exclusive.pieces.empty(),"recorded Cacodemon has a remaining exclusive Back upper area");
    const auto recordedReference=syncBuildReferenceGuide(recorded);
    requireTarget(recordedReference.known&&!syncExclusiveContains(recordedReference,recorded.ranges.playerOrigin),"anchoring to the enemy does not restore the recorded false-green Back/Right overlap");
    requireTarget(!syncExclusiveContains(exclusive,recorded.ranges.playerOrigin),"recorded Back/Right overlap is removed rather than recoloured");
    const Vec3 backOnly{-5.5275612f,-346.13031f,32.2000046f};
    requireTarget(syncExclusiveContains(exclusive,backOnly),"position behind the Cacodemon outside all three competing volumes is retained");
    requireTarget(syncExclusiveDestination(exclusive,recorded.ranges.playerOrigin,destination)&&syncExclusiveContains(exclusive,destination),"movement destination leads out of the recorded overlap into exclusive Back upper geometry");
    for(const auto& other:exclusive.competitors)requireTarget(!syncRangeContains(other,destination),"gold destination is outside every competing native volume");
    for(const auto& piece:exclusive.pieces)for(size_t i=1;i+1<piece.size();++i)for(auto weights:std::array<std::array<double,3>,5>{{{{1,0,0}},{{0,1,0}},{{0,0,1}},{{.5,.5,0}},{{1./3,1./3,1./3}}}}){
        Vec3 point{float(piece[0].x*weights[0]+piece[i].x*weights[1]+piece[i+1].x*weights[2]),float(piece[0].y*weights[0]+piece[i].y*weights[1]+piece[i+1].y*weights[2]),exclusive.z};
        requireTarget(syncExclusiveNativeContains(exclusive,point),"filled triangles and their interiors contain no competing native animation area");}
    for(const auto& edge:exclusive.boundary){Vec3 midpoint{float((edge.first.x+edge.second.x)*.5),float((edge.first.y+edge.second.y)*.5),exclusive.z};
        requireTarget(syncExclusiveNativeContains(exclusive,midpoint),"remaining outline excludes native overlaps as well as its fill");}
    auto partial=recorded;partial.regionState="partial facing preview";
    requireTarget(!syncBuildExclusiveGuide(partial).known,"unknown rules hide exclusive geometry instead of claiming no overlap");
    partial=recorded;partial.regions[1].minXY=.5f;
    requireTarget(!syncBuildExclusiveGuide(partial).known,"unsupported competing constraints cannot silently be skipped");
    auto aimFiltered=recorded;aimFiltered.aim.known=true;aimFiltered.aim.quadrant=1;aimFiltered.regions[0].aim.quadrants=1;
    for(size_t i=1;i<aimFiltered.regions.size();++i)aimFiltered.regions[i].aim.quadrants=2;
    auto aimExclusive=syncBuildExclusiveGuide(aimFiltered);
    requireTarget(aimExclusive.known&&syncExclusiveContains(aimExclusive,aimFiltered.ranges.playerOrigin),"a competing GK forbidden by current aim no longer erases the chosen standing area");
    aimFiltered.aim.quadrant=2;aimExclusive=syncBuildExclusiveGuide(aimFiltered);
    requireTarget(aimExclusive.known&&aimExclusive.pieces.empty()&&aimExclusive.detail.find("Aim at top left")!=std::string::npos,"selected animation with wrong aim area draws no positional promise");
    aimFiltered.aim.quadrant=3;aimExclusive=syncBuildExclusiveGuide(aimFiltered);
    requireTarget(aimExclusive.known&&!syncExclusiveContains(aimExclusive,aimFiltered.ranges.playerOrigin),"centre-line aim matching both masks retains the real ambiguity");
    aimFiltered.aim.known=false;requireTarget(!syncBuildExclusiveGuide(aimFiltered).known,"missing native aim reference hides restricted positional geometry");
    aimFiltered.aim.known=true;aimFiltered.aim.quadrant=1;aimFiltered.regions[0].aim.damageGroups={"head"};
    requireTarget(!syncBuildExclusiveGuide(aimFiltered).known,"joint-focus history requirements cannot be falsely claimed supported by current sphere-group hits");
    aimFiltered.jointFocus={true,"head"};requireTarget(syncBuildExclusiveGuide(aimFiltered).known,"verified native head focus enables a head-dependent region");
    aimFiltered.jointFocus.group="left_arm";auto wrongJoint=syncBuildExclusiveGuide(aimFiltered);requireTarget(wrongJoint.known&&wrongJoint.pieces.empty(),"wrong native joint group does not display the selected head region");
    auto focused=geometry;focused.jointFocus={true,"head"};focused.regions[0].aim.damageGroups={"head"};auto armCompetitor=focused.regions[0];armCompetitor.name="arm_only";armCompetitor.aim.damageGroups={"left_arm"};focused.regions.push_back(armCompetitor);
    requireTarget(syncExclusiveContains(syncBuildReferenceGuide(focused),focused.ranges.playerOrigin),"a native focus mismatch removes an otherwise identical competing arm animation");
    auto aboveRegion=geometry;aboveRegion.regions[0].minZ=.031f;aboveRegion.ranges.playerOrigin.z=0;
    requireTarget(syncBuildReferenceGuide(aboveRegion).known&&syncBuildReferenceGuide(aboveRegion).pieces.empty(),"native lower absolute-height threshold excludes the grounded zero-height case");
    for(float z:{-.5f,.5f}){aboveRegion.ranges.playerOrigin.z=z;requireTarget(syncExclusiveContains(syncBuildReferenceGuide(aboveRegion),aboveRegion.ranges.playerOrigin),"both signs of the native absolute height band are supported without inferring above/below from animation names");}
    auto wide=geometry;wide.regions[0].halfYaw=100*syncFacingPi/180;
    const auto wideGuide=syncBuildExclusiveGuide(wide);
    requireTarget(wideGuide.known&&syncExclusiveContains(wideGuide,{-.2615f,2.9886f,-1.6f})&&!syncExclusiveContains(wideGuide,{-1,0,-1.6f}),"a native 200-degree sector retains its greater-than-half-circle tails and removes the opposite wedge");
    const auto wideSafe=syncBuildReferenceGuide(wide);
    requireTarget(wideSafe.known&&!syncExclusiveContains(wideSafe,{-.2615f,2.9886f,-1.6f}),"wide-sector tails also require full player-footprint clearance");
    auto wideOther=geometry;auto wideRival=wideOther.regions[0];wideRival.name="wide_competitor";wideRival.halfYaw=100*syncFacingPi/180;wideRival.incoming={0,1,0};wideOther.regions.push_back(wideRival);
    const auto wideOtherGuide=syncBuildExclusiveGuide(wideOther);
    requireTarget(wideOtherGuide.known&&!syncExclusiveContains(wideOtherGuide,{2,0,-1.6f})&&syncExclusiveContains(wideOtherGuide,{1.5321f,-1.2856f,-1.6f}),"a wide competing sector is subtracted as the union of its convex halves");
    wide.regions[0].halfYaw=syncFacingPi;requireTarget(syncExclusiveContains(syncBuildExclusiveGuide(wide),{-1,0,-1.6f}),"180-degree half-yaw means a full circle, not an invented front/back split");
    auto box=[](float x,float half){SyncRangeVolume v;v.balls.push_back({{0,0,0},100,false});
        v.planes={{{1,0,0},-double(x+half)},{{-1,0,0},double(x-half)},{{0,1,0},-half},{{0,-1,0},-half},{{0,0,1},-2},{{0,0,-1},-2}};return v;};
    auto holes=syncExclusiveBuild(box(0,5),{box(1,.5f),box(3,.5f)},0);
    requireTarget(holes.known&&!holes.pieces.empty()&&!syncExclusiveContains(holes,{1,0,0})&&syncExclusiveContains(holes,{2,0,0}),"subtraction retains disconnected exclusive pieces around multiple holes");
    const auto intervals=syncExclusiveApproach(holes,{4,0,0});
    requireTarget(intervals.size()==3&&intervals[0].second<.5&&intervals[1].first>1.5&&intervals[1].second<2.5&&intervals[2].first>3.5,"exclusive distance intervals preserve both excluded gaps instead of spanning them");
    auto covered=syncExclusiveBuild(box(0,5),{box(0,6)},0);
    requireTarget(covered.known&&covered.pieces.empty()&&!syncExclusiveDestination(covered,{0,0,0},destination),"fully covered area has no displayed footprint or movement destination");
    auto repeated=syncExclusiveBuild(box(0,5),{box(0,1),box(0,1)},0);
    requireTarget(repeated.known&&!repeated.boundary.empty()&&!syncExclusiveContains(repeated,{0,0,0})&&syncExclusiveContains(repeated,{3,0,0}),"duplicate overlapping exclusions retain the true outer and hole boundaries");
    auto lowCompetitor=box(0,2);lowCompetitor.planes[4].d=lowCompetitor.planes[5].d=-.5;
    auto above=syncExclusiveBuild(box(0,5),{lowCompetitor},1);
    requireTarget(above.known&&syncExclusiveContains(above,{0,0,1}),"competing animation outside its native height band does not erase valid exclusive positions");
    SyncRangeVolume circle;circle.balls={{{0,0,0},10,false},{{0,0,0},4,true}};auto inner=circle;inner.balls[1].radius=2;
    auto ring=syncExclusiveBuild(circle,{inner},0);double ringArea=0;
    for(const auto& piece:ring.pieces){ringArea+=syncExclusiveArea(piece);for(size_t i=1;i+1<piece.size();++i){Vec3 center{float((piece[0].x+piece[i].x+piece[i+1].x)/3),float((piece[0].y+piece[i].y+piece[i+1].y)/3),0};
        requireTarget(syncExclusiveNativeContains(ring,center),"curved exclusions leave no triangle bridge across the removed circle");}}
    requireTarget(ring.known&&fabs(ringArea-12*syncFacingPi)<.2&&!syncExclusiveContains(ring,{1.99f,0,0})&&syncExclusiveContains(ring,{3,0,0}),"curved competitor is conservatively subtracted while retaining the surrounding area");
    auto unionInside=[&](const SyncExclusiveGuide& guide,SyncExclusivePoint p){for(const auto& piece:guide.pieces)if(syncExclusiveInside(piece,p))return true;return false;};
    for(const auto& edge:ring.boundary){const double dx=edge.second.x-edge.first.x,dy=edge.second.y-edge.first.y,length=hypot(dx,dy);if(length<.0001)continue;
        const SyncExclusivePoint middle{(edge.first.x+edge.second.x)*.5,(edge.first.y+edge.second.y)*.5};const double nx=-dy/length*1e-5,ny=dx/length*1e-5;
        requireTarget(unionInside(ring,{middle.x+nx,middle.y+ny})!=unionInside(ring,{middle.x-nx,middle.y-ny}),"outline contains only real outer/hole edges, not artificial convex partition seams");}
    const auto benchmarkStart=std::chrono::steady_clock::now();
    for(unsigned n=0;n<32;++n){auto rebuilt=syncBuildExclusiveGuide(recorded);requireTarget(rebuilt.known&&!rebuilt.pieces.empty(),"exclusive Cacodemon calculation remains bounded and repeatable");}
    printf("Exclusive recorded Cacodemon geometry mean %.3f ms (offline CPU, not game FPS).\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-benchmarkStart).count()/32);
    wchar_t fixturePath[32768]{};const DWORD fixtureLength=GetEnvironmentVariableW(L"LABPLUS_SYNC_EXCLUSIVE_FIXTURE_OUTPUT",fixturePath,32768);
    if(fixtureLength&&fixtureLength<32768){std::ofstream dump{std::filesystem::path(fixturePath)};dump.precision(12);
        for(size_t i=0;i<exclusive.pieces.size();++i)for(auto p:exclusive.pieces[i])dump<<"P\t"<<i<<'\t'<<p.x<<'\t'<<p.y<<'\n';
        for(const auto& edge:exclusive.boundary)dump<<"L\t"<<edge.first.x<<'\t'<<edge.first.y<<'\t'<<edge.second.x<<'\t'<<edge.second.y<<'\n';
        dump<<"T\t"<<recorded.facing.origin.x<<'\t'<<recorded.facing.origin.y<<'\n'<<"U\t"<<recorded.ranges.playerOrigin.x<<'\t'<<recorded.ranges.playerOrigin.y<<'\n';
        if(syncExclusiveDestination(exclusive,recorded.ranges.playerOrigin,destination))dump<<"D\t"<<destination.x<<'\t'<<destination.y<<'\n';}
    std::vector<Vertex> vertices;appendSyncTargetGeometry(camera,geometry,vertices,false);
    requireTarget(!vertices.empty()&&vertices.size()%3==0,"exclusive sector has a translucent fill");
    for(const auto& v:vertices)requireTarget(syncPreviewFinite(v.position)&&v.color[3]>0&&v.color[3]<.3f&&syncRangeContains(volume,{v.position.x,v.position.y,geometry.ranges.playerOrigin.z},-.0001),"filled sector retains native XY constraints at the evaluated foot height");
    vertices.clear();appendSyncTargetGeometry(camera,geometry,vertices,true);requireTarget(!vertices.empty()&&vertices.size()%2==0&&vertices.size()<2400,"sector draws only its external boundary");
    std::vector<Vertex> again;appendSyncTargetGeometry(camera,geometry,again,true);requireTarget(vertices.size()==again.size()&&!memcmp(vertices.data(),again.data(),vertices.size()*sizeof(Vertex)),"stationary geometry and colour remain deterministic");
    geometry.ranges.known=false;vertices.clear();appendSyncTargetGeometry(camera,geometry,vertices,false);requireTarget(vertices.empty(),"missing range data never falls back to a body-sized diagram");geometry.ranges.known=true;
    auto shortHeight=geometry.regions[0];shortHeight.maxZ=.5f;
    requireTarget(syncRangeBuild(&shortHeight,geometry.facing,geometry.ranges,false,0,volume),"thin native height band retains a valid interior");
    for(Vec3 direction:std::vector<Vec3>{{0,0,1},{0,0,-1},{1,0,0}}){requireTarget(syncRangeRay(volume,direction,boundary)&&fabsf(boundary.z)<=.5001f,"native height cap clips the visible volume");}
    requireTarget(syncRangeBuild(nullptr,geometry.facing,geometry.ranges,true,0,volume)&&syncRangeContains(volume,{9.9f,0,-1.6f})&&!syncRangeContains(volume,{10.1f,0,-1.6f}),"Chainsaw uses its own camera-space radius with the feet offset");
    geometry.action=1;vertices.clear();appendSyncTargetGeometry(camera,geometry,vertices,true);requireTarget(vertices.size()==24,"missing native selector poses leave only the enemy frame, without invented Chainsaw sectors");
    geometry.selectedAnimation="cut_front";requireTarget(syncSelectedVolume(geometry,selectedVolume,selected),"front Chainsaw variant selects shared native range");
    const auto frontSection=syncGuideSection(selectedVolume,-1.6f);geometry.selectedAnimation="cut_back";
    requireTarget(syncSelectedVolume(geometry,selectedVolume,selected)&&syncGuideSection(selectedVolume,-1.6f).size()==frontSection.size(),"Chainsaw variants do not invent separate directional regions");
    auto aimedChainsaw=geometry;aimedChainsaw.selectedAnimation="cut_front";aimedChainsaw.aim.known=true;aimedChainsaw.aim.quadrant=1;
    aimedChainsaw.chainsawRules={{"cut_front",SyncAimRequirement{1,true,{}},true},{"cut_back",SyncAimRequirement{2,true,{}},true}};
    auto chainsawExclusive=syncBuildExclusiveGuide(aimedChainsaw);
    requireTarget(chainsawExclusive.known&&syncExclusiveContains(chainsawExclusive,{-9,0,-1.6f})&&syncExclusiveContains(chainsawExclusive,{9,0,-1.6f}),"unique Chainsaw area uses native aim masks and its range, including behind the target without GK facing sectors");
    aimedChainsaw.aim.quadrant=3;chainsawExclusive=syncBuildExclusiveGuide(aimedChainsaw);
    requireTarget(chainsawExclusive.known&&chainsawExclusive.pieces.empty(),"Chainsaw shared-axis ambiguity is excluded rather than choosing a side");
    aimedChainsaw.aim.quadrant=1;aimedChainsaw.chainsawRules[0].quick=false;
    requireTarget(!syncBuildExclusiveGuide(aimedChainsaw).known,"non-QUICK Chainsaw rules are not replaced with the quick bypass");
    auto poseChainsaw=geometry;poseChainsaw.selectedAnimation="cut_front";poseChainsaw.selectionKnown=true;poseChainsaw.selectionTolerance=.381f;
    poseChainsaw.chainsawRules={{"cut_front",{},true,true,1.25f,{-2,0,-1.6f}},{"cut_back",{},true,true,1.25f,{2,0,-1.6f}}};
    const auto poseGuide=syncBuildExclusiveGuide(poseChainsaw);
    const auto chainsawReference=syncBuildReferenceGuide(poseChainsaw);const auto chainsawReferenceMesh=syncBuildSectorMesh(chainsawReference,geometryBox);
    auto chainsawAimShift=poseChainsaw;chainsawAimShift.ranges.aimOrigin={3,4,5};chainsawAimShift.ranges.modelViewOffset=1.2f;chainsawAimShift.ranges.eyeOffset.x=.6f;chainsawAimShift.ranges.eyeOffset.y=-.4f;
    const auto chainsawShiftReference=syncBuildReferenceGuide(chainsawAimShift);const auto chainsawShiftMesh=syncBuildSectorMesh(chainsawShiftReference,geometryBox);
    requireTarget(chainsawReferenceMesh.known&&chainsawShiftMesh.known&&!chainsawReferenceMesh.fill.empty()&&chainsawReferenceMesh.fill.size()==chainsawShiftMesh.fill.size()&&chainsawReferenceMesh.outline.size()==chainsawShiftMesh.outline.size()&&
        !memcmp(chainsawReferenceMesh.fill.data(),chainsawShiftMesh.fill.data(),chainsawReferenceMesh.fill.size()*sizeof(Vec3))&&!memcmp(chainsawReferenceMesh.outline.data(),chainsawShiftMesh.outline.data(),chainsawReferenceMesh.outline.size()*sizeof(Vec3)),
        "Chainsaw reference range also remains attached to the enemy while current native selector poses stay unchanged");
    requireTarget(poseGuide.known&&poseGuide.currentPoses&&!poseGuide.pieces.empty(),"equal native priorities use start-pose distance instead of subtracting an identical activation sphere");
    const auto bodyPoseGuide=syncBuildExclusiveGuide(poseChainsaw,true);
    requireTarget(syncExclusiveContains(poseGuide,{-.3f,0,-1.6f})&&!syncExclusiveContains(bodyPoseGuide,{-.3f,0,-1.6f})&&syncExclusiveContains(bodyPoseGuide,{-5,0,-1.6f}),"Chainsaw random-choice clearance expands by the full player footprint rather than testing only its centre");
    requireTarget(syncExclusiveContains(poseGuide,{-5,0,-1.6f})&&!syncExclusiveContains(poseGuide,{5,0,-1.6f})&&!syncExclusiveContains(poseGuide,{0,0,-1.6f}),"distance-choice region includes the chosen side and excludes the entire random tie band");
    size_t included=0;
    for(float x=-9;x<=9;x+=.2f)for(float y=-9;y<=9;y+=.2f)if(syncExclusiveContains(poseGuide,{x,y,-1.6f})){
        ++included;const double a=sqrt((double(x)+2)*(x+2)+double(y)*y),b=sqrt((double(x)-2)*(x-2)+double(y)*y);
        requireTarget(b>a+.381&&sqrt(double(x)*x+double(y)*y)<10,"every displayed distance-choice sample is strictly outside the native random band and inside activation range");}
    requireTarget(included>1000,"inner approximation retains a useful part of the unique area");
    for(const auto& polygon:poseGuide.pieces)for(size_t i=0;i<polygon.size();++i){auto a=polygon[i],b=polygon[(i+1)%polygon.size()];
        requireTarget(syncExclusiveNativeContains(poseGuide,{float((a.x+b.x)/2),float((a.y+b.y)/2),-1.6f}),"clipped edges never cross the native random band");}
    auto samePose=poseChainsaw;samePose.chainsawRules[1].selectionPosition=samePose.chainsawRules[0].selectionPosition;
    requireTarget(syncBuildExclusiveGuide(samePose).pieces.empty(),"identical selector sites have no unique distance-choice region");
    auto priorityPose=poseChainsaw;priorityPose.chainsawRules[1].priority=2;
    requireTarget(syncBuildExclusiveGuide(priorityPose).known&&syncBuildExclusiveGuide(priorityPose).pieces.empty(),"higher competing native priority prevents a false chosen region");
    priorityPose.chainsawRules[1].priority=1;const auto priorityGuide=syncBuildExclusiveGuide(priorityPose);
    requireTarget(syncExclusiveContains(priorityGuide,{5,0,-1.6f}),"sole highest priority is independent of a closer lower-priority pose");
    poseChainsaw.chainsawRules[1].selectionKnown=false;
    requireTarget(!syncBuildExclusiveGuide(poseChainsaw).known,"missing runtime pose data cannot fall back to invented front/back sectors");
    const auto poseMesh=syncBuildSectorMesh(poseGuide,geometryBox);
    requireTarget(poseMesh.known&&!poseMesh.fill.empty()&&!poseMesh.outline.empty()&&poseMesh.outline.size()<4000,"current-pose sector builds bounded closed surfaces and a seam-free outline");
    size_t verticals=0;
    for(size_t i=0;i<poseMesh.outline.size();i+=2){const auto a=poseMesh.outline[i],b=poseMesh.outline[i+1];
        requireTarget(a.z==geometryBox[2]||a.z==geometryBox[5],"sector outline uses the enemy box height");
        requireTarget(b.z==geometryBox[2]||b.z==geometryBox[5],"sector outline ends at the enemy box height");
        if(a.z!=b.z){++verticals;requireTarget(a.x==b.x&&a.y==b.y,"only true corners get vertical boundary lines");}
        else {const double dx=b.x-a.x,dy=b.y-a.y,length=hypot(dx,dy);if(length<1e-6)continue;
            const float x=(a.x+b.x)*.5f,y=(a.y+b.y)*.5f,nx=float(-dy/length*1e-4),ny=float(dx/length*1e-4);
            auto inside=[&](float px,float py){return syncExclusiveContains(poseGuide,{px,py,-1.6f})&&!(px>geometryBox[0]&&px<geometryBox[3]&&py>geometryBox[1]&&py<geometryBox[4]);};
            requireTarget(inside(x+nx,y+ny)!=inside(x-nx,y-ny),"outline has no internal triangle fan or partition seam");}}
    requireTarget(verticals>0&&verticals<20,"smooth sector arcs have no vertical picket fence");
    for(size_t i=0;i<poseMesh.fill.size();i+=3){Vec3 middle{};for(unsigned j=0;j<3;++j){const auto p=poseMesh.fill[i+j];
        requireTarget(p.z==geometryBox[2]||p.z==geometryBox[5],"fill is extruded exactly between enemy box bottom and top");
        requireTarget(syncExclusiveNativeContains(poseGuide,{p.x,p.y,-1.6f}),"extruded vertices retain the current-pose exclusion predicates in XY");middle=motionAdd(middle,motionScale(p,1.f/3));}
        requireTarget(!(middle.x>geometryBox[0]+1e-5&&middle.x<geometryBox[3]-1e-5&&middle.y>geometryBox[1]+1e-5&&middle.y<geometryBox[4]-1e-5),"sector fill stops at the enemy box instead of covering its interior");}
    const auto ringMesh=syncBuildSectorMesh(ring,geometryBox);requireTarget(ringMesh.known&&!ringMesh.fill.empty(),"sector extrusion retains holes");
    for(size_t i=0;i<ringMesh.fill.size();i+=3){const auto p=motionScale(motionAdd(motionAdd(ringMesh.fill[i],ringMesh.fill[i+1]),ringMesh.fill[i+2]),1.f/3);
        requireTarget(hypot(p.x,p.y)>1.99,"no cap or side face bridges the excluded central animation area");}
    SyncRangeVolume drawingVolume;drawingVolume.balls={{{0,0,0},8,true}};drawingVolume.planes={{{1,1,0},0},{{1,-1,0},0}};
    const auto drawingGuide=syncExclusiveBuild(drawingVolume,{},0);const float drawingBox[6]{-1,-1,0,1,1,3};const auto drawingMesh=syncBuildSectorMesh(drawingGuide,drawingBox);
    requireTarget(drawingMesh.known&&!drawingMesh.fill.empty(),"example wedge meets the box faces and has a filled height");
    unsigned dividers=0;for(size_t i=0;i<drawingMesh.outline.size();i+=2){const auto a=drawingMesh.outline[i],b=drawingMesh.outline[i+1];
        if(a.z==b.z||hypot(a.x,a.y)<7.9)continue;const double angle=fabs(remainder(atan2(double(a.y),double(a.x))-syncFacingPi,2*double(syncFacingPi)));
        if(fabs(angle-syncFacingPi/12)<.01){++dividers;requireTarget(a.x==b.x&&a.y==b.y&&a.z==drawingBox[2]&&b.z==drawingBox[5],"outer-arc divider spans exactly the enemy box height");}}
    requireTarget(dividers==2,"exactly two additional verticals divide the outer sector arc at one third and two thirds");
    {std::ofstream dump("build/sync-sector-v9-mesh.tsv");for(const auto& v:drawingMesh.fill)dump<<"F\t"<<v.x<<'\t'<<v.y<<'\t'<<v.z<<'\n';for(const auto& v:drawingMesh.outline)dump<<"L\t"<<v.x<<'\t'<<v.y<<'\t'<<v.z<<'\n';}
    auto benchPose=poseChainsaw;benchPose.chainsawRules[1].selectionKnown=true;const auto poseAt=std::chrono::steady_clock::now();for(unsigned i=0;i<40;++i){const auto g=syncBuildExclusiveGuide(benchPose);const auto mesh=syncBuildSectorMesh(g,geometryBox);requireTarget(mesh.known,"sector geometry remains within its work budget");}
    printf("Current-pose guide plus filled sector mean %.3f ms (offline CPU, not game FPS).\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-poseAt).count()/40);
    requireTarget(syncAimMaskLabel(0x7fffffff)=="no aim-quadrant restriction","unrestricted quadrant label does not imply that every aim condition is absent");
    auto guideLines=syncPositionGuideLines(geometry);requireTarget(std::any_of(guideLines.begin(),guideLines.end(),[](const auto& s){return s.find("Distance:")!=std::string::npos&&s.find(" / ")!=std::string::npos;}),"HUD shows actual distance together with the camera limit");
    geometry.selectedAnimation.clear();
    geometry.hasChainsaw=false;vertices.clear();appendSyncTargetGeometry(camera,geometry,vertices,true);requireTarget(vertices.size()==24,"no native Chainsaw interaction means only the enemy frame remains");
    requireTarget(syncFacingContains(0,syncFacingPi/3,59*syncFacingPi/180)&&!syncFacingContains(0,syncFacingPi/3,61*syncFacingPi/180),"native sixty-degree boundary is not replaced by equal quadrants");
    SyncFacingPose rotated{{0,1,0,-1,0,0,0,0,1},{0,0,0}};
    requireTarget(fabsf(syncFacingYaw(rotated,{1,0,0})-syncFacingPi/2)<1e-6f,"diagram rotates with actual target facing");
    SyncCandidatePresentation presentation;SyncTargetFrame observed;observed.level=scene.level;observed.entity=scene.enemies[0].ref;observed.entity.slot=1;observed.candidatesReady=true;
    unsigned changes=0;std::vector<std::string> previousChoices;
    for(ULONGLONG t=0;t<1000;t+=4){observed.chainsaw=t%8?std::vector<std::string>{"cut_front"}:std::vector<std::string>{"cut_back"};presentation.update(observed,t);if(presentation.chainsaw!=previousChoices){++changes;previousChoices=presentation.chainsaw;}}
    requireTarget(presentation.ready&&presentation.chainsaw==std::vector<std::string>({"cut_back","cut_front"})&&changes<=8,"rapid per-frame candidates are merged into readable bounded presentation windows");
    observed.chainsaw.clear();for(ULONGLONG t=1000;t<=1300;t+=4)presentation.update(observed,t);
    requireTarget(presentation.ready&&presentation.chainsaw.empty(),"completed empty windows remove old candidates");
    observed.entity.id++;observed.chainsaw={"cut_other_enemy"};presentation.update(observed,1301);
    requireTarget(!presentation.ready&&presentation.chainsaw.empty(),"changing target clears all previous names immediately");
    observed.candidatesReady=false;for(ULONGLONG t=1305;t<=1605;t+=4)presentation.update(observed,t);
    requireTarget(!presentation.ready&&presentation.chainsaw.empty(),"incoherent-only window is marked checking, not ready");
    requireTarget(syncZoneLabel("kill_front_lower")=="Front lower","UI labels retain native animation meaning");
    printf("PASS: aimed target identity, native action candidates, authored facing regions, native range intersections and stable HUD presentation.\n");
}
