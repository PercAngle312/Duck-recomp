#pragma once
#if defined(__ARM_NEON)
#include <arm_neon.h>
#include <cstdint>
// Mixed signed/unsigned 16-bit products, keeping the full modulo-48-bit RSP
// accumulator. Inputs are loaded before any write, including aliased vd.
template<bool Accumulate,bool SignedSource>
inline void duck_rsp_multiply_mixed(void *destination,const void *source,const void *target,
                                   void *low,void *middle,void *high){
    const uint16x8_t a=vld1q_u16((const uint16_t*)source),b=vld1q_u16((const uint16_t*)target);
    const uint16x8_t oldl=vld1q_u16((const uint16_t*)low),oldm=vld1q_u16((const uint16_t*)middle),oldh=vld1q_u16((const uint16_t*)high);
    auto half=[](uint16x4_t av,uint16x4_t bv,uint16x4_t al,uint16x4_t am,uint16x4_t ah,
                 uint16x4_t &l,uint16x4_t &m,uint16x4_t &h){
        uint32x4_t p=vmull_u16(av,bv);
        const uint16x4_t signedValue=SignedSource?av:bv,unsignedValue=SignedSource?bv:av;
        const uint16x4_t sign=vreinterpret_u16_s16(vshr_n_s16(vreinterpret_s16_u16(signedValue),15));
        p=vsubq_u32(p,vshlq_n_u32(vmovl_u16(vand_u16(sign,unsignedValue)),16));
        const uint32x4_t pl=vandq_u32(p,vdupq_n_u32(0xffff)),pm=vshrq_n_u32(p,16);
        const uint16x4_t ph=vmovn_u32(vreinterpretq_u32_s32(vshrq_n_s32(vreinterpretq_s32_u32(p),31)));
        if constexpr (Accumulate){
            const uint32x4_t ls=vaddq_u32(vmovl_u16(al),pl);
            const uint32x4_t ms=vaddq_u32(vaddq_u32(vmovl_u16(am),pm),vshrq_n_u32(ls,16));
            l=vmovn_u32(ls);m=vmovn_u32(ms);h=vadd_u16(vadd_u16(ah,ph),vmovn_u32(vshrq_n_u32(ms,16)));
        }else{l=vmovn_u32(pl);m=vmovn_u32(pm);h=ph;}
    };
    uint16x4_t ll,lm,lh,hl,hm,hh;
    half(vget_low_u16(a),vget_low_u16(b),vget_low_u16(oldl),vget_low_u16(oldm),vget_low_u16(oldh),ll,lm,lh);
    half(vget_high_u16(a),vget_high_u16(b),vget_high_u16(oldl),vget_high_u16(oldm),vget_high_u16(oldh),hl,hm,hh);
    const uint16x8_t l=vcombine_u16(ll,hl),m=vcombine_u16(lm,hm),h=vcombine_u16(lh,hh);
    uint16x8_t result;
    if constexpr (!Accumulate){result=SignedSource?m:l;}
    else if constexpr (SignedSource){
        const int32x4_t x=vreinterpretq_s32_u32(vorrq_u32(vmovl_u16(lm),vshlq_n_u32(vmovl_u16(lh),16)));
        const int32x4_t y=vreinterpretq_s32_u32(vorrq_u32(vmovl_u16(hm),vshlq_n_u32(vmovl_u16(hh),16)));
        result=vreinterpretq_u16_s16(vcombine_s16(vqmovn_s32(x),vqmovn_s32(y)));
    }else{
        const uint16x8_t negative=vcltq_s16(vreinterpretq_s16_u16(h),vdupq_n_s16(0));
        const uint16x8_t sign=vreinterpretq_u16_s16(vshrq_n_s16(vreinterpretq_s16_u16(m),15));
        const uint16x8_t fits=vceqq_u16(h,sign);
        result=vbslq_u16(fits,l,vmvnq_u16(negative));
    }
    vst1q_u16((uint16_t*)low,l);vst1q_u16((uint16_t*)middle,m);vst1q_u16((uint16_t*)high,h);vst1q_u16((uint16_t*)destination,result);
}
inline void duck_rsp_multiply_low(void *destination,const void *source,const void *target,
                                  void *low,void *middle,void *high){
    const uint16x8_t a=vld1q_u16((const uint16_t*)source),b=vld1q_u16((const uint16_t*)target);
    const uint16x8_t l=vcombine_u16(vshrn_n_u32(vmull_u16(vget_low_u16(a),vget_low_u16(b)),16),vshrn_n_u32(vmull_u16(vget_high_u16(a),vget_high_u16(b)),16));
    vst1q_u16((uint16_t*)low,l);vst1q_u16((uint16_t*)middle,vdupq_n_u16(0));vst1q_u16((uint16_t*)high,vdupq_n_u16(0));vst1q_u16((uint16_t*)destination,l);
}
#endif
