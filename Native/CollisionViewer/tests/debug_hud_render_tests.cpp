#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../src/debug_hud_stack.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <limits>
#include <unordered_set>
struct Vec3 { float x,y,z; };
struct Vertex { Vec3 position;float color[4];float texcoord[2]{-1,-1}; };
#include "../src/debug_hud_render.h"
static void expectHudRender(bool valid,const char* message) {
    if(!valid) { std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1); }
}
static bool nearHudRender(float left,float right) { return std::abs(left-right)<.0002f; }
static void verifyHudLayout(const DebugHudFrame& frame,const DebugHudLayout& layout,uint32_t width,uint32_t height) {
    expectHudRender(layout.fontScale>0&&std::isfinite(layout.fontScale)&&!layout.lines.empty(),"visible frame has a finite positive shared font scale");
    std::vector<std::string> restored(frame.rows.size());std::vector<bool> seen(frame.rows.size(),false);
    for(const auto& line:layout.lines) {
        expectHudRender(line.sourceRow<frame.rows.size(),"layout source identity belongs to the current immutable frame");
        expectHudRender(line.x>=0&&line.x<width&&line.baselineY>=0&&line.baselineY<=height,"all positions use the current viewport");
        expectHudRender(eternalUi2TextWidth(line.text,layout.fontScale)<=layout.columnWidth+.01f,"every long line wraps within its allocated column");
        expectHudRender(line.color==frame.rows[line.sourceRow].color,"wrapping retains section and row colors");
        restored[line.sourceRow]+=line.text;seen[line.sourceRow]=true;
    }
    for(size_t row=0;row<frame.rows.size();++row) {
        std::string expected=frame.rows[row].text;expected.erase(std::remove(expected.begin(),expected.end(),'\n'),expected.end());
        expectHudRender(seen[row]&&restored[row]==expected,"all input rows and every non-newline byte survive wrapping without truncation");
    }
}
static void verifyHudVertices(const std::vector<Vertex>& triangles) {
    expectHudRender(!triangles.empty()&&triangles.size()%6==0,"every shadow and ink glyph produces complete triangle quads");
    bool ink=false,shadow=false;
    for(const auto& vertex:triangles) {
        expectHudRender(std::isfinite(vertex.position.x)&&std::isfinite(vertex.position.y)&&vertex.position.x>=-1&&vertex.position.x<=1&&vertex.position.y>=-1&&vertex.position.y<=1&&vertex.position.z==0,"HUD vertices remain in the current screen NDC plane");
        expectHudRender(vertex.texcoord[0]>=0&&vertex.texcoord[0]<=1&&vertex.texcoord[1]>=0&&vertex.texcoord[1]<=1,"glyph vertices carry valid atlas coordinates");
        if(vertex.color[0]==0&&vertex.color[1]==0&&vertex.color[2]==0)shadow=true;else ink=true;
    }
    expectHudRender(ink&&shadow,"all text has separate shadow and straight-color ink passes");
}
int main() {
    DebugHudFrame frame;frame.scale=1;frame.visible=true;
    for(unsigned row=0;row<40;++row)frame.rows.push_back({"Enemy "+std::to_string(row)+" HP 200/200",row%10?0xffeeeeeeu:0xffffd040u,row%10==0});
    auto layout=layoutDebugHud(frame,1920,1080);verifyHudLayout(frame,layout,1920,1080);
    expectHudRender(nearHudRender(layout.fontScale,1)&&layout.columns==1,"base font is exactly18px at1080p when all rows fit");
    expectHudRender(std::any_of(layout.lines.begin(),layout.lines.end(),[](const auto& line){return line.baselineY>480;}),"full-viewport layout includes rows below the old480 canvas");
    auto twice=layoutDebugHud(frame,3840,2160);verifyHudLayout(frame,twice,3840,2160);
    expectHudRender(nearHudRender(twice.fontScale,2)&&nearHudRender(twice.lines[0].baselineY,layout.lines[0].baselineY*2),"resize scales the common18px face and positions together");
    frame.scale=1.5f;auto larger=layoutDebugHud(frame,3840,2160);verifyHudLayout(frame,larger,3840,2160);
    expectHudRender(nearHudRender(larger.fontScale,3),"debug HUD user scale is applied uniformly to all sections");
    frame.scale=1;
    for(unsigned row=40;row<120;++row)frame.rows.push_back({"Enemy "+std::to_string(row),0xffeeeeeeu,row%10==0});
    layout=layoutDebugHud(frame,1920,1080);verifyHudLayout(frame,layout,1920,1080);
    expectHudRender(layout.columns>=3&&nearHudRender(layout.fontScale,1),"height overflow creates full-viewport columns before shrinking the common font");
    std::vector<Vertex> triangles;appendDebugHudGeometry(triangles,layout,1920,1080);verifyHudVertices(triangles);
    bool gold=false;
    for(const auto& vertex:triangles)if(nearHudRender(vertex.color[0],1)&&nearHudRender(vertex.color[1],208.f/255)&&nearHudRender(vertex.color[2],64.f/255))gold=true;
    expectHudRender(gold,"ARGB producer colors decode to the expected straight RGBA values");
    // A large input exercises actual wrapping, columns and fitting together.
    frame.rows.clear();
    for(unsigned row=0;row<2048;++row)frame.rows.push_back({"Enemy "+std::to_string(row)+": HP 1200/1200; equipment cooldown 5.25s",row%20?0xffeeeeeeu:0xffffd040u,row%20==0});
    const auto begin=std::chrono::steady_clock::now();layout=layoutDebugHud(frame,1920,1080);
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-begin).count();
    verifyHudLayout(frame,layout,1920,1080);
    expectHudRender(layout.fontScale<1&&layout.columns>1,"2048 rows auto-fit with a single reduced font instead of dropping data");
    triangles.clear();appendDebugHudGeometry(triangles,layout,1920,1080);verifyHudVertices(triangles);
    auto narrow=layoutDebugHud(frame,800,600);verifyHudLayout(frame,narrow,800,600);
    expectHudRender(narrow.fontScale!=layout.fontScale&&narrow.lines[0].x!=layout.lines[0].x,"new viewport extent produces a fresh fit and never reuses foreign positions");
    DebugHudFrame wrapped;wrapped.rows={{"A very long line with spaces, punctuation: 100/200; repeated words followed by a long token "+std::string(700,'W')+"\nsecond line\tUTF8 \xd0\xb0\xd0\xb1",0xffeeeeeeu,true}};
    auto small=layoutDebugHud(wrapped,320,240);verifyHudLayout(wrapped,small,320,240);
    expectHudRender(small.lines.size()>1,"long tokens, explicit newlines, tabs and UTF8 wrap without slicing codepoints");
    triangles.clear();appendDebugHudGeometry(triangles,small,320,240);verifyHudVertices(triangles);
    std::vector<Vertex> retained(1);retained[0].position={7,8,9};wrapped.visible=false;
    appendDebugHudGeometry(retained,wrapped,320,240);expectHudRender(retained.size()==1&&retained[0].position.x==7,"hidden HUD appends nothing and leaves unrelated geometry untouched");
    wrapped.visible=true;wrapped.scale=std::numeric_limits<float>::quiet_NaN();
    expectHudRender(layoutDebugHud(wrapped,320,240).lines.empty()&&layoutDebugHud(frame,0,240).lines.empty(),"invalid scale and zero extent produce no invalid screen positions");
    std::printf("PASS: full-viewport HUD rows, columns, exact wrapping,2048-row uniform fit, resize, ARGB colors, atlas/shadow geometry and visibility.2048-row layout: %lldms\n",elapsed);
}
