#include "duck_vita_os_io.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static uint8_t *ram;
static void wr(unsigned p,uint32_t v){for(unsigned i=0;i<4;i++)ram[p+i]=(uint8_t)(v>>(8*i));}
static uint32_t rd(unsigned p){return (uint32_t)ram[p]|((uint32_t)ram[p+1]<<8)|((uint32_t)ram[p+2]<<16)|((uint32_t)ram[p+3]<<24);}
static void queue(unsigned q,unsigned buffer,unsigned size){
 wr(q,0);wr(q+4,0);wr(q+8,0);wr(q+12,0);wr(q+16,size);wr(q+20,0x80000000u|buffer);
}
static recomp_context ctx;
static void request(unsigned mb,unsigned priority,unsigned rom,unsigned dest,unsigned size) {
 ctx.r4=0x80000000u|mb;ctx.r5=priority;ctx.r6=0;ctx.r7=rom;ctx.r29=0xffffffff80005000ull;
 wr(0x5010,0x80000000u|dest);wr(0x5014,size);wr(0x5018,0x80003000u);
 osPiStartDma_recomp(ram,&ctx);
}
int main(void) {
 uint8_t rom[256];for(unsigned i=0;i<256;i++)rom[i]=(uint8_t)(i^0x5a);
 ram=calloc(1,0x800000);assert(ram);
 assert(!duck_vita_os_io_reset(ram,rom,sizeof rom));
 queue(0x3000,0x3100,4);
 ctx.r4=0x10000004;ctx.r5=0xffffffffa0000100ull;
 osPiReadIo_recomp(ram,&ctx);
 assert(ctx.r2==0&&rd(0x100)==0x5e5f5c5du);
 ctx.r4=0x10000100;osPiReadIo_recomp(ram,&ctx);
 assert(ctx.r2==(gpr)-1&&rd(0x100)==0x5e5f5c5du);
 ctx.r4=0;ctx.r5=0x18000100u;osPiReadIo_recomp(ram,&ctx);assert(ctx.r2==(gpr)-1);
 request(0x4000,0,8,0x6000,16);assert(ctx.r2==(gpr)-1); // manager not created
 ctx.r4=100;ctx.r5=0x80002000u;ctx.r6=0x80002100u;ctx.r7=2;
 osCreatePiManager_recomp(ram,&ctx);
 assert(rd(0x2010)==2&&rd(0x2014)==0x80002100u);
 request(0x4000,0,8,0x6000,16);assert(ctx.r2==0);
 request(0x4040,1,24,0x6020,8);assert(ctx.r2==0);
 assert(rd(0x6000)==0); // queued, no early success/copy
 request(0x4080,0,0,0x6040,8);assert(ctx.r2==(gpr)-1); // command queue full
 assert(duck_vita_os_pi_service()==1);
 assert(rd(0x3100)==0x80004040u); // priority job, actual OSIoMesg pointer
 assert(rd(0x6000)==0);
 for(unsigned i=0;i<8;i++)assert(ram[(0x6020+i)^3]==rom[24+i]);
 assert(rd(0x4040)==0x000b0100u && rd(0x4044)==0x80003000u);
 assert(duck_vita_os_pi_service()==1 && duck_vita_os_pi_service()==0);
 assert(rd(0x3104)==0x80004000u);
 for(unsigned i=0;i<16;i++)assert(ram[(0x6000+i)^3]==rom[8+i]);
 request(0x4080,0,250,0x6040,16);assert(ctx.r2==(gpr)-1&&rd(0x6040)==0);
 request(0x4080,0,0,0x7ffff8,16);assert(ctx.r2==(gpr)-1);
 request(0x4080,0,0,0x6041,8);assert(ctx.r2==(gpr)-1);
 ctx.r4=8;ctx.r5=0x80003000u;ctx.r6=0x12345678;
 osSetEventMesg_recomp(ram,&ctx);assert(!duck_vita_os_event_emit(8));
 assert(rd(0x3108)==0x12345678);
 assert(!duck_vita_os_event_emit(8));assert(duck_vita_os_event_emit(8)==-1); // full
 ctx.r4=8;ctx.r5=0;osSetEventMesg_recomp(ram,&ctx);assert(duck_vita_os_event_emit(8)==-1);
 assert(duck_vita_os_event_emit(23)==-1);
 // No event is delivered just because its registration changed.
 assert(rd(0x3008)==4);
 assert(duck_vita_os_io_reset(0,rom,sizeof rom)==-1);
 assert(duck_vita_os_pi_service()==0&&duck_vita_os_event_emit(8)==-1);
 free(ram);puts("PASS: PIO lanes/bounds, queued priority ROM DMA, OSIoMesg completion, event registration/delivery/full queues");
}
