/* Reproducible source for the four early callbacks recovered only as machine
 * code in the historical resident object. No guest thread wait lists or TLB
 * emulation are supplied here. Matches the nonblocking guest queue ABI. */
#include "recomp.h"
#include <stdint.h>
void __osDisableInt_recomp(uint8_t *rdram,recomp_context *ctx) {
    (void)rdram;if(!ctx)return;
    ctx->r2=ctx->status_reg&1u;ctx->status_reg&=~1u;
}
void __osRestoreInt_recomp(uint8_t *rdram,recomp_context *ctx) {
    (void)rdram;if(ctx)ctx->status_reg|=(uint32_t)ctx->r4&1u;
}
void osVirtualToPhysical_recomp(uint8_t *rdram,recomp_context *ctx) {
    (void)rdram;if(!ctx)return;
    uint32_t address=(uint32_t)ctx->r4,segment=address&0xe0000000u;
    ctx->r2=(segment==0x80000000u||segment==0xa0000000u) ? address&0x1fffffffu : 0xffffffffu;
}
static int span(uint64_t address,uint32_t size,uint32_t *offset) {
    uint32_t a=(uint32_t)address,p=a&0x1fffffffu;
    if(address!=(uint64_t)a && address!=(uint64_t)(int64_t)(int32_t)a)return -1;
    if((a&0xe0000000u)!=0x80000000u && (a&0xe0000000u)!=0xa0000000u)return -1;
    if((p&3u)||p>0x800000u||size>0x800000u-p)return -1;
    *offset=p;return 0;
}
static void wr(uint8_t *r,uint32_t p,uint32_t value) {
    for(unsigned i=0;i<4;i++)r[p+i]=(uint8_t)(value>>(8*i));
}
void osCreateMesgQueue_recomp(uint8_t *rdram,recomp_context *ctx) {
    uint32_t q,buffer;
    if(!rdram||!ctx||!ctx->r6||ctx->r6>0x200000u||span(ctx->r4,24,&q)||
       span(ctx->r5,(uint32_t)ctx->r6*4,&buffer))return;
    if(q<buffer+(uint32_t)ctx->r6*4 && buffer<q+24)return;
    wr(rdram,q,0);wr(rdram,q+4,0);wr(rdram,q+8,0);wr(rdram,q+12,0);
    wr(rdram,q+16,(uint32_t)ctx->r6);wr(rdram,q+20,(uint32_t)ctx->r5);
}
