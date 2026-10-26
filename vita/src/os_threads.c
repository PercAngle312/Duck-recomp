#include "duck_vita_threads.h"
#include <string.h>
enum { FREE,STOPPED,READY,RUNNING,RECV,SEND,DEAD,MAX_THREADS=16 };
static struct {uint32_t address,entry,queue,rcp_mask;unsigned checkpoints;int priority,state;uint64_t order;recomp_context ctx;} threads[MAX_THREADS];
static uint8_t *ram;
static DuckThreadLookup lookup;
static DuckThreadPump pump;
static int current=-1,fault;
static uint64_t clock_order;
static uint64_t checkpoint_yields;
static uint32_t boot_rcp_mask=0x3f;
uint32_t duck_vita_thread_rcp_mask(void){return current>=0?threads[current].rcp_mask:boot_rcp_mask;}
void duck_vita_thread_set_rcp_mask(uint32_t mask){
 if(current<0){boot_rcp_mask=mask&0x3f;return;}
 threads[current].rcp_mask=mask&0x3f;
 if(threads[current].address){
  uint32_t value=mask&0x3f;
  memcpy(ram+(threads[current].address&0x1fffffffu)+0x128,&value,4);
 }
}
static int offset(uint64_t v,uint32_t size,uint32_t *out) {
 uint32_t a=(uint32_t)v,p=a&0x1fffffffu;
 if(v!=(uint64_t)a&&v!=(uint64_t)(int64_t)(int32_t)a)return -1;
 if((a&0xe0000000u)!=0x80000000u&&(a&0xe0000000u)!=0xa0000000u)return -1;
 if((p&3)||p>0x800000u||size>0x800000u-p)return -1;
 *out=p;return 0;
}
static uint32_t rd(uint32_t p){uint32_t v;memcpy(&v,ram+p,4);return v;}
static void wr(uint32_t p,uint32_t v){memcpy(ram+p,&v,4);}
static void state(int i,int value) {
 threads[i].state=value;
 if(threads[i].address){
  uint32_t p=threads[i].address&0x1fffffffu;
  unsigned guest=value==RUNNING?4:value==READY?2:(value==RECV||value==SEND)?8:1;
  wr(p+16,(guest<<16)|(rd(p+16)&0xffff));
 }
 if(value==READY)threads[i].order=++clock_order;
}
static int find(uint32_t address){
 if(!address)return current;
 for(int i=0;i<MAX_THREADS;i++)if(threads[i].state!=FREE&&threads[i].state!=DEAD&&
    threads[i].address&& (threads[i].address&0x1fffffffu)==(address&0x1fffffffu))return i;
 return -1;
}
static int best(void){
 int b=-1;
 for(int i=0;i<MAX_THREADS;i++)if(threads[i].state==READY&&
  (b<0||threads[i].priority>threads[b].priority||
   (threads[i].priority==threads[b].priority&&threads[i].order<threads[b].order)))b=i;
 return b;
}
static void suspend(void){int i=current;duck_fiber_suspend((unsigned)i);}
uint64_t duck_vita_threads_checkpoint_yields(void){return checkpoint_yields;}
void duck_vita_thread_checkpoint(uint8_t *rdram,recomp_context *ctx){
 if(current<0)return; // Direct host invocations have no guest stack to suspend.
 if(rdram!=ram||ctx!=&threads[current].ctx){fault=1;suspend();return;}
 if(--threads[current].checkpoints)return;
 threads[current].checkpoints=DUCK_THREADS_CHECKPOINT_QUANTUM;
 // Never insert a guest scheduling boundary inside disabled-IRQ or exception
 // regions. This is cooperative scheduling, not cycle-accurate CPU exceptions.
 if((ctx->status_reg&7u)!=1u||!(ctx->status_reg&0xff00u))return;
 uint64_t order=threads[current].order;
 state(current,READY);
 // A host poll must not act like osYieldThread's equal-priority rotation.
 threads[current].order=order;
 checkpoint_yields++;
 suspend();
}
static void preempt(void){
 int b=best();if(current>=0&&b>=0&&threads[b].priority>threads[current].priority){state(current,READY);suspend();}
}
static int wait_queue(uint32_t queue,int sending){
 if(current<0)return -1;
 threads[current].queue=queue&0x1fffffffu;threads[current].order=++clock_order;state(current,sending?SEND:RECV);suspend();return fault?-1:0;
}
static void wake_queue(uint32_t queue,int sending){
 int b=-1;
 for(int i=0;i<MAX_THREADS;i++)if(threads[i].state==(sending?SEND:RECV)&&
  threads[i].queue==(queue&0x1fffffffu)&&(b<0||threads[i].priority>threads[b].priority||
  (threads[i].priority==threads[b].priority&&threads[i].order<threads[b].order)))b=i;
 if(b>=0)state(b,READY);
}
static void entry(unsigned i){
 recomp_func_t *function=lookup(threads[i].entry);
 if(!function)fault=1;
 else function(ram,&threads[i].ctx);
 state((int)i,DEAD);suspend();
 /* A dead guest stack must never resume. */
 for(;;){fault=1;suspend();}
}
static int create(uint32_t address,uint32_t pc,uint32_t stack,uint32_t arg,int priority){
 uint32_t p;
 if(!ram||!lookup||priority<0||priority>255||offset(stack-16u,16,&p)||
    (pc&3)||((pc&0xe0000000u)!=0x80000000u&&(pc&0xe0000000u)!=0xa0000000u))return -1;
 if(address&&offset(address,0x1b0,&p))return -1;
 if(address&&find(address)>=0)return -1;
 int i;for(i=0;i<MAX_THREADS&&threads[i].state!=FREE;i++);
 if(i==MAX_THREADS)return -1;
 memset(&threads[i],0,sizeof threads[i]);threads[i].address=address;threads[i].entry=pc;
 threads[i].checkpoints=DUCK_THREADS_CHECKPOINT_QUANTUM;
 threads[i].priority=priority;threads[i].ctx.r29=(gpr)(int32_t)(stack-16);
 threads[i].rcp_mask=0x3f;
 threads[i].ctx.r4=(gpr)(int32_t)arg;threads[i].ctx.status_reg=0xff01;
 threads[i].ctx.f_odd=&threads[i].ctx.f0.u32h;
 if(duck_fiber_create((unsigned)i,entry))return -1;
 if(address){p=address&0x1fffffffu;for(unsigned n=0;n<0x1b0;n+=4)wr(p+n,0);wr(p+4,(uint32_t)priority);}
 state(i,STOPPED);return i;
}
int duck_vita_threads_init(uint8_t *rdram,DuckThreadLookup resolve,DuckThreadPump service){
 if(current>=0)return -1;
 for(unsigned i=0;i<MAX_THREADS;i++){duck_fiber_destroy(i);memset(&threads[i],0,sizeof threads[i]);}
 ram=rdram;lookup=resolve;pump=service;fault=0;clock_order=checkpoint_yields=0;boot_rcp_mask=0x3f;
 if(duck_fiber_prepare()){fault=1;return -1;}
 duck_vita_os_queue_hooks(wait_queue,wake_queue,preempt);
 return ram&&lookup?0:-1;
}
int duck_vita_threads_boot(uint32_t pc,uint32_t stack,uint32_t arg,int priority){
 int i=create(0,pc,stack,arg,priority);if(i<0)return -1;state(i,READY);return 0;
}
int duck_vita_threads_error(void){return fault;}
int duck_vita_threads_run(unsigned budget){
 if(current>=0||!ram||!lookup||fault)return DUCK_THREADS_ERROR;
 for(unsigned n=0;n<budget;n++){
  if(pump&&pump()<0){fault=1;return DUCK_THREADS_ERROR;}
  int i=best();
  if(i<0){for(int j=0;j<MAX_THREADS;j++)if(threads[j].state==RECV||threads[j].state==SEND)return DUCK_THREADS_WAITING;return DUCK_THREADS_IDLE;}
  current=i;state(i,RUNNING);
  if(duck_fiber_run((unsigned)i))fault=1;
  current=-1;
  if(threads[i].state==DEAD){duck_fiber_destroy((unsigned)i);threads[i].state=FREE;}
  if(fault)return DUCK_THREADS_ERROR;
 }
 return DUCK_THREADS_BUDGET;
}
void osCreateThread_recomp(uint8_t *r,recomp_context *c){
 uint32_t sp,p;if(!c||r!=ram||offset(c->r29,24,&sp)||offset(c->r4,0x1b0,&p)){fault=1;return;}
 int i=create((uint32_t)c->r4,(uint32_t)c->r6,rd(sp+16),(uint32_t)c->r7,(int32_t)rd(sp+20));
 if(i<0){fault=1;return;}wr(p+20,(uint32_t)c->r5);
}
void osStartThread_recomp(uint8_t *r,recomp_context *c){
 if(!c||r!=ram){fault=1;return;}int i=find((uint32_t)c->r4);
 if(i<0){fault=1;return;}
 if(threads[i].state==STOPPED||threads[i].state==RECV||threads[i].state==SEND)state(i,READY);
 preempt();
}
void osDestroyThread_recomp(uint8_t *r,recomp_context *c){
 if(!c||r!=ram){fault=1;return;}int i=find((uint32_t)c->r4);
 if(i<0){fault=1;return;}state(i,DEAD);
 if(i==current)suspend();else{duck_fiber_destroy((unsigned)i);threads[i].state=FREE;}
}
void osSetThreadPri_recomp(uint8_t *r,recomp_context *c){
 if(!c||r!=ram||c->r5>255){fault=1;return;}int i=find((uint32_t)c->r4);
 if(i<0){fault=1;return;}threads[i].priority=(int)c->r5;
 if(threads[i].address)wr((threads[i].address&0x1fffffffu)+4,(uint32_t)c->r5);
 preempt();
}
void osYieldThread_recomp(uint8_t *r,recomp_context *c){
 (void)c;if(r!=ram||current<0){fault=1;return;}state(current,READY);suspend();
}
void pause_self(uint8_t *rdram){
 if(rdram!=ram||current<0){fault=1;return;}state(current,STOPPED);suspend();
}
