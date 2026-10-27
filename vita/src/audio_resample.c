#include "duck_vita_ai.h"
static int sample(const uint8_t *ram,uint32_t address){
 unsigned value=((unsigned)ram[address^3u]<<8)|ram[(address+1)^3u];
 return value<=32767?(int)value:(int)value-65536;
}
unsigned duck_vita_audio_resample_joined(DuckAudioResampler *state,const uint8_t *ram,
 uint32_t address,unsigned frames,unsigned rate,int16_t *out,unsigned capacity,int *done,const uint8_t *next_frame){
 if(!done)return 0;
 *done=0;if(!state||!ram||!out||!frames||!rate||!capacity)return 0;
 // Phase is measured in input-frame units. On a DMA rate change, preserve
 // the residual time rather than interpreting the old units at a new rate.
 if(state->phase_rate&&state->phase_rate!=rate)
  state->phase=state->phase*rate/state->phase_rate;
 state->phase_rate=rate;
 uint64_t end=(uint64_t)frames*48000;unsigned produced=0;
 while(state->phase<end&&produced<capacity){
  unsigned i=(unsigned)(state->phase/48000),fraction=(unsigned)(state->phase%48000);
  unsigned next=i+1<frames?i+1:i;
  for(unsigned ch=0;ch<2;ch++){
   int first=sample(ram,address+i*4+ch*2);
   int second=i+1==frames&&next_frame?sample(next_frame,ch*2):sample(ram,address+next*4+ch*2);
   out[produced*2+ch]=(int16_t)(first+((int64_t)(second-first)*fraction)/48000);
  }
  produced++;state->phase+=rate;
 }
 if(state->phase>=end){state->phase-=end;*done=1;}
 return produced;
}
unsigned duck_vita_audio_resample(DuckAudioResampler *state,const uint8_t *ram,
 uint32_t address,unsigned frames,unsigned rate,int16_t *out,unsigned capacity,int *done){
 return duck_vita_audio_resample_joined(state,ram,address,frames,rate,out,capacity,done,0);
}
