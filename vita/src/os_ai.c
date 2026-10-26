/* Two-slot AI DMA FIFO. Completion is emitted only after the sink finishes;
 * production Vita output uses start/poll without blocking the guest host. */
#include "duck_vita_ai.h"
#include "duck_vita_os_io.h"
#include <string.h>
static struct {
 uint8_t *ram;unsigned clock,rate,count;int fault,processing,inflight;
 struct {uint32_t address,bytes;unsigned rate;} fifo[2];
 DuckAiOutput output;DuckAiAsyncSink async;void *opaque;
} ai;
static int bound(void){return ai.output||ai.async.start;}
void duck_vita_ai_detach(void){memset(&ai,0,sizeof ai);}
int duck_vita_ai_init(uint8_t *ram,unsigned clock,DuckAiOutput output,void *opaque){
 if(!ram||!clock||!output)return -1;
 memset(&ai,0,sizeof ai);ai.ram=ram;ai.clock=clock;ai.output=output;ai.opaque=opaque;return 0;
}
int duck_vita_ai_init_async(uint8_t *ram,unsigned clock,const DuckAiAsyncSink *sink){
 if(!ram||!clock||!sink||!sink->start||!sink->poll||sink->capacity>2)return -1;
 memset(&ai,0,sizeof ai);ai.ram=ram;ai.clock=clock;ai.async=*sink;return 0;
}
int duck_vita_ai_error(void){return ai.fault;}
void osAiSetFrequency_recomp(uint8_t *ram,recomp_context *c){
 if(!c)return;
 c->r2=(gpr)-1;if(ram!=ai.ram||!bound()||ai.fault||!c->r4||c->r4>UINT32_MAX)return;
 unsigned request=(unsigned)c->r4;uint64_t divider=((uint64_t)ai.clock+request/2)/request;
 if(divider<132||divider>16384)return;
 ai.rate=ai.clock/(unsigned)divider;c->r2=ai.rate;
}
void osAiSetNextBuffer_recomp(uint8_t *ram,recomp_context *c){
 if(!c)return;
 c->r2=(gpr)-1;uint32_t address=(uint32_t)c->r4,p=address&0x1fffffffu,length=(uint32_t)c->r5;
 if(ram!=ai.ram||!bound()||ai.fault||!ai.rate||ai.count==2||
    (c->r4!=(uint64_t)address&&c->r4!=(uint64_t)(int64_t)(int32_t)address)||c->r5!=length||
    ((address&0xe0000000u)!=0x80000000u&&(address&0xe0000000u)!=0xa0000000u)||
    (p&7)||!length||(length&7)||length>0x3fff8u||p>0x800000u-length)return;
 ai.fifo[ai.count].address=p;ai.fifo[ai.count].bytes=length;ai.fifo[ai.count].rate=ai.rate;ai.count++;c->r2=0;
}
void osAiGetLength_recomp(uint8_t *ram,recomp_context *c){
 if(!c)return;
 if(ram!=ai.ram||!bound()||ai.fault){c->r2=(gpr)-1;return;}
 unsigned bytes=ai.count?ai.fifo[0].bytes:0;
 if(ai.inflight&&ai.async.remaining_bytes){
  unsigned remaining=ai.async.remaining_bytes(ai.async.opaque);
  if(remaining>bytes||(remaining&7u)){ai.fault=-1;c->r2=(gpr)-1;return;}
  bytes=remaining;
 }
 c->r2=bytes;
}
void osAiGetStatus_recomp(uint8_t *ram,recomp_context *c){
 if(!c)return;
 if(ram!=ai.ram||!bound()||ai.fault){c->r2=(gpr)-1;return;}
 uint32_t status=(ai.count?0x40000000u:0)|(ai.count==2?0x80000000u:0);
 // N64 lw sign-extends the 32-bit hardware register, including FIFO_FULL.
 c->r2=(gpr)(int32_t)status;
}
int duck_vita_ai_service(void){
 if(ai.fault)return -1;
 if(!bound()||!ai.count||ai.processing)return 0;
 ai.processing=1;
 if(ai.async.start){
  unsigned limit=ai.async.capacity?ai.async.capacity:1;
  int started=0;
  while((unsigned)ai.inflight<ai.count&&(unsigned)ai.inflight<limit){
   unsigned i=(unsigned)ai.inflight;
   int r=ai.async.start(ai.async.opaque,ai.ram,ai.fifo[i].address,ai.fifo[i].bytes/4,ai.fifo[i].rate);
   if(r){ai.processing=0;ai.fault=r<0?r:-1;return -1;}
   ai.inflight++;started=1;
  }
  if(started){ai.processing=0;return 0;}
  int r=ai.async.poll(ai.async.opaque);
  if(!r){ai.processing=0;return 0;}
  if(r!=1){ai.processing=0;ai.fault=r<0?r:-1;return -1;}
  ai.inflight--;ai.fifo[0]=ai.fifo[1];ai.count--;
  // Start the already-queued next DMA before returning to guest execution.
  if((unsigned)ai.inflight<ai.count){
   unsigned i=(unsigned)ai.inflight;
   r=ai.async.start(ai.async.opaque,ai.ram,ai.fifo[i].address,ai.fifo[i].bytes/4,ai.fifo[i].rate);
   if(r)ai.fault=r<0?r:-1;else ai.inflight++;
  }
  ai.processing=0;duck_vita_os_event_emit(6);
  return ai.fault?-1:1;
 }
 int result=ai.output(ai.opaque,ai.ram,ai.fifo[0].address,ai.fifo[0].bytes/4,ai.fifo[0].rate);
 ai.processing=0;
 if(result){ai.fault=result;return -1;}
 ai.fifo[0]=ai.fifo[1];ai.count--;
 duck_vita_os_event_emit(6); // OS_EVENT_AI, after successful output only
 return 1;
}
