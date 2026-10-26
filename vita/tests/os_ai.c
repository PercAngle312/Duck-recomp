#include "duck_vita_ai.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static int calls,interrupts,failure;
static int starts,poll_result,start_failure;
static int begin(void *opaque,const uint8_t *ram,uint32_t address,unsigned frames,unsigned rate){
 (void)opaque;assert(ram&&address==0x1000&&frames==4&&rate>31000&&rate<33000);
 starts++;return starts==2?start_failure:0;
}
static int poll(void *opaque){(void)opaque;return poll_result;}
static void enqueue(uint8_t *ram,recomp_context *c){
 c->r4=32000;osAiSetFrequency_recomp(ram,c);assert(c->r2>31000&&c->r2<33000);
 c->r4=0x80001000;c->r5=16;osAiSetNextBuffer_recomp(ram,c);assert(!c->r2);
}
int duck_vita_os_event_emit(unsigned event){assert(event==6);interrupts++;return 0;}
static int output(void *opaque,const uint8_t *ram,uint32_t address,unsigned frames,unsigned rate){
 (void)opaque;assert(ram&&address==0x1000&&frames==4&&rate>31000&&rate<33000);calls++;return failure;
}
int main(void){
 uint8_t *ram=calloc(1,0x800000);assert(ram);recomp_context c={0};
 assert(!duck_vita_ai_init(ram,48681812,output,0));c.r4=32000;osAiSetFrequency_recomp(ram,&c);assert(c.r2>31000&&c.r2<33000);
 c.r4=0xffffffff80001000ull;c.r5=16;osAiSetNextBuffer_recomp(ram,&c);assert(!c.r2);
 osAiSetNextBuffer_recomp(ram,&c);assert(!c.r2);osAiGetStatus_recomp(ram,&c);assert(c.r2==(gpr)(int32_t)0xc0000000u);
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
 assert(pcm[2]==-20000&&pcm[3]==20000);
 DuckAiAsyncSink sink={begin,poll,0};unsigned before=(unsigned)interrupts;
 assert(!duck_vita_ai_init_async(ram,48681812,&sink));enqueue(ram,&c);enqueue(ram,&c);
 assert(!duck_vita_ai_service()&&starts==1);
 assert(!duck_vita_ai_service()&&(unsigned)interrupts==before);
 poll_result=1;assert(duck_vita_ai_service()==1&&starts==2&&(unsigned)interrupts==before+1);
 assert(duck_vita_ai_service()==1&&(unsigned)interrupts==before+2);assert(!duck_vita_ai_service());
 starts=0;start_failure=-9;assert(!duck_vita_ai_init_async(ram,48681812,&sink));enqueue(ram,&c);enqueue(ram,&c);
 before=(unsigned)interrupts;assert(!duck_vita_ai_service());
 assert(duck_vita_ai_service()==-1&&duck_vita_ai_error()==-9&&(unsigned)interrupts==before+1);
 assert(duck_vita_ai_service()==-1&&(unsigned)interrupts==before+1); // second start failed, first completion remains valid
 for(unsigned i=0;i<2;i++){
  starts=0;start_failure=0;poll_result=i?2:-7;
  assert(!duck_vita_ai_init_async(ram,48681812,&sink));enqueue(ram,&c);before=(unsigned)interrupts;
  assert(!duck_vita_ai_service());assert(duck_vita_ai_service()==-1&&(unsigned)interrupts==before);
 }
 duck_vita_ai_detach();free(ram);
 puts("PASS: AI FIFO limits, real sink completion IRQ, device failure and stereo byte lanes/resampling");return 0;
}
