#pragma once
#if defined(__ARM_NEON)
#include <arm_neon.h>
#include <cstdint>
#include "duck_vita_rsp_accumulator.h"

// VMUDH/VMADH add a signed 16x16 product at accumulator bit 16. The low
// accumulator slice is zeroed by VMUDH and untouched by VMADH. Widened NEON
// multiply-accumulate handles the middle/high carry as one modulo-32-bit sum.
template<bool Accumulate>
inline void duck_rsp_multiply_high(void *destination,const void *source,const void *target,
                                  void *low,void *middle,void *high){
    const int16x8_t a=vld1q_s16(static_cast<const int16_t*>(source));
    const int16x8_t b=vld1q_s16(static_cast<const int16_t*>(target));
    int32x4_t x,y;
    if constexpr (Accumulate){
        const uint16x8_t m=vld1q_u16(static_cast<const uint16_t*>(middle));
        const uint16x8_t h=vld1q_u16(static_cast<const uint16_t*>(high));
        const uint32x4_t oldx=vorrq_u32(vmovl_u16(vget_low_u16(m)),vshlq_n_u32(vmovl_u16(vget_low_u16(h)),16));
        const uint32x4_t oldy=vorrq_u32(vmovl_u16(vget_high_u16(m)),vshlq_n_u32(vmovl_u16(vget_high_u16(h)),16));
        x=vmlal_s16(vreinterpretq_s32_u32(oldx),vget_low_s16(a),vget_low_s16(b));
        y=vmlal_s16(vreinterpretq_s32_u32(oldy),vget_high_s16(a),vget_high_s16(b));
    }else{
        x=vmull_s16(vget_low_s16(a),vget_low_s16(b));
        y=vmull_s16(vget_high_s16(a),vget_high_s16(b));
        vst1q_u16(static_cast<uint16_t*>(low),vdupq_n_u16(0));
    }
    vst1q_s16(static_cast<int16_t*>(middle),vcombine_s16(vmovn_s32(x),vmovn_s32(y)));
    vst1q_s16(static_cast<int16_t*>(high),vcombine_s16(vshrn_n_s32(x,16),vshrn_n_s32(y,16)));
    vst1q_s16(static_cast<int16_t*>(destination),vcombine_s16(vqmovn_s32(x),vqmovn_s32(y)));
}

// Operate on the RSP's physical lane order. The caller already applied the
// instruction's element shuffle. Preserve all 48 accumulator bits, including
// overflow: saturating arithmetic belongs only on the destination register.
template<bool Accumulate, bool Unsigned>
__attribute__((always_inline)) inline void duck_rsp_multiply(void *destination,const void *source,const void *target,
                              void *low,void *middle,void *high){
    const int16x8_t a=vld1q_s16(static_cast<const int16_t*>(source));
    const int16x8_t b=vld1q_s16(static_cast<const int16_t*>(target));
    uint16x8_t l,m,h;
    if constexpr (!Accumulate){
        const int32x4_t p=vmull_s16(vget_low_s16(a),vget_low_s16(b));
        const int32x4_t q=vmull_s16(vget_high_s16(a),vget_high_s16(b));
        // (product * 2 + 0x8000) >> 16, without signed 32-bit overflow.
        const int32x4_t pm=vshrq_n_s32(vaddq_s32(p,vdupq_n_s32(0x4000)),15);
        const int32x4_t qm=vshrq_n_s32(vaddq_s32(q,vdupq_n_s32(0x4000)),15);
        l=vaddq_u16(vshlq_n_u16(vcombine_u16(vmovn_u32(vreinterpretq_u32_s32(p)),vmovn_u32(vreinterpretq_u32_s32(q))),1),vdupq_n_u16(0x8000));
        m=vreinterpretq_u16_s16(vcombine_s16(vmovn_s32(pm),vmovn_s32(qm)));
        h=vreinterpretq_u16_s16(vcombine_s16(vshrn_n_s32(pm,16),vshrn_n_s32(qm,16)));
    }else{
        const uint16x8_t oldl=vld1q_u16(static_cast<const uint16_t*>(low));
        const uint16x8_t oldm=vld1q_u16(static_cast<const uint16_t*>(middle));
        const uint16x8_t oldh=vld1q_u16(static_cast<const uint16_t*>(high));
        const int32x4_t p=vmull_s16(vget_low_s16(a),vget_low_s16(b));
        const int32x4_t q=vmull_s16(vget_high_s16(a),vget_high_s16(b));
        const uint16x8_t pl=vcombine_u16(vmovn_u32(vreinterpretq_u32_s32(p)),vmovn_u32(vreinterpretq_u32_s32(q)));
        const uint16x8_t pm=vreinterpretq_u16_s16(vcombine_s16(vshrn_n_s32(p,16),vshrn_n_s32(q,16)));
        const uint16x8_t product_low=vshlq_n_u16(pl,1);
        const uint16x8_t product_middle=vorrq_u16(vshlq_n_u16(pm,1),vshrq_n_u16(pl,15));
        const uint16x8_t product_high=vreinterpretq_u16_s16(vshrq_n_s16(vreinterpretq_s16_u16(pm),15));
        l=oldl;m=oldm;h=oldh;
        duck_rsp_accumulate48(l,m,h,product_low,product_middle,product_high);
    }
    uint16x8_t result;
    if constexpr (Unsigned){
        const uint16x8_t negative=vcltq_s16(vreinterpretq_s16_u16(h),vdupq_n_s16(0));
        const uint16x8_t overflow=vorrq_u16(vmvnq_u16(vceqq_u16(h,vdupq_n_u16(0))),vcltq_s16(vreinterpretq_s16_u16(m),vdupq_n_s16(0)));
        result=vbicq_u16(vorrq_u16(m,overflow),negative);
    }else{
        result=duck_rsp_saturate_middle(m,h);
    }
    vst1q_u16(static_cast<uint16_t*>(low),l);
    vst1q_u16(static_cast<uint16_t*>(middle),m);
    vst1q_u16(static_cast<uint16_t*>(high),h);
    vst1q_u16(static_cast<uint16_t*>(destination),result);
}
#endif
