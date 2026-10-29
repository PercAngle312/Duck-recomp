#pragma once
#include <cstdint>
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif
// Compile-time RSP element selection. Avoid the generic dynamic shuffle table,
// its initialization guard and out-of-line shuffle call for constant selectors.
template<unsigned E,class Vector>
__attribute__((always_inline)) inline Vector duck_rsp_element(const Vector &source){
 static_assert(E<16);
#if defined(__ARM_NEON)
 if constexpr (E<=1)return source;
 else{
  const uint16x8_t value=vld1q_u16((const uint16_t*)&source.u128);uint16x8_t selected;
  if constexpr (E==2)selected=vtrnq_u16(value,value).val[1];
  else if constexpr (E==3)selected=vtrnq_u16(value,value).val[0];
  else if constexpr (E<8)selected=vcombine_u16(vdup_lane_u16(vget_low_u16(value),7-E),vdup_lane_u16(vget_high_u16(value),7-E));
  else if constexpr (15-E<4)selected=vdupq_lane_u16(vget_low_u16(value),15-E);
  else selected=vdupq_lane_u16(vget_high_u16(value),11-E);
  Vector out;vst1q_u16((uint16_t*)&out.u128,selected);return out;
 }
#else
 return source(E);
#endif
}
