#include "duck_vita_ai.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
unsigned reference_joined(DuckAudioResampler*,const uint8_t*,uint32_t,unsigned,unsigned,int16_t*,unsigned,int*,const uint8_t*);
static unsigned seed=81723;
static unsigned random32(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(void){
 uint8_t *ram=malloc(500000);assert(ram);for(unsigned i=0;i<500000;i++)ram[i]=(uint8_t)random32();
 uint64_t total=0;DuckAudioResampler a={0},b={0};int16_t x[2048],y[2048];
 const unsigned rates[]={8000,11025,22050,24004,32000,44100,48000,96000,368801};
 for(unsigned test=0;test<6000;test++){
  unsigned frames=test%31==0?90000:test%29==0?65534:1+random32()%4096;
  unsigned rate=rates[random32()%9];const uint8_t *next=test&1?ram+400000:NULL;
  if(test%31==0){a.phase=b.phase=UINT32_MAX-15ull;a.phase_rate=b.phase_rate=rate;}
  int da=0,db=0;unsigned steps=0;
  do{
   unsigned cap=1+random32()%1024;
   unsigned na=duck_vita_audio_resample_joined(&a,ram,0,frames,rate,x,cap,&da,next);
   unsigned nb=reference_joined(&b,ram,0,frames,rate,y,cap,&db,next);
   assert(na==nb&&da==db&&a.phase==b.phase&&a.phase_rate==b.phase_rate);
   assert(!memcmp(x,y,na*2*sizeof *x));total+=na;assert(++steps<600000);
  }while(!da);
 }
 printf("PASS resampler: 6000 DMA cases, %llu output frames identical to v019; chunking, rate changes, joined stereo, max DMA and 64-bit fallback\n",(unsigned long long)total);
 free(ram);
}
