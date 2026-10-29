#pragma once
#include <cstdint>
#if defined(__ARM_NEON)
#include <arm_neon.h>
// LQV<0>/LRV<0>: gather within one complete DMEM block, preserving every
// register byte outside the selected span. DMEM is word-swapped; registers
// are reversed across all sixteen logical bytes. The block cannot wrap DMEM.
template<bool Right>
inline void duck_rsp_load_partial(void *destination,const uint8_t *dmem,uint32_t address){
 const unsigned offset=address&15u;
 if constexpr (Right) {if(!offset)return;}
 const uint32x4_t words=vrev64q_u32(vld1q_u32((const uint32_t*)(dmem+(address&0xff0u))));
 const uint8x16_t block=vreinterpretq_u8_u32(vcombine_u32(vget_high_u32(words),vget_low_u32(words)));
 if constexpr (!Right) {if(!offset){vst1q_u8((uint8_t*)destination,block);return;}}
 const uint8x8x2_t table={{vget_low_u8(block),vget_high_u8(block)}};
 alignas(16) static constexpr uint8_t order[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
 const uint8x16_t lanes=vld1q_u8(order),shift=vdupq_n_u8((uint8_t)offset);
 const uint8x16_t indices=Right?vaddq_u8(lanes,vdupq_n_u8((uint8_t)(16-offset))):vsubq_u8(lanes,shift);
 const uint8x16_t selected=vcombine_u8(vtbl2_u8(table,vget_low_u8(indices)),vtbl2_u8(table,vget_high_u8(indices)));
 const uint8x16_t mask=Right?vcltq_u8(lanes,shift):vcgeq_u8(lanes,shift);
 vst1q_u8((uint8_t*)destination,vbslq_u8(mask,selected,vld1q_u8((const uint8_t*)destination)));
}
#endif
