#pragma once
#include <cstdint>
#include <cstring>
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif
// Words already have the host-endian layout consumed by bridge Read32.
// Scan only bounded full commands, preserving the first END/no-push branch.
inline uint32_t duck_count_commands(const void *commands,uint32_t count,uint8_t end_op,uint8_t branch_op,uint8_t no_push){
 if(!commands)return 0;
 const auto *bytes=(const uint8_t*)commands;uint32_t n=0;
 const uint32_t branch=(uint32_t(branch_op)<<8)|no_push;
#if defined(__ARM_NEON)
 const uint32x4_t end=vdupq_n_u32(end_op),tail=vdupq_n_u32(branch);
 for(;count-n>=4;n+=4){
  const uint32x4_t words=vld2q_u32((const uint32_t*)(bytes+n*8)).val[0];
  const uint32x4_t matches=vorrq_u32(vceqq_u32(vshrq_n_u32(words,24),end),vceqq_u32(vshrq_n_u32(words,16),tail));
  const uint32x2_t either=vorr_u32(vget_low_u32(matches),vget_high_u32(matches));
  if(vget_lane_u32(either,0)|vget_lane_u32(either,1))break;
 }
#endif
 for(;n<count;n++){
  uint32_t word;std::memcpy(&word,bytes+n*8,4);
  if((word>>24)==end_op||(word>>16)==branch)return n+1;
 }
 return 0;
}
