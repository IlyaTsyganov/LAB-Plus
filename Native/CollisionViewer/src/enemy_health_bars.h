#pragma once
#include <iomanip>
#include <sstream>
#include <locale>
#include <utility>
#include "eternal_ui2_font.h"

struct EnemyHealthBarGeometry {
    std::vector<Vertex> triangles,lines;
    size_t bars=0;
};
static std::array<float,4> enemyHealthBarColor(double fraction) {
    const float ratio=static_cast<float>(std::clamp(fraction,0.0,1.0));
    // Keep half health bright yellow instead of a dark red/green mixture.
    return {std::min(1.f,2.f*(1-ratio)),std::min(1.f,2.f*ratio),.06f,1.f};
}
static std::string enemyHealthBarNumber(double value) {
    std::ostringstream output; output.imbue(std::locale::classic()); output<<std::setprecision(6)<<value; return output.str();
}
static std::string enemyHealthBarText(double current,double maximum) {
    return enemyHealthBarNumber(current)+"/"+enemyHealthBarNumber(maximum);
}
static std::string enemyStaggerBarText(double seconds,double maximum=-1) {
    if(!std::isfinite(seconds)||seconds<0||seconds>600) return {};
    std::ostringstream output; output.imbue(std::locale::classic()); output<<"GK "<<std::fixed<<std::setprecision(2)<<seconds;
    if(std::isfinite(maximum)&&maximum>0&&maximum<=600) output<<" / "<<maximum<<" s"; else output<<'s';
    return output.str();
}
// All vertices remain in world space and use the existing depth-aware
// triangle pipeline. Pixel sizing keeps numbers readable at normal distances.
// Validation may refresh both the pose and current health/stagger values in
// the copied row; nothing in this renderer reads or changes game memory.
template<class Validate> static void populateEnemyHealthBars(EnemyHealthBarGeometry& result,const GameCamera& camera,
    const InvisibleNumbersSnapshot& snapshot,const LevelIdentity& level,uint64_t now,float distance,
    uint32_t viewportHeight,Validate&& validate) {
    result.triangles.clear(); result.lines.clear(); result.bars=0;
    if(snapshot.phase!="live"||snapshot.level!=level||now>snapshot.captured+250||snapshot.captured>now+1000||
       !viewportHeight||viewportHeight>32768||!std::isfinite(distance)||distance<=0) return;
    Vec3 right{camera.matrix[0],camera.matrix[1],camera.matrix[2]},up{camera.matrix[4],camera.matrix[5],camera.matrix[6]};
    const float rightLength=motionLength(right),upLength=motionLength(up);
    if(!std::isfinite(rightLength)||!std::isfinite(upLength)||rightLength<1e-5f||upLength<1e-5f) return;
    right=motionScale(right,1/rightLength); up=motionScale(up,1/upLength);
    const Vec3 cameraPosition{camera.position[0],camera.position[1],camera.position[2]};
    if(!motionFinite(cameraPosition)) return;
    std::unordered_set<uintptr_t> seen;
    for(const auto& observed:snapshot.barEnemies) {
        auto row=observed;
        if(!row.positionKnown||!row.maximumKnown||!row.entity||seen.count(row.entity.address)) continue;
        Vec3 top{};
        if(!validate(row,top)||!motionFinite(top)||!std::isfinite(row.maximum)||row.maximum<=0||
           !std::isfinite(row.health)||row.health<=0||motionLength(motionSub(top,cameraPosition))>distance) continue;
        // The reader's top already includes a small margin over the bounds.
        // Raising the whole panel another 0.6m leaves the enemy unobscured.
        Vec3 center=top; center.z+=.6f;
        const float w=camera.matrix[12]*center.x+camera.matrix[13]*center.y+camera.matrix[14]*center.z+camera.matrix[15];
        if(!std::isfinite(w)||w<=.05f) continue;
        const float projectedX=(camera.matrix[0]*center.x+camera.matrix[1]*center.y+camera.matrix[2]*center.z+camera.matrix[3])/w;
        const float projectedY=(camera.matrix[4]*center.x+camera.matrix[5]*center.y+camera.matrix[6]*center.z+camera.matrix[7])/w;
        if(!std::isfinite(projectedX)||!std::isfinite(projectedY)||std::abs(projectedX)>1.4f||std::abs(projectedY)>1.4f) continue;
        // Limit size very close to the camera; otherwise preserve 14px digits.
        const float pixel=std::max(.0001f,2.f*w/(static_cast<float>(viewportHeight)*upLength));
        const double ratio=std::clamp(row.health/row.maximum,0.0,1.0);
        const std::string health=enemyHealthBarText(row.health,row.maximum);
        const std::string stagger=row.staggerKnown&&row.staggerActive?enemyStaggerBarText(row.staggerSeconds,row.staggerDuration>0?static_cast<double>(row.staggerDuration)/999960.0:-1):std::string{};
        auto textWidth=[](const std::string& text) { return eternalUi2TextWidth(text); };
        const float width=std::max(96.f,std::max(textWidth(health),textWidth(stagger))+12.f);
        auto point=[&](float x,float y) { return motionAdd(center,motionAdd(motionScale(right,x*pixel),motionScale(up,y*pixel))); };
        auto quad=[&](float left,float bottom,float rightEdge,float topEdge,const std::array<float,4>& color) {
            if(rightEdge<=left||topEdge<=bottom) return;
            const Vec3 positions[]{point(left,bottom),point(rightEdge,bottom),point(rightEdge,topEdge),point(left,bottom),point(rightEdge,topEdge),point(left,topEdge)};
            for(const auto& position:positions) { Vertex vertex{}; vertex.position=position; memcpy(vertex.color,color.data(),sizeof(vertex.color)); result.triangles.push_back(vertex); }
        };
        auto text=[&](const std::string& value,float baseline,const std::array<float,4>& color) {
            const float x=-textWidth(value)*.5f;
            auto pass=[&](float shadowX,float shadowY,const std::array<float,4>& ink) {
                appendEternalUi2GlyphQuads(value,x+shadowX,-baseline-shadowY,1,ink,
                    [&](float left,float topEdge,float rightEdge,float bottom,float u0,float v0,float u1,float v1,const std::array<float,4>& glyphColor) {
                        const Vec3 positions[]{point(left,-bottom),point(rightEdge,-bottom),point(rightEdge,-topEdge),point(left,-bottom),point(rightEdge,-topEdge),point(left,-topEdge)};
                        const float coordinates[6][2]{{u0,v1},{u1,v1},{u1,v0},{u0,v1},{u1,v0},{u0,v0}};
                        for(size_t index=0;index<6;++index) {
                            Vertex vertex{};vertex.position=positions[index];
                            memcpy(vertex.color,glyphColor.data(),sizeof(vertex.color));
                            memcpy(vertex.texcoord,coordinates[index],sizeof(vertex.texcoord));
                            result.triangles.push_back(vertex);
                        }
                    });
            };
            pass(1,-1,{0,0,0,1});pass(0,0,color);
        };
        const std::array<float,4> frame=stagger.empty()?std::array<float,4>{.025f,.025f,.03f,1}:std::array<float,4>{1,.72f,.12f,1};
        quad(-width*.5f-1.5f,-6,width*.5f+1.5f,6,frame);
        quad(-width*.5f,-4.5f,width*.5f,4.5f,{.16f,.025f,.025f,1});
        quad(-width*.5f,-4.5f,-width*.5f+width*static_cast<float>(ratio),4.5f,enemyHealthBarColor(ratio));
        text(health,10,{1,1,1,1});
        if(!stagger.empty()) text(stagger,28,{1,.8f,.18f,1});
        seen.insert(row.entity.address); ++result.bars;
    }
    return;
}

template<class Validate> static EnemyHealthBarGeometry buildEnemyHealthBars(const GameCamera& camera,
    const InvisibleNumbersSnapshot& snapshot,const LevelIdentity& level,uint64_t now,float distance,
    uint32_t viewportHeight,Validate&& validate) {
    EnemyHealthBarGeometry result;
    populateEnemyHealthBars(result,camera,snapshot,level,now,distance,viewportHeight,std::forward<Validate>(validate));
    return result;
}
