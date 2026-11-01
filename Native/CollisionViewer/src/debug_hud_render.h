#pragma once
#include "eternal_ui2_font.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// DebugHudFrame and Vertex are supplied by the shared HUD/Vulkan owner.
// Layout owns only screen positions; it never imports engine text coordinates.
struct DebugHudPlacedLine {
    std::string text;
    float x=0,baselineY=0;
    unsigned color=0xffeeeeeeu;
    size_t sourceRow=0;
};
struct DebugHudLayout {
    std::vector<DebugHudPlacedLine> lines;
    float fontScale=0,lineHeight=0,columnWidth=0;
    size_t columns=0;
};
namespace debug_hud_render_detail {
struct Glyph { size_t begin=0,end=0; float width=0; bool newline=false,breakAfter=false; };
struct Row { const DebugHudLine* source=nullptr; std::vector<Glyph> glyphs; float width=0; size_t hardLines=1; };

template<class Emit> bool wrap(const Row& row,float width,Emit&& emit) {
    const auto& glyphs=row.glyphs;
    size_t begin=0,index=0,breakAt=0; float used=0;
    auto bytes=[&](size_t first,size_t end) {
        const size_t left=first<glyphs.size()?glyphs[first].begin:row.source->text.size();
        const size_t right=end>first?glyphs[end-1].end:left;
        return std::string_view(row.source->text).substr(left,right-left);
    };
    while(index<glyphs.size()) {
        const auto& glyph=glyphs[index];
        if(glyph.newline) {
            if(!emit(bytes(begin,index))) return false;
            begin=++index;breakAt=begin;used=0;continue;
        }
        if(used+glyph.width>width+.0001f&&index>begin) {
            const size_t end=breakAt>begin?breakAt:index;
            if(!emit(bytes(begin,end))) return false;
            begin=index=end;breakAt=begin;used=0;continue;
        }
        used+=glyph.width;
        if(glyph.breakAfter) breakAt=index+1;
        ++index;
    }
    return emit(bytes(begin,glyphs.size()));
}
}

static DebugHudLayout layoutDebugHud(const DebugHudFrame& frame,uint32_t width,uint32_t height) {
    DebugHudLayout empty;
    if(!frame.visible||frame.rows.empty()||!width||!height||!std::isfinite(frame.scale)||frame.scale<=0) return empty;
    using namespace debug_hud_render_detail;
    std::vector<Row> rows;rows.reserve(frame.rows.size());
    float widestGlyph=0;size_t hardLines=0,totalGlyphs=0;
    for(const auto& source:frame.rows) {
        Row row;row.source=&source;row.glyphs.reserve(source.text.size());
        float lineWidth=0;
        for(size_t index=0;index<source.text.size();) {
            const size_t begin=index;const auto codepoint=eternalUi2NextCodepoint(source.text,index);
            const bool newline=codepoint=='\n';
            const float advance=newline||codepoint=='\r'?0.f:codepoint=='\t'?4*eternalUi2Glyph(' ').advance:eternalUi2Glyph(codepoint).advance;
            row.glyphs.push_back({begin,index,advance,newline,codepoint==' '||codepoint=='\t'});
            widestGlyph=std::max(widestGlyph,advance);
            if(newline) { row.width=std::max(row.width,lineWidth);lineWidth=0;++row.hardLines; }
            else lineWidth+=advance;
        }
        row.width=std::max(row.width,lineWidth);hardLines+=row.hardLines;totalGlyphs+=std::max(size_t(1),row.glyphs.size());rows.push_back(std::move(row));
    }
    widestGlyph=std::max(widestGlyph,1.f);
    const float viewportWidth=static_cast<float>(width),viewportHeight=static_cast<float>(height);
    // At a fixed scale, try the fewest equal-width columns that fit the entire
    // frame. Cheap lower bounds reject impossible columns before wrapping.
    auto arrange=[&](float scale,DebugHudLayout* output) {
        const float margin=8*scale,gap=20*scale,lineHeight=kEternalUi2LineHeight*scale,sectionGap=4*scale;
        const float availableHeight=viewportHeight-2*margin;
        if(!std::isfinite(scale)||scale<=0||lineHeight<=0||availableHeight<lineHeight) return false;
        const double capacityValue=std::floor(double(availableHeight)/lineHeight+.00001);
        if(capacityValue<1) return false;
        const size_t capacity=static_cast<size_t>(std::min(capacityValue,double(std::numeric_limits<size_t>::max()/2)));
        const size_t minimumColumns=std::max(size_t(1),hardLines/capacity+(hardLines%capacity!=0));
        const double maximumValue=std::floor((double(viewportWidth)-2*margin+gap)/(widestGlyph*scale+gap));
        if(maximumValue<1||double(minimumColumns)>maximumValue) return false;
        const size_t maximumColumns=static_cast<size_t>(std::min(maximumValue,double(totalGlyphs)));
        for(size_t columns=minimumColumns;columns<=maximumColumns;++columns) {
            const float columnWidth=(viewportWidth-2*margin-static_cast<float>(columns-1)*gap)/static_cast<float>(columns);
            if(columnWidth<widestGlyph*scale-.0001f) continue;
            size_t lower=0;
            for(const auto& row:rows) {
                lower+=std::max(row.hardLines,static_cast<size_t>(std::ceil(double(row.width)*scale/columnWidth-.00001)));
                if(double(lower)>double(columns)*capacity) break;
            }
            if(double(lower)>double(columns)*capacity) continue;
            if(output) { output->lines.clear();output->fontScale=scale;output->lineHeight=lineHeight;output->columnWidth=columnWidth;output->columns=columns; }
            size_t column=0;float y=margin;bool complete=true;
            for(size_t rowIndex=0;rowIndex<rows.size()&&complete;++rowIndex) {
                bool first=true;
                complete=wrap(rows[rowIndex],columnWidth/scale,[&](std::string_view text) {
                    float separation=first&&rows[rowIndex].source->sectionStart&&y>margin+.0001f?sectionGap:0;
                    if(y+separation+lineHeight>viewportHeight-margin+.0001f) { ++column;y=margin;separation=0; }
                    if(column>=columns) return false;
                    y+=separation;
                    if(output) output->lines.push_back({std::string(text),margin+static_cast<float>(column)*(columnWidth+gap),y+kEternalUi2Ascent*scale,rows[rowIndex].source->color,rowIndex});
                    y+=lineHeight;first=false;return true;
                });
            }
            if(complete) return true;
        }
        if(output) *output={};
        return false;
    };
    const float desired=frame.scale*(viewportHeight/1080.f);
    if(!std::isfinite(desired)||desired<=0) return empty;
    float scale=desired;
    if(!arrange(scale,nullptr)) {
        float upper=scale;
        do {
            scale*=.5f;
            if(scale<std::numeric_limits<float>::min()*1024) return empty;
        } while(!arrange(scale,nullptr));
        for(unsigned step=0;step<12;++step) {
            const float middle=(scale+upper)*.5f;
            if(arrange(middle,nullptr)) scale=middle;else upper=middle;
        }
    }
    DebugHudLayout result;arrange(scale,&result);return result;
}

static void appendDebugHudGeometry(std::vector<Vertex>& triangles,const DebugHudLayout& layout,uint32_t width,uint32_t height) {
    if(!width||!height||layout.lines.empty()||!std::isfinite(layout.fontScale)||layout.fontScale<=0) return;
    const float viewportWidth=static_cast<float>(width),viewportHeight=static_cast<float>(height);
    auto quad=[&](float left,float top,float right,float bottom,float u0,float v0,float u1,float v1,const std::array<float,4>& color) {
        if(!std::isfinite(left)||!std::isfinite(top)||!std::isfinite(right)||!std::isfinite(bottom)||right<=left||bottom<=top) return;
        const float clippedLeft=std::max(0.f,left),clippedTop=std::max(0.f,top),clippedRight=std::min(viewportWidth,right),clippedBottom=std::min(viewportHeight,bottom);
        if(clippedRight<=clippedLeft||clippedBottom<=clippedTop) return;
        const float us[]{u0+(u1-u0)*(clippedLeft-left)/(right-left),u0+(u1-u0)*(clippedRight-left)/(right-left)};
        const float vs[]{v0+(v1-v0)*(clippedTop-top)/(bottom-top),v0+(v1-v0)*(clippedBottom-top)/(bottom-top)};
        const float xs[]{2*clippedLeft/viewportWidth-1,2*clippedRight/viewportWidth-1};
        const float ys[]{1-2*clippedTop/viewportHeight,1-2*clippedBottom/viewportHeight};
        constexpr unsigned corners[][2]{{0,0},{1,0},{1,1},{0,0},{1,1},{0,1}};
        for(const auto& corner:corners) {
            Vertex vertex{};vertex.position={xs[corner[0]],ys[corner[1]],0};
            for(unsigned channel=0;channel<4;++channel) vertex.color[channel]=color[channel];
            vertex.texcoord[0]=us[corner[0]];vertex.texcoord[1]=vs[corner[1]];triangles.push_back(vertex);
        }
    };
    // Draw all shadows before all ink so the final text remains crisp where
    // columns or wrapped lines meet. Both passes use exactly one shared scale.
    for(bool shadow:{true,false}) for(const auto& line:layout.lines) {
        std::array<float,4> color{float((line.color>>16)&255)/255,float((line.color>>8)&255)/255,float(line.color&255)/255,float((line.color>>24)&255)/255};
        if(shadow) color={0,0,0,color[3]*.85f};
        const float offset=shadow?layout.fontScale:0;
        appendEternalUi2GlyphQuads(line.text,line.x+offset,line.baselineY+offset,layout.fontScale,color,quad);
    }
}
static void appendDebugHudGeometry(std::vector<Vertex>& triangles,const DebugHudFrame& frame,uint32_t width,uint32_t height) {
    appendDebugHudGeometry(triangles,layoutDebugHud(frame,width,height),width,height);
}
