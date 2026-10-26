#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
// N64 light-colour MOVEWORD values are logical 0xRRGGBBxx words, independent
// of host byte order. F3DEX2 slots stride 24 bytes; original F3D slots stride 32.
template<class Light, size_t N>
inline bool duck_update_light_color(Light (&lights)[N], unsigned offset, uint32_t word, unsigned stride) {
    if (stride!=24 && stride!=32) return false;
    const unsigned index=offset/stride, field=offset%stride;
    if (index>=N || (field!=0 && field!=4)) return false;
    auto *rgb=field==0?lights[index].l.col:lights[index].l.colc;
    rgb[0]=(uint8_t)(word>>24);rgb[1]=(uint8_t)(word>>16);rgb[2]=(uint8_t)(word>>8);
    return true;
}
template<class Color>
inline void duck_update_vertex_color(Color &color, uint32_t word) {
    color.r=(uint8_t)(word>>24);color.g=(uint8_t)(word>>16);
    color.b=(uint8_t)(word>>8);color.a=(uint8_t)word;
}
// Explicit N64 RGBA5551 packing. Pinned vitaGL's GPU readback maps both
// GL_UNSIGNED_SHORT_5_5_5_1 and its reversed variant to ABGR1555 storage.
inline uint16_t duck_rgba8_to_rgba5551(const uint8_t *rgba) {
    return (uint16_t)(((rgba[0]>>3)<<11)|((rgba[1]>>3)<<6)|((rgba[2]>>3)<<1)|(rgba[3]>>7));
}
// N64 TLUT can store either RGBA5551 or intensity/alpha pairs.
inline void duck_decode_tlut(uint16_t word, bool ia16, uint8_t *out) {
    if(ia16){out[0]=out[1]=out[2]=(uint8_t)(word>>8);out[3]=(uint8_t)word;}
    else {out[0]=(uint8_t)(((word>>11)&31)*255/31);out[1]=(uint8_t)(((word>>6)&31)*255/31);
          out[2]=(uint8_t)(((word>>1)&31)*255/31);out[3]=(word&1)?255:0;}
}
template<class Key>
inline bool duck_texture_keys_equal(const Key &a,const Key &b){
    for(size_t i=0;i<8;i++)if(a.decode_view[i]!=b.decode_view[i])return false;
    return a.texture_addr==b.texture_addr&&a.palette_addrs[0]==b.palette_addrs[0]&&
        a.palette_addrs[1]==b.palette_addrs[1]&&a.fmt==b.fmt&&a.siz==b.siz&&
        a.palette_index==b.palette_index&&a.size_bytes==b.size_bytes&&a.palette_format==b.palette_format;
}

// Crop destination rows without changing the pitch of the staged source image.
inline void duck_decode_rgba16_rect(const uint8_t *src,unsigned width,unsigned height,unsigned pitch,uint8_t *dst){
    for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++){
        const uint8_t *p=src+(size_t)y*pitch+x*2;
        uint16_t v=((uint16_t)p[0]<<8)|p[1];
        unsigned r=v>>11,g=(v>>6)&31,b=(v>>1)&31;
        uint8_t *d=dst+((size_t)y*width+x)*4;
        d[0]=(uint8_t)((r<<3)|(r>>2));d[1]=(uint8_t)((g<<3)|(g>>2));
        d[2]=(uint8_t)((b<<3)|(b>>2));d[3]=(v&1)?255:0;
    }
}
inline void duck_copy_rgba32_rect(const uint8_t *src,unsigned width,unsigned height,unsigned pitch,uint8_t *dst){
    for(unsigned y=0;y<height;y++)std::memcpy(dst+(size_t)y*width*4,src+(size_t)y*pitch,(size_t)width*4);
}
