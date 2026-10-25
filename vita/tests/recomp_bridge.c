#include "duck_vita_recomp_bridge.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void translated_shape(uint8_t *ram,recomp_context *ctx) {
    ctx->r1+=ram[0];
    ctx->f0.u64=0x1122334455667788ull;
    ctx->hi=0x55aa;
    ctx->r0=1234;
}
int main(void) {
    DuckVitaScheduler s;DuckVitaRecompStore store;
    recomp_context ctx={0};uint8_t ram[8]={3};
    duck_vita_scheduler_init(&s);duck_vita_recomp_store_init(&store);
    assert(duck_vita_scheduler_add(&s,1,8)==0);
    assert(duck_vita_scheduler_add(&s,2,5)==0);
    assert(duck_vita_recomp_call(&store,&s,ram,translated_shape)==-3);
    ctx.r1=9;ctx.f1.u64=0xdeadbeef;ctx.mips3_float_mode=1;
    assert(duck_vita_recomp_set(&store,&s,1,&ctx)==0);
    assert(duck_vita_recomp_call(&store,&s,ram,translated_shape)==1);
    assert(duck_vita_recomp_get(&store,&s,1,&ctx)==0);
    assert(ctx.r0==0 && ctx.r1==12 && ctx.hi==0x55aa);
    assert(ctx.f0.u64==0x1122334455667788ull && ctx.f1.u64==0xdeadbeef);
    assert(ctx.mips3_float_mode==1);
    assert(duck_vita_scheduler_stop(&s,1)==0);
    assert(duck_vita_recomp_call(&store,&s,ram,translated_shape)==-3);
    assert(duck_vita_recomp_get(&store,&s,2,&ctx)==-1);
    puts("PASS: pinned recomp_context ABI-shaped invocation, FPR/GPR isolation, uninitialized guard");
    return 0;
}
