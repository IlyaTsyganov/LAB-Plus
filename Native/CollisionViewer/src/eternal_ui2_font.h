#pragma once
#include "eternal_ui2_font_data.h"
#include <cmath>
#include <string_view>

inline const EternalUi2Glyph& eternalUi2Glyph(uint32_t codepoint) {
    if(codepoint<32||codepoint>126) codepoint='?';
    return kEternalUi2Glyphs[codepoint-32];
}
// The baked face covers printable ASCII. Consume one complete UTF-8 sequence
// for unsupported characters, so a missing glyph is one '?' rather than bytes.
inline uint32_t eternalUi2NextCodepoint(std::string_view text,size_t& index) {
    const auto first=static_cast<unsigned char>(text[index++]);
    if(first<128) return first;
    unsigned following=first>=0xc2&&first<=0xdf?1:first>=0xe0&&first<=0xef?2:first>=0xf0&&first<=0xf4?3:0;
    while(following&&index<text.size()&&(static_cast<unsigned char>(text[index])&0xc0)==0x80) { ++index;--following; }
    return '?';
}
inline float eternalUi2TextWidth(std::string_view text,float scale=1) {
    if(!std::isfinite(scale)||scale<=0) return 0;
    float width=0,maximum=0;
    for(size_t index=0;index<text.size();) {
        const auto codepoint=eternalUi2NextCodepoint(text,index);
        if(codepoint=='\n') { maximum=std::max(maximum,width);width=0;continue; }
        if(codepoint=='\r') continue;
        width+=(codepoint=='\t'?4*eternalUi2Glyph(' ').advance:eternalUi2Glyph(codepoint).advance)*scale;
    }
    return std::max(maximum,width);
}
// Screen coordinates: x advances right, baselineY advances down. The callback
// receives one glyph quad (left,top,right,bottom,u0,v0,u1,v1,straight RGBA).
// Coverage belongs to the R8 atlas and must be multiplied into alpha once by
// the fragment shader; color is deliberately not premultiplied here.
template<class Quad> inline void appendEternalUi2GlyphQuads(std::string_view text,float x,float baselineY,float scale,
    const std::array<float,4>& color,Quad&& quad) {
    if(!std::isfinite(x)||!std::isfinite(baselineY)||!std::isfinite(scale)||scale<=0) return;
    const float origin=x;
    constexpr float inverseWidth=1.f/kEternalUi2AtlasWidth,inverseHeight=1.f/kEternalUi2AtlasHeight;
    for(size_t index=0;index<text.size();) {
        const auto codepoint=eternalUi2NextCodepoint(text,index);
        if(codepoint=='\n') { x=origin;baselineY+=kEternalUi2LineHeight*scale;continue; }
        if(codepoint=='\r') continue;
        if(codepoint=='\t') { x+=4*eternalUi2Glyph(' ').advance*scale;continue; }
        const auto& glyph=eternalUi2Glyph(codepoint);
        if(glyph.width&&glyph.height) {
            const float left=x+glyph.left*scale,top=baselineY+glyph.top*scale;
            quad(left,top,left+glyph.width*scale,top+glyph.height*scale,
                glyph.x*inverseWidth,glyph.y*inverseHeight,(glyph.x+glyph.width)*inverseWidth,(glyph.y+glyph.height)*inverseHeight,color);
        }
        x+=glyph.advance*scale;
    }
}
