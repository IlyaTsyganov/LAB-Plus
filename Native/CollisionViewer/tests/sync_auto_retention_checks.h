#pragma once

static SyncTargetFrame automaticPositionFixture() {
    SyncTargetFrame frame;frame.level={0x10000,0x20000,"fixture/auto-position"};frame.entity={0x30000,0x10000,0x40000,1,1,"idAI2"};
    frame.regionState="ready";frame.ranges.known=true;frame.ranges.gloryKillTarget=10;frame.ranges.chainsawTarget=10;frame.ranges.lunge=10;
    frame.facing.axis[0]=frame.facing.axis[4]=frame.facing.axis[8]=1;frame.aim.known=true;frame.aim.quadrant=1;
    frame.playerFootprint={true,{-.3f,-.3f,0},{.3f,.3f,0}};
    const float bounds[]{-.5f,-.5f,0,.5f,.5f,2};memcpy(frame.shotBounds,bounds,24);
    SyncFacingRegion front;front.name="front";front.incoming={1,0,0};front.halfYaw=syncFacingPi/3;front.maxXY=8;front.maxZ=3;
    auto back=front;back.name="back";back.incoming={-1,0,0};frame.regions={front,back};frame.ranges.playerOrigin={3,0,0};return frame;
}
static void checkAutomaticEnteredArea() {
    auto frame=automaticPositionFixture();SyncAutomaticSelection automatic;automatic.update(frame);
    requireTarget(frame.automaticAnimation=="front","automatic initially chooses the occupied front sector");
    frame.ranges.playerOrigin={-3,0,0};automatic.update(frame);
    requireTarget(frame.automaticAnimation=="back"&&syncExclusiveContains(syncBuildReferenceGuide(frame),frame.ranges.playerOrigin),"entering a back sector replaces the still-drawable previous front sector");
    frame.ranges.playerOrigin={3,0,0};automatic.update(frame);requireTarget(frame.automaticAnimation=="front","automatic follows the player back across sectors");
    frame.ranges.playerOrigin={-.1f,0,0};automatic.update(frame);
    requireTarget(frame.automaticAnimation=="back"&&!syncExclusiveContains(syncBuildReferenceGuide(frame),frame.ranges.playerOrigin)&&
        syncExclusiveContains(syncBuildReferenceGuide(frame,false),frame.ranges.playerOrigin),"entering the visible amber band selects that area even before the full collider fits its core");
    frame.selectedAnimation="front";automatic.update(frame);requireTarget(!frame.automaticResolved&&syncSelectedAnimation(frame)=="front","manual animation selection remains under user control");
    frame.selectedAnimation.clear();frame.action=1;frame.hasChainsaw=true;frame.selectionKnown=true;frame.selectionTolerance=.2f;frame.ranges.ignoreQuickLunge=true;
    frame.chainsawAnimations={"cut_front","cut_back"};frame.chainsawRules={{"cut_front",{},true,true,1,{2,0,0},{}},{"cut_back",{},true,true,1,{-2,0,0},{}}};
    frame.ranges.playerOrigin={3,0,0};automatic.update(frame);requireTarget(frame.automaticAnimation=="cut_front","Chainsaw automatic initially selects the occupied unique distance area");
    frame.ranges.playerOrigin={-3,0,0};automatic.update(frame);requireTarget(frame.automaticAnimation=="cut_back","entering another Chainsaw area also replaces a still-drawable previous cut");
    frame.ranges.playerOrigin={0,0,0};const auto random=syncBuildReferenceGuide(frame,false);
    requireTarget(!syncExclusiveContains(random,frame.ranges.playerOrigin),"automatic never paints the random-choice gap as an entered exclusive area");
    auto arms=automaticPositionFixture();arms.regions[0].name="right_arm";arms.regions[0].aim.damageGroups={"right_arm"};
    arms.regions[1]=arms.regions[0];arms.regions[1].name="left_arm";arms.regions[1].aim.damageGroups={"left_arm"};
    arms.spheres={{{0,0,1},.2f,"",1,true,"right_arm"},{{0,0,1},.2f,"",2,true,"left_arm"},{{0,0,1},.2f,"",3,true,"chest"}};
    arms.jointFocus={true,"chest"};SyncAutomaticSelection body;body.update(arms);
    requireTarget(arms.automaticAnimation=="right_arm"&&syncBuildReferenceGuide(arms).requiredAimReference,"entering a limb zone displays its required aim guide before the player aims at the limb");
    arms.jointFocus={true,"left_arm"};body.update(arms);requireTarget(arms.automaticAnimation=="left_arm","among overlapping limb zones a current matching focus outranks the previously displayed conditional guide");
    auto quadrants=automaticPositionFixture();quadrants.regions[0].aim.quadrants=3;quadrants.regions[1]=quadrants.regions[0];quadrants.regions[1].name="lower";quadrants.regions[1].aim.quadrants=12;
    SyncAutomaticSelection quadrant;quadrant.update(quadrants);requireTarget(quadrants.automaticAnimation=="front","current upper aim wins an overlapping upper/lower position");
    quadrants.aim.quadrant=12;quadrant.update(quadrants);requireTarget(quadrants.automaticAnimation=="lower","current lower aim replaces the previous upper guide inside their shared standing region");
}
static bool sameRetainedPieces(const SyncExclusiveGuide& a,const SyncExclusiveGuide& b) {
    if(a.pieces.size()!=b.pieces.size())return false;for(size_t i=0;i<a.pieces.size();++i)
        if(a.pieces[i].size()!=b.pieces[i].size()||memcmp(a.pieces[i].data(),b.pieces[i].data(),a.pieces[i].size()*sizeof(SyncExclusivePoint)))return false;return true;
}
static void checkRetainedReferenceGeometry() {
    auto frame=automaticPositionFixture();frame.selectedAnimation="front";const auto original=syncBuildReferenceGuide(frame);
    frame.retainedTarget=true;frame.retainedAim=frame.aim;frame.retainedFocus=frame.jointFocus;frame.aim={};frame.jointFocus={};
    frame.ranges.modelViewOffset=syncFacingPi;frame.ranges.aimOrigin={42,35,8};
    const auto retained=syncBuildReferenceGuide(frame);
    requireTarget(retained.known&&sameRetainedPieces(retained,original),"looking away preserves the enemy-anchored reference geometry");
    frame.candidatesReady=true;frame.gloryKills={"front"};const auto feedback=syncGuideFeedback(frame,retained.selected,"front");
    requireTarget(feedback.tone==SyncSectorTone::Waiting&&!feedback.selectedOnlyObserved,"retained geometry cannot produce a green execution confirmation");
    requireTarget(syncTargetWithinActionRadius(frame),"a nearby target stays retained independently of camera direction");
    frame.ranges.playerOrigin={11,0,0};requireTarget(!syncTargetWithinActionRadius(frame),"leaving the GK distance releases the retained target");
    frame.ranges.playerOrigin={3,0,11};requireTarget(!syncTargetWithinActionRadius(frame),"retention uses three-dimensional range, not a horizontal-only circle");
    frame.ranges.playerOrigin={3,0,0};frame.action=1;frame.ranges.chainsawTarget=2;frame.hasChainsaw=true;
    requireTarget(!syncTargetWithinActionRadius(frame),"retention uses the selected action's own distance cap");frame.ranges.chainsawTarget=10;
    requireTarget(syncTargetWithinActionRadius(frame),"a Chainsaw-compatible target can stay retained in its own larger radius");frame.hasChainsaw=false;
    requireTarget(!syncTargetWithinActionRadius(frame),"GK-only enemies cannot acquire a retained Chainsaw area");
    frame.hasChainsaw=true;frame.selectedAnimation="cut_front";frame.chainsawAnimations={"cut_front","cut_back"};
    frame.retainedSelectionKnown=true;frame.retainedSelectionTolerance=.2f;frame.retainedSelectionPose=frame.facing;
    frame.retainedChainsawRules={{"cut_front",{},true,true,1,{2,0,0},{}},{"cut_back",{},true,true,1,{-2,0,0},{}}};frame.ranges.ignoreQuickLunge=true;
    const auto cut=syncBuildReferenceGuide(frame);requireTarget(cut.known&&!cut.pieces.empty()&&cut.detail.find("last aimed Chainsaw")!=std::string::npos,"a retained Chainsaw diagram survives loss of the temporary native candidate pool as an explicit reference");
    requireTarget(!syncNativeSelection(frame).known,"sampled reference selector poses never become current native observations");
    frame.facing.origin={20,-8,0};frame.ranges.playerOrigin={23,-8,0};const auto shifted=syncBuildReferenceGuide(frame);
    requireTarget(syncExclusiveContains(cut,{3,0,0})==syncExclusiveContains(shifted,{23,-8,0}),"the conditional sampled Chainsaw arrangement follows the enemy instead of floating at its old world position");
    GameCamera camera{};auto arm=loadRightArmPose(camera);const auto limb=syncBuildReferenceGuide(arm);
    arm.retainedTarget=true;arm.retainedAim=arm.aim;arm.retainedFocus=arm.jointFocus;arm.aim={};arm.jointFocus={};
    const auto heldLimb=syncBuildReferenceGuide(arm);requireTarget(heldLimb.known&&sameRetainedPieces(heldLimb,limb),"looking away also preserves a selected limb's conditional standing reference");
}
