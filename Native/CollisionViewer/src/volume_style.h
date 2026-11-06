#pragma once
static bool recentVolumeChange(const Mesh& mesh,ULONGLONG now) {
    return mesh.changedAt&&now>=mesh.changedAt&&now-mesh.changedAt<VolumeHistory::pulseMs;
}
static bool volumeShown(const Mesh& mesh,ULONGLONG now) {
    if(mesh.volumeState==EntityStatus::Enabled) return true;
    if(mesh.volumeState==EntityStatus::Disabled) return showDisabledVolumes||(showVolumeHistory&&recentVolumeChange(mesh,now));
    return showVolumeHistory;
}
struct VolumeStyle { std::array<float,4> fill,line; bool filled=true,dashed=false; };
static VolumeStyle volumeStyle(const Mesh& mesh,bool selected,ULONGLONG now) {
    VolumeStyle style;
    // Settings order: World=0, Trigger=1, Wall=2, Inspector=3.
    // Mesh kinds use Trigger=0 and World=1, so their indexes are reversed.
    const size_t appearanceIndex=selected?3:(mesh.kind==3?4:mesh.kind==1?0:(mesh.kind==0?1:2));
    const auto& appearance=volumeAppearances[appearanceIndex];
    const auto& rgb=appearance.color;
    style.fill={rgb[0],rgb[1],rgb[2],appearance.opacity};
    style.line={rgb[0],rgb[1],rgb[2],selected?1.f:(mesh.kind==1?.65f:.9f)};
    style.filled=mesh.volumeState==EntityStatus::Enabled&&!mesh.historical;
    if(mesh.volumeState==EntityStatus::Disabled) style.line[3]=.4f;
    else if(mesh.volumeState==EntityStatus::Removed) { style.line[3]=.35f;style.dashed=true; }
    else if(mesh.volumeState==EntityStatus::Unavailable) { style.line[3]=.3f;style.dashed=true; }
    if(showVolumeHistory&&recentVolumeChange(mesh,now))style.line[3]=1.f;
    if(selected)style.line[3]=1.f;
    return style;
}
