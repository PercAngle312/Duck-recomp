// Test wrapper for the real os_time.c. The driver builds it with both versions.
#include "duck_vita_time.h"
#include <string.h>
static uint8_t memory[0x800000];static uint64_t tick,events,hash;static int action;
static recomp_context context;static DuckViConfig vi;static unsigned generation;
static uint64_t clock_tick(void *unused){(void)unused;return tick;}
void duck_vi_reset(void){}
void duck_vi_retrace(void){}
void duck_vi_config_reset(void){memset(&vi,0,sizeof vi);generation=0;}
int duck_vi_config_snapshot(DuckViConfig *out){*out=vi;return 0;}
uint32_t duck_vi_config_generation(void){return generation;}
static void wr(unsigned p,uint32_t v){memcpy(memory+p,&v,4);}
static void set_timer(recomp_context *c,unsigned slot,uint64_t delay,uint64_t interval){
 c->r4=0x80004000u+slot*32;c->r6=delay>>32;c->r7=(uint32_t)delay;c->r29=0x80003000u;
 wr(0x3010,(uint32_t)(interval>>32));wr(0x3014,(uint32_t)interval);wr(0x3018,0x80002000);wr(0x301c,slot*17+1);
 osSetTimer_recomp(memory,c);
}
void osSendMesg_recomp(uint8_t *r,recomp_context *c){
 (void)r;++events;hash=(hash^(uint32_t)c->r4)*1099511628211ull;hash=(hash^(uint32_t)c->r5)*1099511628211ull;
 if(action){int a=action;action=0;recomp_context nested={0};
  if(a==1){nested.r4=0x80004000;osStopTimer_recomp(memory,&nested);}
  else if(a==2)set_timer(&nested,1,0,0);
  else duck_vita_time_poll();
 }
 c->r2=((uint32_t)c->r5%13)==0?(gpr)-1:0;
}
void duck_test_time_reset(void){
 memset(memory,0,sizeof memory);memset(&context,0,sizeof context);tick=events=hash=0;action=0;
 duck_vita_time_bind(memory,clock_tick,0,1000);osInitialize_recomp(memory,&context);
}
uint64_t duck_test_time_step(unsigned op,unsigned slot,uint64_t a,uint64_t b){
 context.r2=context.r3=0;
 switch(op){
 case 0:set_timer(&context,slot,a,b);break;
 case 1:context.r4=0x80004000u+slot*32;osStopTimer_recomp(memory,&context);break;
 case 2:tick+=a;context.r2=(gpr)duck_vita_time_poll();break;
 case 3:osCreateViManager_recomp(memory,&context);break;
 case 4:vi.event_queue=0x80002000;vi.event_message=slot;vi.retrace_count=(uint32_t)a;++generation;break;
 case 5:osGetTime_recomp(memory,&context);break;
 case 6:osGetCount_recomp(memory,&context);break;
 case 7:context.r4=a;__osSetCompare_recomp(memory,&context);break;
 case 8:action=(int)a;break;
 }
 uint64_t digest=hash^events^context.r2^(context.r3*65537u)^((uint64_t)duck_cp0_timer_pending()<<32);
 const uint8_t *p=memory+0x3000;
 for(unsigned i=0;i<0x1800;i++)digest=(digest^p[i])*1099511628211ull;
 DuckVitaTimeStats stats;duck_vita_time_stats(&stats);p=(const uint8_t*)&stats;
 for(unsigned i=0;i<sizeof stats;i++)digest=(digest^p[i])*1099511628211ull;
 return digest^(uint64_t)duck_vita_time_error();
}
