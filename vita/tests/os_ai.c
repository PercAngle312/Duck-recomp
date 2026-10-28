#include "duck_vita_ai.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static int calls,interrupts,failure;
static int starts,poll_result,start_failure;
static unsigned remaining;
static unsigned progress(void *opaque){(void)opaque;return remaining;}
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
 assert(pcm[14]==30000&&pcm[15]==-30000); // no-lookahead fallback
 // A queued next frame supplies the interpolation neighbour at the last
 // current-DMA sample. Legacy callers without lookahead still clamp safely.
 uint8_t next_frame[4]={0};
 next_frame[3]=0x8a;next_frame[2]=0xd0; // left -30000, logical big endian
 next_frame[1]=0x75;next_frame[0]=0x30; // right +30000
 state.phase=0;assert(duck_vita_audio_resample_joined(&state,ram,0x1000,4,24000,pcm,16,&done,next_frame)==8&&done);
 assert(pcm[12]==30000&&pcm[13]==-30000&&pcm[14]==0&&pcm[15]==0);
 // A carried half input frame at 24 kHz is one full frame at 48 kHz.
 // This keeps the next native sample at the same physical time after a
 // guest frequency switch. A repeated call at that rate must not rescale.
 state.phase=24000;state.phase_rate=24000;
 assert(duck_vita_audio_resample(&state,ram,0x1000,4,48000,pcm,1,&done)==1&&!done);
 assert(pcm[0]==-10000&&pcm[1]==10000&&state.phase==96000&&state.phase_rate==48000);
 assert(duck_vita_audio_resample(&state,ram,0x1000,4,48000,pcm,1,&done)==1&&!done);
 assert(pcm[0]==10000&&pcm[1]==-10000&&state.phase==144000);
 DuckAiAsyncSink sink={begin,poll,0,0,0};unsigned before=(unsigned)interrupts;
 assert(!duck_vita_ai_init_async(ram,48681812,&sink));enqueue(ram,&c);enqueue(ram,&c);
 assert(starts==1&&(unsigned)interrupts==before); // starts without a checkpoint or early IRQ
 assert(!duck_vita_ai_service()&&starts==1);
 assert(!duck_vita_ai_service()&&(unsigned)interrupts==before);
 poll_result=1;assert(duck_vita_ai_service()==1&&starts==2&&(unsigned)interrupts==before+1);
 assert(duck_vita_ai_service()==1&&(unsigned)interrupts==before+2);assert(!duck_vita_ai_service());
 starts=0;start_failure=-9;assert(!duck_vita_ai_init_async(ram,48681812,&sink));enqueue(ram,&c);enqueue(ram,&c);
 before=(unsigned)interrupts;
 assert(duck_vita_ai_service()==-1&&duck_vita_ai_error()==-9&&(unsigned)interrupts==before+1);
 assert(duck_vita_ai_service()==-1&&(unsigned)interrupts==before+1); // second start failed, first completion remains valid
 for(unsigned i=0;i<2;i++){
  starts=0;start_failure=0;poll_result=i?2:-7;
  assert(!duck_vita_ai_init_async(ram,48681812,&sink));enqueue(ram,&c);before=(unsigned)interrupts;
  assert(duck_vita_ai_service()==-1&&(unsigned)interrupts==before);
 }
 starts=0;start_failure=0;poll_result=0;sink.remaining_bytes=progress;
 assert(!duck_vita_ai_init_async(ram,48681812,&sink));enqueue(ram,&c);enqueue(ram,&c);
 remaining=16;assert(!duck_vita_ai_service());osAiGetLength_recomp(ram,&c);assert(c.r2==16);
 remaining=8;osAiGetLength_recomp(ram,&c);assert(c.r2==8);osAiGetStatus_recomp(ram,&c);assert(c.r2==(gpr)(int32_t)0xc0000000u);
 remaining=0;osAiGetLength_recomp(ram,&c);assert(!c.r2); // still awaiting completion poll
 remaining=17;osAiGetLength_recomp(ram,&c);assert(c.r2==(gpr)-1&&duck_vita_ai_error()==-1);
 // Queue-capable sink owns both entries before the first playback completes;
 // the guest FIFO remains full and IRQs still retire in original order.
 starts=0;start_failure=0;poll_result=0;sink.remaining_bytes=0;sink.capacity=2;
 assert(!duck_vita_ai_init_async(ram,48681812,&sink));enqueue(ram,&c);enqueue(ram,&c);
 assert(starts==2); // both owned before any scheduler service
 before=(unsigned)interrupts;assert(!duck_vita_ai_service()&&starts==2);
 osAiGetStatus_recomp(ram,&c);assert(c.r2==(gpr)(int32_t)0xc0000000u);
 assert(!duck_vita_ai_service()&&(unsigned)interrupts==before);
 poll_result=1;assert(duck_vita_ai_service()==1&&(unsigned)interrupts==before+1&&starts==2);
 assert(duck_vita_ai_service()==1&&(unsigned)interrupts==before+2&&starts==2);
 sink.capacity=3;assert(duck_vita_ai_init_async(ram,48681812,&sink)==-1);
 duck_vita_ai_detach();free(ram);
 puts("PASS: AI FIFO limits, real sink completion IRQ, device failure and stereo byte lanes/resampling");return 0;
}
