#pragma once
struct FunSpatialMix { float gain=1,pan=0; bool located=false; };
static float funSpatialDistanceGain(float distance) {
    if(!std::isfinite(distance)||distance<0) return 0;
    float gain=12.f/std::max(12.f,distance);
    if(distance>180.f) gain*=std::clamp((240.f-distance)/60.f,0.f,1.f);
    return gain;
}
static FunSpatialMix funSpatialMix(const GameCamera& camera,const float (&source)[3]) {
    float delta[3]{},squared=0,rightSquared=0,rightDot=0;
    for(unsigned i=0;i<3;++i) {
        if(!std::isfinite(source[i])||!std::isfinite(camera.position[i])||!std::isfinite(camera.matrix[i])) return {};
        delta[i]=source[i]-camera.position[i]; squared+=delta[i]*delta[i];
        rightSquared+=camera.matrix[i]*camera.matrix[i]; rightDot+=delta[i]*camera.matrix[i];
    }
    if(!std::isfinite(squared)||!std::isfinite(rightSquared)) return {};
    const float distance=sqrtf(squared);
    FunSpatialMix result; result.located=true;
    // A close shot has the normal helmet volume. Amplitude then follows an
    // gentler inverse-distance curve: full volume through 12 m, then a
    // smooth fade to silence between 180 and 240 m for wide cinematic shots.
    result.gain=funSpatialDistanceGain(distance);
    if(distance>.01f&&rightSquared>.0001f) result.pan=std::clamp(rightDot/(distance*sqrtf(rightSquared)),-.95f,.95f);
    return result;
}

// Panoramic cameras often hide the Slayer's render model entirely. His live
// first-person origin still locates the helmet, so an outside cinematic must
// not lose its music just because no animated visor is submitted this frame.
static FunSpatialMix funCinematicSpatialMix(const GameCamera& camera,FunSpatialMix animated,const float* playerHelmetOrigin) {
    if(!animated.located&&playerHelmetOrigin) {
        float position[3]{};memcpy(position,playerHelmetOrigin,12);
        animated=funSpatialMix(camera,position);
    }
    if(animated.located&&std::isfinite(animated.gain)&&std::isfinite(animated.pan)) {
        // Perceived listener distance is three metres from the helmet.
        // Pan still follows the real helmet/camera direction; ordinary
        // third-person gameplay retains its actual-distance attenuation.
        animated.gain=funSpatialDistanceGain(3.f);
    } else {animated.gain=0;animated.pan=0;animated.located=false;}
    return animated;
}
