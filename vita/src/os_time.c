/* Single guest CPU. Timer records live on the host; ABI fields remain in
 * word-swapped guest RAM. No intrusive libultra timer-list ABI is emulated.
 * Retire/rearm events BEFORE sending: a queue send may switch guest fibers. */
#include "duck_vita_time.h"
#include "duck_vita_os_io.h"
#include <string.h>
#include <fenv.h>
extern void osSendMesg_recomp(uint8_t*,recomp_context*);
enum { TIMERS=32,EVENT_BUDGET=128 };
static struct {uint32_t key,queue,message;uint64_t deadline,interval;int active;} timers[TIMERS];
static uint8_t *ram;
static DuckVitaTickClock clock_source;
static void *clock_opaque;
static uint64_t epoch,now,time_origin,last_count_tick,vi_period,next_vi,vi_remaining;
static uint32_t vi_generation,count,compare;
static int compare_valid,timer_pending,initialized,vi_active,error;
static DuckVitaTimeStats stats;
static int offset(uint64_t address,uint32_t length,uint32_t *out){
 uint32_t a=(uint32_t)address,p=a&0x1fffffffu;
 if(address!=(uint64_t)a&&address!=(uint64_t)(int64_t)(int32_t)a)return -1;
 if((a&0xe0000000u)!=0x80000000u&&(a&0xe0000000u)!=0xa0000000u)return -1;
 if((p&3)||p>0x800000u||length>0x800000u-p)return -1;
 *out=p;return 0;
}
static uint32_t rd(uint32_t p){uint32_t v;memcpy(&v,ram+p,4);return v;}
static void wr(uint32_t p,uint32_t value){memcpy(ram+p,&value,4);}
static void wr64(uint32_t p,uint64_t v){wr(p,(uint32_t)(v>>32));wr(p+4,(uint32_t)v);}
static uint64_t rd64(uint32_t p){return ((uint64_t)rd(p)<<32)|rd(p+4);}
void duck_cp0_reset(uint32_t value){count=value;compare=0;compare_valid=0;timer_pending=0;}
void duck_cp0_advance(uint32_t ticks){
 uint32_t distance=compare-count;
 if(compare_valid&&ticks&&distance&&distance<=ticks)timer_pending=1;
 count+=ticks;
}
uint32_t duck_cp0_timer_pending(void){return (uint32_t)timer_pending;}
static int update_clock(void){
 if(!clock_source)return -1;
 uint64_t raw=clock_source(clock_opaque);
 if(raw<epoch||raw-epoch<now){error=1;return -1;}
 now=raw-epoch;
 uint64_t delta=now-last_count_tick;
 if(delta>UINT32_MAX&&compare_valid)timer_pending=1;
 duck_cp0_advance((uint32_t)delta);last_count_tick=now;
 return 0;
}
int duck_vita_time_bind(uint8_t *rdram,DuckVitaTickClock clock,void *opaque,uint64_t period){
 if(!rdram||!clock||!period)return -1;
 ram=rdram;clock_source=clock;clock_opaque=opaque;epoch=clock(opaque);
 now=time_origin=last_count_tick=0;vi_period=period;next_vi=period;vi_remaining=0;
 initialized=vi_active=error=0;vi_generation=0;
 memset(timers,0,sizeof timers);memset(&stats,0,sizeof stats);duck_cp0_reset(0);
 return 0;
}
int duck_vita_time_error(void){return error;}
void duck_vita_time_stats(DuckVitaTimeStats *out){if(out)*out=stats;}
void osInitialize_recomp(uint8_t *r,recomp_context *c){
 if(!c||r!=ram||!clock_source){error=1;return;}
 if(!initialized){
  if(update_clock())return;
  time_origin=now;
  duck_vi_reset();duck_vi_config_reset();initialized=1;
 }
 c->status_reg|=0x20000000u;c->f_odd=&c->f0.u32h;
 if(fesetround(FE_TONEAREST))error=1;
}
void osGetCount_recomp(uint8_t *r,recomp_context *c){
 (void)r;if(!c)return;
 if(clock_source)update_clock();
 c->r2=(gpr)(int32_t)count;
}
void osGetTime_recomp(uint8_t *r,recomp_context *c){
 if(!c)return;
 if(r!=ram||update_clock()){c->r2=c->r3=0;error=1;return;}
 uint64_t elapsed=now-time_origin;
 c->r2=(gpr)(int32_t)(elapsed>>32);c->r3=(gpr)(int32_t)elapsed;
}
void __osSetCompare_recomp(uint8_t *r,recomp_context *c){
 (void)r;if(!c)return;
 compare=(uint32_t)c->r4;compare_valid=1;timer_pending=0;
}
void osSetTimer_recomp(uint8_t *r,recomp_context *c){
 uint32_t p,sp,q;if(!c)return;c->r2=(gpr)-1;
 if(r!=ram||!initialized||offset(c->r4,32,&p)||offset(c->r29,32,&sp)||update_clock())return;
 uint64_t delay=((uint64_t)(uint32_t)c->r6<<32)|(uint32_t)c->r7;
 uint64_t interval=rd64(sp+16);if(!delay)delay=interval;
 uint32_t queue=rd(sp+24),message=rd(sp+28);
 if(queue&&offset(queue,24,&q))return;
 if(delay>UINT64_MAX-now)return;
 int i;for(i=0;i<TIMERS;i++)if(timers[i].active&&timers[i].key==p)break;
 if(i==TIMERS)for(i=0;i<TIMERS&&timers[i].active;i++);
 if(i==TIMERS)return;
 timers[i].key=p;timers[i].queue=queue;timers[i].message=message;
 timers[i].deadline=now+delay;timers[i].interval=interval;timers[i].active=1;
 wr(p,0);wr(p+4,0);wr64(p+8,interval);wr64(p+16,delay);wr(p+24,queue);wr(p+28,message);
 c->r2=0;
}
void osStopTimer_recomp(uint8_t *r,recomp_context *c){
 uint32_t p;if(!c)return;c->r2=(gpr)-1;
 if(r!=ram||offset(c->r4,32,&p))return;
 for(int i=0;i<TIMERS;i++)if(timers[i].active&&timers[i].key==p){timers[i].active=0;c->r2=0;return;}
}
void osCreateViManager_recomp(uint8_t *r,recomp_context *c){
 if(!c||r!=ram||!initialized||c->r4>255||update_clock()){error=1;return;}
 if(vi_active)return;
 if(vi_period>UINT64_MAX-now){error=1;return;}
 vi_active=1;next_vi=now+vi_period;
}
static int deliver(uint32_t queue,uint32_t message){
 if(!queue)return 0;
 recomp_context c;c.r4=queue;c.r5=message;c.r6=0;
 osSendMesg_recomp(ram,&c);
 if(c.r2){stats.dropped_messages++;return 0;}
 return 1;
}
int duck_vita_time_poll(void){
 if(error||!ram||update_clock())return -1;
 if(!initialized)return 0;
 int work=0;
 while(work<EVENT_BUDGET){
  int earliest=-1;
  for(int i=0;i<TIMERS;i++)if(timers[i].active&&timers[i].deadline<=now&&
    (earliest<0||timers[i].deadline<timers[earliest].deadline))earliest=i;
  if(vi_active&&next_vi<=now&&(earliest<0||next_vi<timers[earliest].deadline)){
   if(vi_period>UINT64_MAX-next_vi){error=1;return -1;}
   next_vi+=vi_period;stats.retraces++;work++;
   duck_vi_retrace();
   DuckViConfig v;duck_vi_config_snapshot(&v);
   uint32_t generation=duck_vi_config_generation();
   if(generation!=vi_generation){vi_generation=generation;vi_remaining=v.retrace_count;}
   if(v.retrace_count&&vi_remaining&&!--vi_remaining){
    vi_remaining=v.retrace_count;
    int sent=deliver(v.event_queue,v.event_message);if(sent)stats.vi_messages++;
   }
   continue;
  }
  if(earliest<0)break;
  int i=earliest;uint32_t q=timers[i].queue,msg=timers[i].message;
  if(timers[i].interval){
   if(timers[i].interval>UINT64_MAX-timers[i].deadline){error=1;return -1;}
   timers[i].deadline+=timers[i].interval;
  }else timers[i].active=0;
  work++;int sent=deliver(q,msg);if(sent)stats.timer_messages++;
 }
 return work;
}
