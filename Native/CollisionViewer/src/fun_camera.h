#pragma once

// A first-person animation rig can also drive an external shot while a
// separate cinematic Slayer actor is rendered. Its camera joint alone does
// not identify POV; a currently rendered full actor takes precedence.
enum class FunAnimatedView { Unknown,PlayerPov,External };
static bool funActorCommittedWithRig(uint32_t rigFrame,uint32_t actorFrame) {
    if(rigFrame==~0u||actorFrame==~0u) return false;
    // A camera cut can stop submitting the helper while the full actor keeps
    // rendering. Accept newer actor commits, with two frames of queue skew
    // in the other direction. Unsigned ordering also handles counter wrap.
    return uint32_t(actorFrame-rigFrame)<0x80000000u||uint32_t(rigFrame-actorFrame)<=2;
}
static FunAnimatedView funAnimatedView(bool rigMatch,bool fullActorRendered,bool fullActorPoseMatch) {
    if(fullActorRendered&&!fullActorPoseMatch) return FunAnimatedView::External;
    return rigMatch||fullActorPoseMatch?FunAnimatedView::PlayerPov:FunAnimatedView::Unknown;
}

// The player ownership flag remains true during external cinematic cameras.
// In a cinematic, require the renderer to actually occupy the player's POV.
// Normal gameplay keeps the ownership flag so frame latency and view shake
// cannot flicker between the two audio/colour modes.
static bool funPlayerPov(bool localFirstPerson,bool cinematic,const GameCamera* rendered,const float* firstPersonOrigin,FunAnimatedView animatedView=FunAnimatedView::Unknown) {
    if(!localFirstPerson) return false;
    if(!cinematic) return true;
    if(animatedView==FunAnimatedView::External) return false;
    if(rendered&&animatedView==FunAnimatedView::PlayerPov) return true;
    if(!rendered||!firstPersonOrigin) return false;
    double squared=0;
    for(unsigned i=0;i<3;++i) {
        if(!std::isfinite(rendered->position[i])||!std::isfinite(firstPersonOrigin[i])) return false;
        const double delta=double(rendered->position[i])-firstPersonOrigin[i];squared+=delta*delta;
    }
    // Permit the small camera offsets of first-person animation/shake, while
    // excluding a camera outside the player's head (units are world metres).
    return squared<=.75*.75;
}
