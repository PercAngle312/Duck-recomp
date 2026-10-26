#include "duck_vita_ai.h"
static int sample(const uint8_t *ram,uint32_t address){
 unsigned value=((unsigned)ram[address^3u]<<8)|ram[(address+1)^3u];
 return value<=32767?(int)value:(int)value-65536;
}
unsigned duck_vita_audio_resample(DuckAudioResampler *state,const uint8_t *ram,
 uint32_t address,unsigned frames,unsigned rate,int16_t *out,unsigned capacity,int *done){
 if(!done)return 0;
 *done=0;if(!state||!ram||!out||!frames||!rate||!capacity)return 0;
 uint64_t end=(uint64_t)frames*48000;unsigned produced=0;
 while(state->phase<end&&produced<capacity){
  unsigned i=(unsigned)(state->phase/48000),fraction=(unsigned)(state->phase%48000);
  unsigned next=i+1<frames?i+1:i;
  for(unsigned ch=0;ch<2;ch++){
   int first=sample(ram,address+i*4+ch*2),second=sample(ram,address+next*4+ch*2);
   out[produced*2+ch]=(int16_t)(first+((int64_t)(second-first)*fraction)/48000);
  }
  produced++;state->phase+=rate;
 }
 if(state->phase>=end){state->phase-=end;*done=1;}
 return produced;
}
