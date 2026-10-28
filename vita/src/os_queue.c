/* Guest ring queues. Optional fiber hooks preserve blocked native call stacks.
 * Without an active guest fiber a blocking miss fails explicitly. */
#include "duck_vita_threads.h"
#include <stdint.h>
#define RAM_BYTES 0x800000u
static DuckQueueWait wait_hook;
static DuckQueueWake wake_hook;
static void (*preempt_hook)(void);
void duck_vita_os_queue_hooks(DuckQueueWait wait,DuckQueueWake wake,void (*preempt)(void)){wait_hook=wait;wake_hook=wake;preempt_hook=preempt;}
static int offset(uint64_t v,uint32_t len,uint32_t *out){
 if(v!=(uint64_t)(uint32_t)v && v!=(uint64_t)(int64_t)(int32_t)v)return -1;
 uint32_t a=(uint32_t)v, p=a&0x1fffffffu;
 if ((a&0xe0000000u)!=0x80000000u && (a&0xe0000000u)!=0xa0000000u) return -1;
 if((p&3u)||p>RAM_BYTES||len>RAM_BYTES-p) return -1;
 *out=p;return 0;
}
static uint32_t rd(const uint8_t *m,uint32_t p){return (uint32_t)m[p]|((uint32_t)m[p+1]<<8)|((uint32_t)m[p+2]<<16)|((uint32_t)m[p+3]<<24);}
static void wr(uint8_t *m,uint32_t p,uint32_t x){m[p]=(uint8_t)x;m[p+1]=(uint8_t)(x>>8);m[p+2]=(uint8_t)(x>>16);m[p+3]=(uint8_t)(x>>24);}
static int qvalid(uint8_t *ram,uint64_t v,uint32_t *qp,uint32_t *mp,uint32_t *count,uint32_t *first,uint32_t *cap){
 if(!ram||offset(v,24,qp))return -1;
 uint32_t valid=rd(ram,*qp+8),start=rd(ram,*qp+12),n=rd(ram,*qp+16),msgs=rd(ram,*qp+20);
 if(!n||n>RAM_BYTES/4u||valid>n||start>=n||offset(msgs,n*4u,mp))return -1;
 *count=valid;*first=start;*cap=n;return 0;
}
static void send(uint8_t *rdram,recomp_context *ctx,int jam){
 if(!ctx)return;
 uint32_t qp,mp,cnt,first,cap;
 for(;;){
  if(qvalid(rdram,ctx->r4,&qp,&mp,&cnt,&first,&cap)){ctx->r2=(uint64_t)-1;return;}
  if(cnt<cap)break;
  if(ctx->r6!=1||!wait_hook||wait_hook((uint32_t)ctx->r4,1)){ctx->r2=(uint64_t)-1;return;}
 }
 uint32_t index=jam?(first==0?cap-1:first-1):(first+cnt>=cap?first+cnt-cap:first+cnt);
 wr(rdram,mp+index*4u,(uint32_t)ctx->r5);
 if(jam)wr(rdram,qp+12,index);
 wr(rdram,qp+8,cnt+1);
 ctx->r2=0;
 if(wake_hook)wake_hook((uint32_t)ctx->r4,0);
 if(preempt_hook)preempt_hook();
}
void osSendMesg_recomp(uint8_t *rdram,recomp_context *ctx){send(rdram,ctx,0);}
void osJamMesg_recomp(uint8_t *rdram,recomp_context *ctx){send(rdram,ctx,1);}
void osRecvMesg_recomp(uint8_t *rdram,recomp_context *ctx){
 if(!ctx)return;
 uint32_t qp,mp,cnt,first,cap;
 for(;;){
  if(qvalid(rdram,ctx->r4,&qp,&mp,&cnt,&first,&cap)){ctx->r2=(uint64_t)-1;return;}
  if(cnt)break;
  if(ctx->r6!=1||!wait_hook||wait_hook((uint32_t)ctx->r4,0)){ctx->r2=(uint64_t)-1;return;}
 }
 if(ctx->r5){uint32_t dst;if(offset(ctx->r5,4,&dst)){ctx->r2=(uint64_t)-1;return;}wr(rdram,dst,rd(rdram,mp+first*4u));}
 wr(rdram,qp+12,first+1==cap?0:first+1);
 wr(rdram,qp+8,cnt-1);ctx->r2=0;
 if(wake_hook)wake_hook((uint32_t)ctx->r4,1);
 if(preempt_hook)preempt_hook();
}
