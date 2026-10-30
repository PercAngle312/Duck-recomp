#pragma once
#if defined(__ARM_NEON)
#include <arm_neon.h>
#include <cstdint>
#include "duck_vita_rsp_accumulator.h"
// Mixed signed/unsigned 16-bit products, keeping the full modulo-48-bit RSP
// accumulator. Inputs are loaded before any write, including aliased vd.
template<bool Accumulate,bool SignedSource>
__attribute__((always_inline)) inline void duck_rsp_multiply_mixed(void *destination,const void *source,const void *target,
                                   void *low,void *middle,void *high){
    const uint16x8_t a=vld1q_u16((const uint16_t*)source),b=vld1q_u16((const uint16_t*)target);
    const uint16x8_t oldl=vld1q_u16((const uint16_t*)low),oldm=vld1q_u16((const uint16_t*)middle),oldh=vld1q_u16((const uint16_t*)high);
    const uint32x4_t p=vmull_u16(vget_low_u16(a),vget_low_u16(b));
    const uint32x4_t q=vmull_u16(vget_high_u16(a),vget_high_u16(b));
    uint16x8_t l=vcombine_u16(vmovn_u32(p),vmovn_u32(q));
    uint16x8_t m=vcombine_u16(vshrn_n_u32(p,16),vshrn_n_u32(q,16));
    const uint16x8_t signed_value=SignedSource?a:b,unsigned_value=SignedSource?b:a;
    const uint16x8_t sign=vreinterpretq_u16_s16(vshrq_n_s16(vreinterpretq_s16_u16(signed_value),15));
    m=vsubq_u16(m,vandq_u16(sign,unsigned_value));
    uint16x8_t h=vreinterpretq_u16_s16(vshrq_n_s16(vreinterpretq_s16_u16(m),15));
    if constexpr (Accumulate){
        const uint16x8_t pl=l,pm=m,ph=h;
        l=oldl;m=oldm;h=oldh;
        duck_rsp_accumulate48(l,m,h,pl,pm,ph);
    }
    uint16x8_t result;
    if constexpr (!Accumulate){result=SignedSource?m:l;}
    else if constexpr (SignedSource){
        result=duck_rsp_saturate_middle(m,h);
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
