#include "duck_vita_time.h"
#include "duck_vita_threads.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
extern void osCreateMesgQueue_recomp(uint8_t*,recomp_context*);
extern void osViSetEvent_recomp(uint8_t*,recomp_context*);
extern void osViSwapBuffer_recomp(uint8_t*,recomp_context*);
extern void osViGetCurrentFramebuffer_recomp(uint8_t*,recomp_context*);
static uint8_t *ram;static uint64_t ticks;static recomp_context c;
static uint64_t clock_ticks(void *unused){(void)unused;return ticks;}
static void wr(unsigned p,uint32_t value){memcpy(ram+p,&value,4);}
static uint32_t rd(unsigned p){uint32_t v;memcpy(&v,ram+p,4);return v;}
static void setup(void){
 ticks=0;memset(ram,0,0x800000);memset(&c,0,sizeof c);
 assert(!duck_vita_time_bind(ram,clock_ticks,NULL,5));
 osInitialize_recomp(ram,&c);assert(c.status_reg&0x20000000u);
 assert(c.f_odd==&c.f0.u32h);
 c.r4=0x80002000;c.r5=0x80002100;c.r6=8;osCreateMesgQueue_recomp(ram,&c);
}
static void timer(uint32_t key,uint64_t delay,uint64_t interval,uint32_t message){
 c.r4=key;c.r6=delay>>32;c.r7=(uint32_t)delay;c.r29=0xffffffff80005000ull;
 wr(0x5010,(uint32_t)(interval>>32));wr(0x5014,(uint32_t)interval);
 wr(0x5018,0x80002000u);wr(0x501c,message);osSetTimer_recomp(ram,&c);
}
#ifdef DUCK_TIME_FIBER_TEST
static int received;
extern void osRecvMesg_recomp(uint8_t*,recomp_context*);
static void receiver(uint8_t *r,recomp_context *ctx){
 volatile uint32_t local=0x12345678;
 ctx->r4=0x80002000;ctx->r5=0x80003000;ctx->r6=1;
 osRecvMesg_recomp(r,ctx);assert(ctx->r2==0&&rd(0x3000)==0xfeed);
 assert(local==0x12345678);received=1;
}
static recomp_func_t *lookup(uint32_t pc){return pc==0x80001000?receiver:NULL;}
static int pump(void){ticks++;return duck_vita_time_poll();}
#endif
int main(void){
 ram=calloc(1,0x800000);assert(ram);setup();
 timer(0x80004000,100,0,0xdead);assert(c.r2==0);
 assert(rd(0x4010)==0&&rd(0x4014)==100&&rd(0x4018)==0x80002000u);
 ticks=99;assert(duck_vita_time_poll()==0&&rd(0x2008)==0);
 ticks=100;assert(duck_vita_time_poll()==1&&rd(0x2008)==1&&rd(0x2100)==0xdead);
 c.r4=0xa0004000;osStopTimer_recomp(ram,&c);assert(c.r2==(gpr)-1);
 timer(0x80004040,0,10,0xbeef);assert(!c.r2);
 ticks=140;assert(duck_vita_time_poll()==4&&rd(0x2008)==5);
 c.r4=0xffffffffa0004040ull;osStopTimer_recomp(ram,&c);assert(!c.r2);
 ticks=150;assert(!duck_vita_time_poll());
 timer(0x80004080,UINT64_C(0x100000001),0,0x44);assert(!c.r2);
 c.r4=0x80004080;osStopTimer_recomp(ram,&c);assert(!c.r2);
 timer(0x80004080,0,0,0x55);assert(!c.r2);assert(duck_vita_time_poll()==1);
 // Invalid guest bounds and timestamp overflow do not create a timer.
 timer(0x807ffff0,1,0,1);assert(c.r2==(gpr)-1);
 timer(0x80004080,UINT64_MAX,0,1);assert(c.r2==(gpr)-1);
 // Full queues drop nonblocking expiry messages, never fake delivery.
 wr(0x2008,8);timer(0x80004080,1,0,1);ticks=151;assert(duck_vita_time_poll()==1);
 DuckVitaTimeStats stats;duck_vita_time_stats(&stats);assert(stats.dropped_messages==1&&stats.timer_messages==6);
 // CP0 equality re-arms on the next full wrap, not the very next tick.
 duck_cp0_reset(0);c.r4=0;__osSetCompare_recomp(ram,&c);
 duck_cp0_advance(UINT32_MAX);assert(!duck_cp0_timer_pending());
 duck_cp0_advance(1);assert(duck_cp0_timer_pending());
 __osSetCompare_recomp(ram,&c);assert(!duck_cp0_timer_pending());
 duck_cp0_reset(0xfffffff0);c.r4=5;__osSetCompare_recomp(ram,&c);
 duck_cp0_advance(20);assert(!duck_cp0_timer_pending());duck_cp0_advance(1);assert(duck_cp0_timer_pending());
 setup();c.r4=254;osCreateViManager_recomp(ram,&c);
 c.r4=0xffffffff80002000ull;c.r5=0x99;c.r6=2;osViSetEvent_recomp(ram,&c);
 c.r4=0x80010000;osViSwapBuffer_recomp(ram,&c);
 ticks=5;assert(duck_vita_time_poll()==1&&rd(0x2008)==0);
 osViGetCurrentFramebuffer_recomp(ram,&c);assert((uint32_t)c.r2==0x80010000);
 ticks=10;assert(duck_vita_time_poll()==1&&rd(0x2008)==1&&rd(0x2100)==0x99);
 c.r4=0x80002000;c.r5=0x77;c.r6=3;osViSetEvent_recomp(ram,&c);
 ticks=20;assert(duck_vita_time_poll()==2&&rd(0x2008)==1);
 ticks=25;assert(duck_vita_time_poll()==1&&rd(0x2008)==2&&rd(0x2104)==0x77);
 c.r6=0;osViSetEvent_recomp(ram,&c);ticks=30;assert(duck_vita_time_poll()==1&&rd(0x2008)==2);
 setup();timer(0x80004000,0,1,1);ticks=1000;
 assert(duck_vita_time_poll()==128);assert(duck_vita_time_poll()==128);
 ticks=999;assert(duck_vita_time_poll()==-1&&duck_vita_time_error());
#ifdef DUCK_TIME_FIBER_TEST
 setup();received=0;timer(0x80004000,10,0,0xfeed);assert(!c.r2);
 assert(!duck_vita_threads_init(ram,lookup,pump));
 assert(!duck_vita_threads_boot(0x80001000,0x80070000,0,5));
 assert(duck_vita_threads_run(1)==DUCK_THREADS_BUDGET);
 for(unsigned i=0;i<12&&!received;i++)duck_vita_threads_run(1);
 assert(received&&ticks==10);
 assert(duck_vita_threads_run(1)==DUCK_THREADS_IDLE);
 assert(!duck_vita_threads_init(ram,lookup,NULL));
#endif
 free(ram);puts("PASS: 64-bit timer ABI, deadlines, periodic/cancel/drop behavior, Compare wrap, VI events and timer-driven fiber wakeup");return 0;
}
