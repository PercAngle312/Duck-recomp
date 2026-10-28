#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

inline uint16_t duck_abgr1555_pixel(uint16_t x){
    return uint16_t(((x&31)<<11)|(((x>>5)&31)<<6)|(((x>>10)&31)<<1)|(x>>15));
}
inline void duck_abgr1555_to_rgba5551(uint16_t *p,size_t count){
    size_t i=0;
#if defined(__ARM_NEON)
    const uint16x8_t mask=vdupq_n_u16(31);
    for(;i+8<=count;i+=8){
        uint16x8_t x=vld1q_u16(p+i);
        uint16x8_t r=vshlq_n_u16(vandq_u16(x,mask),11);
        uint16x8_t g=vshlq_n_u16(vandq_u16(vshrq_n_u16(x,5),mask),6);
        uint16x8_t b=vshlq_n_u16(vandq_u16(vshrq_n_u16(x,10),mask),1);
        vst1q_u16(p+i,vorrq_u16(vorrq_u16(r,g),vorrq_u16(b,vshrq_n_u16(x,15))));
    }
#endif
    for(;i<count;i++)p[i]=duck_abgr1555_pixel(p[i]);
}

// Recompiler RDRAM stores logical halfwords at (byte_address ^ 2). Use
// contiguous vector/pair stores for aligned spans; preserve odd tails and
// halfword-aligned starts instead of touching neighbouring guest pixels.
inline void duck_store_rgba5551(uint8_t *ram,size_t offset,const uint16_t *src,size_t count){
    size_t i=0;
    if((offset&3)==2 && count){std::memcpy(ram+(offset^2),src,2);i=1;offset+=2;}
#if defined(__ARM_NEON)
    for(;i+8<=count;i+=8,offset+=16)
        vst1q_u16(reinterpret_cast<uint16_t*>(ram+offset),vrev32q_u16(vld1q_u16(src+i)));
#endif
    for(;i+2<=count;i+=2,offset+=4){
        uint32_t pair=uint32_t(src[i])<<16|src[i+1];std::memcpy(ram+offset,&pair,4);
    }
    if(i<count)std::memcpy(ram+(offset^2),src+i,2);
}
