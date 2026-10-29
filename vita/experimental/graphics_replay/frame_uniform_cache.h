#pragma once
#include <cstdint>
#include <cstring>
struct DuckUniformStamp {
 uint32_t frame=0,noise_bits=0;bool valid=false;
 bool changed(uint32_t next_frame,float noise){
  uint32_t bits;std::memcpy(&bits,&noise,4);
  if(valid&&frame==next_frame&&noise_bits==bits)return false;
  frame=next_frame;noise_bits=bits;valid=true;return true;
 }
};
