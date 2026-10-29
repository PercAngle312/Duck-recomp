#pragma once
#include <cstdint>
#include <cstring>
#include "duck_vita_rsp_memory.h"
#if defined(__ARM_NEON)
// Preserve the original command's final iteration and register/accumulator
// state. Prefix kernels own no persistent state and retain preload-before-store.
inline bool duck_audio_clear_prefix(uint8_t *d,uint32_t &address,uint32_t &remaining){
 const uint32_t bytes=((remaining?remaining:1)+15u)&~15u;
 if(bytes<32||(address&3u)||address>4096||bytes>4096-address)return false;
 std::memset(d+address,0,bytes-16);address+=bytes-16;remaining-=bytes-16;return true;
}
inline bool duck_audio_move_prefix(uint8_t *d,uint32_t &source,uint32_t &dest,uint32_t &remaining){
 const uint32_t bytes=((remaining?remaining:1)+15u)&~15u;
 if(bytes<32||((source|dest)&3u)||source>4096||dest>4096||bytes>4096-source||bytes>4096-dest)return false;
 for(uint32_t n=0;n<bytes-16;n+=16){const uint8x16_t block=vld1q_u8(d+source+n);vst1q_u8(d+dest+n,block);}
 source+=bytes-16;dest+=bytes-16;remaining-=bytes-16;return true;
}
inline void duck_audio_interleave_prefix(uint8_t *d){
 for(unsigned n=0;n<352;n+=16){
  const uint16x8_t left=vrev32q_u16(vld1q_u16((const uint16_t*)(d+0x9d0+n)));
  const uint16x8_t right=vrev32q_u16(vld1q_u16((const uint16_t*)(d+0xb40+n)));
  const uint16x8x2_t stereo={{right,left}};vst2q_u16((uint16_t*)(d+0x4f0+n*2),stereo);
 }
}
inline int16x4_t duck_audio_mix_four(int16x4_t output,int16x4_t input,int16_t factor,int16_t gain){
 const int32x4_t a=vmull_n_s16(output,factor),b=vmull_n_s16(input,gain);
 const int64x2_t low=vaddq_s64(vshlq_n_s64(vaddl_s32(vget_low_s32(a),vget_low_s32(b)),1),vdupq_n_s64(32768));
 const int64x2_t high=vaddq_s64(vshlq_n_s64(vaddl_s32(vget_high_s32(a),vget_high_s32(b)),1),vdupq_n_s64(32768));
 return vqmovn_s32(vcombine_s32(vqmovn_s64(vshrq_n_s64(low,16)),vqmovn_s64(vshrq_n_s64(high,16))));
}
inline bool duck_audio_mix_prefix(uint8_t *d,uint32_t &output,uint32_t &input,uint32_t &remaining,int16_t factor,int16_t gain,void *output_register,void *input_register){
 if(((output|input)&15u)||output>4096-384||input>4096-384||remaining!=368)return false;
 int16x8_t a=vld1q_s16((const int16_t*)(d+output)),b=vld1q_s16((const int16_t*)(d+input));
 for(unsigned n=0;n<352;n+=16){
  const int16x8_t value=vcombine_s16(duck_audio_mix_four(vget_low_s16(a),vget_low_s16(b),factor,gain),duck_audio_mix_four(vget_high_s16(a),vget_high_s16(b),factor,gain));
  // The microcode reads the following block before writing the current one.
  a=vld1q_s16((const int16_t*)(d+output+n+16));b=vld1q_s16((const int16_t*)(d+input+n+16));
  vst1q_s16((int16_t*)(d+output+n),value);
 }
 alignas(16) int16_t next_a[8],next_b[8];vst1q_s16(next_a,a);vst1q_s16(next_b,b);
 duck_rsp_reverse_words16(output_register,next_a);duck_rsp_reverse_words16(input_register,next_b);
 output+=352;input+=352;remaining=16;return true;
}
#endif
