#pragma once
#include <cstdint>
#include <cstring>
// Word-swapped DMEM <-> reversed eight-byte vector slice. Full eight-byte
// accesses at every byte alignment; wrapping spans retain the original loop.
inline bool duck_rsp_sample_load8(void *destination,const uint8_t *dmem,uint32_t address){
 address&=4095u;if(address>4088)return false;
 const unsigned shift=(address&3u)*8;const uint8_t *p=dmem+(address&~3u);
 uint32_t first,second,third=0;std::memcpy(&first,p,4);std::memcpy(&second,p+4,4);
 uint32_t words[2];
 if(shift){std::memcpy(&third,p+8,4);words[0]=(second<<shift)|(third>>(32-shift));words[1]=(first<<shift)|(second>>(32-shift));}
 else{words[0]=second;words[1]=first;}
 std::memcpy(destination,words,8);return true;
}
inline bool duck_rsp_sample_store8(uint8_t *dmem,uint32_t address,const void *source){
 address&=4095u;if(address>4088)return false;
 uint32_t words[2];std::memcpy(words,source,8);const uint32_t first=words[1],second=words[0];
 const unsigned shift=(address&3u)*8;uint8_t *p=dmem+(address&~3u);
 if(!shift){std::memcpy(p,&first,4);std::memcpy(p+4,&second,4);}
 else{
  uint32_t a,c;std::memcpy(&a,p,4);std::memcpy(&c,p+8,4);
  a=(a&(~0u<<(32-shift)))|(first>>shift);
  const uint32_t b=(first<<(32-shift))|(second>>shift);
  c=(c&(~0u>>shift))|(second<<(32-shift));
  std::memcpy(p,&a,4);std::memcpy(p+4,&b,4);std::memcpy(p+8,&c,4);
 }
 return true;
}
