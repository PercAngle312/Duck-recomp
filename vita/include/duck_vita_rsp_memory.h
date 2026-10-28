#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

/* RDRAM and DMEM both store each logical word byte-reversed. RSP vector
 * registers store logical bytes in reverse order across the whole register.
 * Aligned transfers therefore exchange WORDS, not individual byte order. */
static inline void duck_rsp_reverse_words16(void *destination,const void *source){
#if defined(__ARM_NEON)
 uint32x4_t v=vrev64q_u32(vld1q_u32((const uint32_t*)source));
 vst1q_u32((uint32_t*)destination,vcombine_u32(vget_high_u32(v),vget_low_u32(v)));
#else
 uint32_t words[4],reversed[4];memcpy(words,source,16);
 for(unsigned i=0;i<4;i++)reversed[i]=words[3-i];
 memcpy(destination,reversed,16);
#endif
}
static inline void duck_rsp_reverse_words8(void *destination,const void *source){
 uint32_t words[2],reversed[2];memcpy(words,source,8);
 reversed[0]=words[1];reversed[1]=words[0];memcpy(destination,reversed,8);
}
static inline int duck_rsp_dma_read(uint8_t *dmem,const uint8_t *ram,
                                  uint32_t destination,uint32_t source,uint32_t bytes){
 if(((destination|source)&3)||destination>4096||bytes>4096-destination||
    source>0x800000||bytes>0x800000-source)return 0;
 uint32_t words=bytes&~3u;memcpy(dmem+destination,ram+source,words);
 for(uint32_t i=words;i<bytes;i++)dmem[(destination+i)^3u]=ram[(source+i)^3u];
 return 1;
}
static inline int duck_rsp_dma_write(uint8_t *ram,const uint8_t *dmem,
                                   uint32_t destination,uint32_t source,uint32_t bytes){
 if(((destination|source)&3)||source>4096||bytes>4096-source||
    destination>0x800000||bytes>0x800000-destination)return 0;
 uint32_t words=bytes&~3u;memcpy(ram+destination,dmem+source,words);
 for(uint32_t i=words;i<bytes;i++)ram[(destination+i)^3u]=dmem[(source+i)^3u];
 return 1;
}
