#include "duck_vita_threads.h"
#include "duck_vita_os_io.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fenv.h>
extern void osCreateMesgQueue_recomp(uint8_t*,recomp_context*);
extern void osSendMesg_recomp(uint8_t*,recomp_context*);
extern void osRecvMesg_recomp(uint8_t*,recomp_context*);
static uint8_t *ram;static int phase,trace[64],count,after_destroy;
static void record(int n){assert(count<64);trace[count++]=n;}
static void wr(unsigned p,uint32_t v){memcpy(ram+p,&v,4);}
static uint32_t rd(unsigned p){uint32_t v;memcpy(&v,ram+p,4);return v;}
static void make(recomp_context *c,uint32_t thread,uint32_t pc,unsigned priority){
 unsigned sp=(uint32_t)c->r29&0x1fffffffu;wr(sp+16,0x80060000u-(thread&0xffff));wr(sp+20,priority);
 c->r4=thread;c->r5=0;c->r6=pc;c->r7=0x12345678;osCreateThread_recomp(ram,c);
 c->r4=thread;osStartThread_recomp(ram,c);
}
static void receive(uint8_t *r,recomp_context *c){
 assert(c->f_odd==&c->f0.u32h);
 volatile unsigned local[64];for(unsigned i=0;i<64;i++)local[i]=i^0xabcdef;
 assert(c->r4==0x12345678);record(2);
 c->r4=0x80002000;c->r5=0x80003000;c->r6=1;osRecvMesg_recomp(r,c);
 assert(c->r2==0&&rd(0x3000)==11);record(4);
 for(unsigned i=0;i<64;i++)assert(local[i]==(i^0xabcdef));
 osRecvMesg_recomp(r,c);assert(c->r2==0&&rd(0x3000)==22);record(6);
 for(unsigned i=0;i<64;i++)assert(local[i]==(i^0xabcdef));
}
static void send(uint8_t *r,recomp_context *c){
 volatile unsigned local=0xdeadbeef;record(3);
 c->r4=0x80002000;c->r5=11;c->r6=1;osSendMesg_recomp(r,c);assert(!c->r2&&local==0xdeadbeef);record(5);
 c->r5=22;osSendMesg_recomp(r,c);assert(!c->r2&&local==0xdeadbeef);record(7);
 if(phase==2){c->r4=0;osDestroyThread_recomp(r,c);after_destroy++;}
}
static void root(uint8_t *r,recomp_context *c){
 volatile unsigned local=0x314159;record(1);
 c->r4=0x80002000;c->r5=0x80002100;c->r6=1;osCreateMesgQueue_recomp(r,c);
 if(phase==3){
  make(c,0x80004000,0x80004000,1);make(c,0x80004400,0x80005000,1);
  osYieldThread_recomp(r,c);assert(local==0x314159);record(8);return;
 }
 if(phase==1){make(c,0x80004000,0x80002000,5);make(c,0x80004400,0x80003000,3);}
 else {make(c,0x80004400,0x80003000,3);make(c,0x80004000,0x80002000,2);}
 assert(local==0x314159);record(8);
}
static void equal_a(uint8_t *r,recomp_context *c){
 assert(!fesetround(FE_DOWNWARD));record(11);osYieldThread_recomp(r,c);
 assert(fegetround()==FE_DOWNWARD);record(13);
}
static void equal_b(uint8_t *r,recomp_context *c){
 assert(!fesetround(FE_UPWARD));record(12);osYieldThread_recomp(r,c);
 assert(fegetround()==FE_UPWARD);record(14);
}
static recomp_func_t *lookup(uint32_t pc){return pc==0x80001000?root:pc==0x80002000?receive:pc==0x80003000?send:pc==0x80004000?equal_a:pc==0x80005000?equal_b:NULL;}
int main(void){
 ram=calloc(1,0x800000);assert(ram);
 for(phase=1;phase<=3;phase++){
  memset(ram,0,0x800000);count=0;
  assert(!duck_vita_threads_init(ram,lookup,NULL));
  assert(!duck_vita_threads_boot(0x80001000,0x80070000,0,1));
  assert(duck_vita_threads_run(1)==DUCK_THREADS_BUDGET);
  assert(duck_vita_threads_run(100)==DUCK_THREADS_IDLE);
  int a[]={1,2,3,4,5,6,7,8},b[]={1,3,5,2,7,4,6,8},equal[]={1,11,12,8,13,14};
  assert(count==(phase==3?6:8));assert(!memcmp(trace,phase==3?equal:phase==1?a:b,(size_t)count*sizeof(int)));assert(!after_destroy);
  assert(!duck_vita_threads_error());
 }
 // No runnable work is distinct from an empty queue with suspended receivers.
 assert(!duck_vita_threads_init(ram,lookup,NULL));
 recomp_context c={0};c.r4=0x80002000;c.r5=0x80002100;c.r6=1;osCreateMesgQueue_recomp(ram,&c);
 assert(!duck_vita_threads_boot(0x80002000,0x80070000,0x12345678,5));
 assert(duck_vita_threads_run(10)==DUCK_THREADS_WAITING);
 c.r4=0xa0002000;c.r5=11;c.r6=0;osSendMesg_recomp(ram,&c);assert(c.r2==0);
 assert(duck_vita_threads_run(10)==DUCK_THREADS_WAITING);
 c.r5=22;osSendMesg_recomp(ram,&c);assert(c.r2==0);
 assert(duck_vita_threads_run(10)==DUCK_THREADS_IDLE);
 assert(!duck_vita_threads_init(ram,lookup,NULL));
 free(ram);puts("PASS: native stacks survive blocking recv/send, priority preemption, self-destroy, dispatch budgets and host-event wakeups");return 0;
}
