#include "duck_vita_threads.h"
#include <assert.h>
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void osCreateMesgQueue_recomp(uint8_t*,recomp_context*);
extern void osRecvMesg_recomp(uint8_t*,recomp_context*);
extern void osSendMesg_recomp(uint8_t*,recomp_context*);
static uint8_t *ram;
static unsigned polls,spinning,delivered,received,peer_ran,fail_service;
static void word(unsigned at,uint32_t value){memcpy(ram+at,&value,4);}
static void start(recomp_context *c,uint32_t object,uint32_t entry,unsigned priority){
 unsigned sp=(uint32_t)c->r29&0x1fffffffu;word(sp+16,0x80060000u-(object&0xffff));word(sp+20,priority);
 c->r4=object;c->r5=1;c->r6=entry;c->r7=0;osCreateThread_recomp(ram,c);
 c->r4=object;osStartThread_recomp(ram,c);
}
static int service(void){
 polls++;
 if(spinning&&polls>4&&!delivered){
  if(fail_service)return -1;
  recomp_context c={0};c.r4=0x80002000;c.r5=0x12345678;c.r6=0;
  osSendMesg_recomp(ram,&c);assert(!c.r2);delivered=1;
 }
 return 0;
}
static void receiver(uint8_t *r,recomp_context *c){
 c->r4=0x80002000;c->r5=0x80003000;c->r6=1;osRecvMesg_recomp(r,c);
 assert(!c->r2&&delivered);assert(!fesetround(FE_UPWARD));received=1;
}
static void peer(uint8_t *r,recomp_context *c){(void)r;(void)c;peer_ran=1;}
static void busy(uint8_t *r,recomp_context *c){
 c->r4=0x80002000;c->r5=0x80002100;c->r6=1;osCreateMesgQueue_recomp(r,c);
 start(c,0x80004000,0x80002000,5); // higher priority blocks on SI-style queue
 start(c,0x80004400,0x80003000,1); // equal priority must not steal a checkpoint
 volatile uint64_t local=0x123456789abcdef0ull;
 assert(!fesetround(FE_DOWNWARD));spinning=1;
 unsigned before=polls;
 const uint32_t masks[]={0xff00,0xff03,0xff05,1}; // IE off, EXL, ERL, all IM off
 for(unsigned j=0;j<sizeof masks/sizeof masks[0];j++){
  c->status_reg=masks[j];
  for(unsigned i=0;i<DUCK_THREADS_CHECKPOINT_QUANTUM*2;i++)duck_vita_thread_checkpoint(r,c);
  assert(polls==before&&!duck_vita_threads_checkpoint_yields());
 }
 c->status_reg=0xff01;c->r8=0;
 while(!received){
  c->r8++;gpr value=c->r8;
  duck_vita_thread_checkpoint(r,c); // no OS calls in this waiting loop
  assert(c->r8==value&&local==0x123456789abcdef0ull&&fegetround()==FE_DOWNWARD);
  assert(c->r8<DUCK_THREADS_CHECKPOINT_QUANTUM*10);
 }
 assert(!peer_ran&&duck_vita_threads_checkpoint_yields()>=1);
}
static recomp_func_t *lookup(uint32_t pc){return pc==0x80001000?busy:pc==0x80002000?receiver:pc==0x80003000?peer:NULL;}
int main(void){
 ram=calloc(1,0x800000);assert(ram);
 for(unsigned phase=0;phase<2;phase++){
  polls=spinning=delivered=received=peer_ran=0;fail_service=phase;
  assert(!duck_vita_threads_init(ram,lookup,service));
  assert(!duck_vita_threads_boot(0x80001000,0x80070000,0,1));
  int result=duck_vita_threads_run(30);
  if(phase)assert(result==DUCK_THREADS_ERROR&&duck_vita_threads_error());
  else assert(result==DUCK_THREADS_IDLE&&received&&peer_ran&&!duck_vita_threads_error());
 }
 assert(!duck_vita_threads_init(ram,lookup,NULL));free(ram);
 puts("PASS: busy loop returns to host, higher-priority message wakeup, masked-region guard, equal-priority order, native locals/FPU state and host-service failure");return 0;
}
