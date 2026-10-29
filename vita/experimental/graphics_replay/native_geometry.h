#pragma once
#include <cstdint>
#include <cstring>
#include <fenv.h>
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif
// Bound normal values so multiply/add intermediates cannot underflow/overflow.
// Exceptional matrices and non-nearest rounding retain the scalar path.
inline bool duck_native_position_matrix(const float m[4][4]){
 if(fegetround()!=FE_TONEAREST)return false;
 for(unsigned i=0;i<16;i++){uint32_t bits;std::memcpy(&bits,&m[i/4][i%4],4);bits&=0x7fffffffu;if(bits&&((bits>>23)<67||(bits>>23)>187))return false;}
 return true;
}
inline void duck_position_transform(float out[4],const float m[4][4],float x,float y,float z){
#if defined(__ARM_NEON)
 // Separate multiply and add operations preserve the original association.
 float32x4_t v=vmulq_n_f32(vld1q_f32(m[0]),x);
 v=vaddq_f32(v,vmulq_n_f32(vld1q_f32(m[1]),y));
 v=vaddq_f32(v,vmulq_n_f32(vld1q_f32(m[2]),z));
 vst1q_f32(out,vaddq_f32(v,vld1q_f32(m[3])));
#else
 for(unsigned j=0;j<4;j++)out[j]=((x*m[0][j]+y*m[1][j])+z*m[2][j])+m[3][j];
#endif
}
inline void duck_fixed_matrix(float out[16],const void *source){
#if defined(__ARM_NEON)
 const auto *words=(const uint32_t*)source;
 for(unsigned i=0;i<8;i+=4){
  const uint32x4_t ip=vld1q_u32(words+i),fp=vld1q_u32(words+i+8);
  const int32x4_t even=vreinterpretq_s32_u32(vorrq_u32(vandq_u32(ip,vdupq_n_u32(0xffff0000u)),vshrq_n_u32(fp,16)));
  const int32x4_t odd=vreinterpretq_s32_u32(vorrq_u32(vshlq_n_u32(ip,16),vandq_u32(fp,vdupq_n_u32(0xffff))));
  const float32x4x2_t result={{vmulq_n_f32(vcvtq_f32_s32(even),1.0f/65536.0f),vmulq_n_f32(vcvtq_f32_s32(odd),1.0f/65536.0f)}};
  vst2q_f32(out+i*2,result);
 }
#else
 uint32_t words[16];std::memcpy(words,source,64);
 for(unsigned i=0;i<8;i++){out[i*2]=(float)(int32_t)((words[i]&0xffff0000u)|(words[i+8]>>16))/65536.0f;out[i*2+1]=(float)(int32_t)((words[i]<<16)|(words[i+8]&0xffffu))/65536.0f;}
#endif
}
