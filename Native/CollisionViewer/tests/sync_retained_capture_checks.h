#pragma once

static void checkRetainedCaptureLifecycle() {
    TargetCaptureFixture scene;const WallMemory memory;auto commit=[&](size_t at){requireTarget(VirtualAlloc(scene.image+(at&~size_t(4095)),4096,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit native retention fixture pages");};
    for(size_t at:{size_t(0x4000),size_t(0x5000),size_t(0xc293f0),size_t(0xd41ed0),size_t(0x61c9630),size_t(0x61a99b0)})commit(at);
    commit(kProfile11.playerVtableRva+0x620);
    const unsigned char monsterCode[]{0x48,0x8b,0x81,0xf0,0x56,0,0,0x48,0x85,0xc0,0x74,8,0x48,0x8b,0x80,0xf0,1,0,0,0xc3},
        poseCode[]{0x48,0x8d,0x81,0x2c,0x3d,0,0,0xc3},bindCode[]{0x48,0x8d,0x81,0x20,0x1e,0,0,0xc3};
    memcpy(scene.image+0xc293f0,monsterCode,sizeof(monsterCode));memcpy(scene.image+0xd41ed0,poseCode,sizeof(poseCode));memcpy(scene.image+0x5000,bindCode,sizeof(bindCode));
    const auto monsterGetter=scene.base+0xc293f0,poseGetter=scene.base+0xd41ed0,bindGetter=scene.base+0x5000;
    memcpy(scene.image+0x1000+0x1098,&monsterGetter,8);memcpy(scene.image+0x1000+0x620,&bindGetter,8);memcpy(scene.image+0x4000+0x70,&poseGetter,8);
    memcpy(scene.image+kProfile11.playerVtableRva+0x620,&bindGetter,8);
    scene.player.resize(0x3b100);const auto player=targetAddress(scene.player);targetPut(scene.player,0,scene.base+kProfile11.playerVtableRva);
    targetPut(scene.map,0x1ad0,player);targetPut(scene.view,0xc8,player);targetPut(scene.player,0x1d0,player+0x7d28);
    const float axis[]{1.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f};memcpy(scene.player.data()+0x377a4,axis,36);
    std::vector<unsigned char> aim(0x200),csRow(24),maxView(64),quick(64),behavior(0x200),component(0x3d80),otherComponent(0x3d80);
    targetPut(behavior,0x1f0,uint64_t(1));targetPut(aim,0x19c,6.f);targetPut(csRow,0,uint64_t(1));targetPut(csRow,12,10.f);
    targetPut(aim,0x1c0,SyncPreviewList{targetAddress(csRow),1,1,0});targetPut(scene.player,0x70e8+0x210,targetAddress(aim));
    auto cvar=[&](size_t at,std::vector<unsigned char>& data,const char* name,float value){const auto p=targetAddress(data);memcpy(scene.image+at,&p,8);targetPut(data,12,value);targetPut(data,40,reinterpret_cast<uintptr_t>(name));};
    cvar(0x61c9630,maxView,"viewInfo_maxMeleeDistance",20);cvar(0x61a99b0,quick,"sync_quickChainsawIgnoreLungeDistValidation",1);
    targetPut(scene.enemies[0].actor,0x56f0,targetAddress(behavior));targetPut(scene.enemies[0].actor,0x58a0,targetAddress(component));
    targetPut(component,0x18,scene.base+0x4000);targetPut(component,0x18+0x70,scene.level.map);memcpy(component.data()+0x18+0x3d2c,axis,36);targetPut(component,0x18+0xb0,Vec3{0,4,0});targetPut(scene.info,0x30,Vec3{0,4,0});
    targetPut(scene.enemies[1].actor,0x56f0,targetAddress(behavior));targetPut(scene.enemies[1].actor,0x58a0,targetAddress(otherComponent));
    targetPut(otherComponent,0x18,scene.base+0x4000);targetPut(otherComponent,0x18+0x70,scene.level.map);memcpy(otherComponent.data()+0x18+0x3d2c,axis,36);targetPut(otherComponent,0x18+0xb0,Vec3{0,8,0});targetPut(scene.info,0x130,Vec3{0,8,0});
    std::unordered_map<std::string,GraphType> schemas;GraphLookup lookup=[&](const std::string& name)->const GraphType&{return schemas[name];};
    SyncTargetReader reader{0};GameCamera camera{};camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    auto capture=[&](const GameCamera& view){return reader.capture(memory,scene.base,kProfile11,scene.level,view,50,&lookup);};
    const auto aimed=capture(camera);requireTarget(aimed.entity&&!aimed.retainedTarget&&aimed.ranges.known&&aimed.ranges.gloryKillTarget==6,"full native reader seeds retention only after finding an aimed enemy with verified ranges");
    auto away=camera;away.matrix[0]=away.matrix[13]=0;away.matrix[1]=away.matrix[12]=1;
    auto held=capture(away);requireTarget(held.entity.address==aimed.entity.address&&held.retainedTarget&&held.phase=="ready","full capture retains the last living hostile target after a camera turn within radius");
    held=capture(away);requireTarget(held.entity.address==aimed.entity.address&&held.retainedTarget,"successive camera-away samples keep the same validated target rather than expiring the original reticle frame");
    targetPut(scene.player,0x7d28+0xb0,Vec3{0,-4,0});targetPut(scene.view,0,Vec3{0,-4,0});away.position[1]=-4;
    requireTarget(!capture(away).entity,"full native refresh releases the target when the player exits GK range");
    targetPut(scene.player,0x7d28+0xb0,Vec3{});targetPut(scene.view,0,Vec3{});away.position[1]=0;
    requireTarget(!capture(away).entity,"returning inside radius does not reacquire an enemy without aiming after retention was released");
    requireTarget(bool(capture(camera).entity),"aiming back reacquires the target");scene.enemies[0].actor[0x6488]=1;
    requireTarget(!capture(away).entity,"a dead retained actor disappears immediately");scene.enemies[0].actor[0x6488]=0;
    requireTarget(bool(capture(camera).entity),"living target can be aimed again");targetPut(scene.entries,4,uint32_t(2));
    requireTarget(!capture(away).entity,"a reused entity slot cannot retain the previous actor generation");
    requireTarget(bool(capture(camera).entity),"new actor generation is acquired through a fresh aim");targetPut(scene.enemies[0].targeting,0xc4,uint32_t(2));
    requireTarget(!capture(away).entity,"a target that becomes friendly cannot remain retained");targetPut(scene.enemies[0].targeting,0xc4,uint32_t(1));
    requireTarget(bool(capture(camera).entity),"hostile target can be acquired again");reader.clearRetainedTarget();requireTarget(!capture(away).entity,"disabling or hiding gameplay explicitly clears retention");
    requireTarget(bool(capture(camera).entity),"target can be aimed before a map transition");auto other=scene.level;other.name="fixture/another-map";
    requireTarget(!reader.capture(memory,scene.base,kProfile11,other,away,50,&lookup).entity,"a map transition clears the retained target even if its address remains readable");
    requireTarget(capture(camera).entity.address==aimed.entity.address,"the first target can be aimed after returning to the original map");
    targetPut(scene.player,0x7d28+0xb0,Vec3{3,5,0});targetPut(scene.view,0,Vec3{3,5,0});auto another=camera;
    another.position[0]=3;another.position[1]=5;another.matrix[0]=another.matrix[1]=1;another.matrix[12]=-1;
    const auto switched=capture(another);requireTarget(switched.entity.address==targetAddress(scene.enemies[1].actor)&&!switched.retainedTarget,"aiming at a different enemy overrides the old retained target even while the old one is still nearby");
    away.position[0]=3;away.position[1]=5;requireTarget(capture(away).entity.address==switched.entity.address,"subsequent camera-away retention follows the newly aimed enemy");
    printf("PASS: native target retention across camera turns, radius exit, death, hostility, generation, map and reset.\n");
}
