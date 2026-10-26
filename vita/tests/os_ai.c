#include "duck_vita_ai.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static int calls,interrupts,failure;
int duck_vita_os_event_emit(unsigned event){assert(event==6);interrupts++;return 0;}
static int output(void *opaque,const uint8_t *ram,uint32_t address,unsigned frames,unsigned rate){
 (void)opaque;assert(ram&&address==0x1000&&frames==4&&rate>31000&&rate<33000);calls++;return failure;
}
int main(void){
 uint8_t *ram=calloc(1,0x800000);assert(ram);recomp_context c={0};
 assert(!duck_vita_ai_init(ram,48681812,output,0));c.r4=32000;osAiSetFrequency_recomp(ram,&c);assert(c.r2>31000&&c.r2<33000);
 c.r4=0xffffffff80001000ull;c.r5=16;osAiSetNextBuffer_recomp(ram,&c);assert(!c.r2);
 osAiSetNextBuffer_recomp(ram,&c);assert(!c.r2);osAiGetStatus_recomp(ram,&c);assert(c.r2==0xc0000000u);
 osAiSetNextBuffer_recomp(ram,&c);assert(c.r2==(gpr)-1);assert(!calls&&!interrupts);
 assert(duck_vita_ai_service()==1&&calls==1&&interrupts==1);osAiGetLength_recomp(ram,&c);assert(c.r2==16);
 assert(duck_vita_ai_service()==1&&interrupts==2);osAiGetStatus_recomp(ram,&c);assert(!c.r2);
 osAiSetNextBuffer_recomp(ram,&c);assert(!c.r2);failure=-8;assert(duck_vita_ai_service()==-1&&interrupts==2&&duck_vita_ai_error()==-8);
 assert(!duck_vita_ai_init(ram,48681812,output,0));c.r4=1;osAiSetFrequency_recomp(ram,&c);assert(c.r2==(gpr)-1);
 int16_t expected[8]={-30000,30000,-10000,10000,10000,-10000,30000,-30000};
 for(unsigned i=0;i<8;i++){unsigned value=(uint16_t)expected[i];ram[(0x1000+i*2)^3u]=(uint8_t)(value>>8);ram[(0x1001+i*2)^3u]=(uint8_t)value;}
 DuckAudioResampler state={0};int16_t pcm[32];int done=0;
 assert(duck_vita_audio_resample(&state,ram,0x1000,4,48000,pcm,16,&done)==4&&done);
 for(unsigned i=0;i<8;i++)assert(pcm[i]==expected[i]);
 state.phase=0;assert(duck_vita_audio_resample(&state,ram,0x1000,4,24000,pcm,16,&done)==8&&done);
 assert(pcm[2]==-20000&&pcm[3]==20000);free(ram);
 puts("PASS: AI FIFO limits, real sink completion IRQ, device failure and stereo byte lanes/resampling");return 0;
}
