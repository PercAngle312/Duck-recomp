#include "duck_vita_recomp_bridge.h"
#include <string.h>
static int slot(const DuckVitaScheduler *s,unsigned id) {
    if (!s || !id || s->count>DUCK_SCHED_MAX_THREADS) return -1;
    for(size_t i=0;i<s->count;i++) if(s->threads[i].id==id)return (int)i;
    return -1;
}
void duck_vita_recomp_store_init(DuckVitaRecompStore *store) {
    if(store) memset(store,0,sizeof(*store));
}
int duck_vita_recomp_set(DuckVitaRecompStore *store,const DuckVitaScheduler *s,
                         unsigned id,const recomp_context *ctx) {
    int i=slot(s,id);
    if(!store||!ctx||i<0)return -1;
    store->contexts[i]=*ctx;
    store->contexts[i].r0=0;
    store->initialized[i]=1;
    return 0;
}
int duck_vita_recomp_get(const DuckVitaRecompStore *store,const DuckVitaScheduler *s,
                         unsigned id,recomp_context *ctx) {
    int i=slot(s,id);
    if(!store||!ctx||i<0||!store->initialized[i])return -1;
    *ctx=store->contexts[i];
    ctx->r0=0;
    return 0;
}
int duck_vita_recomp_call(DuckVitaRecompStore *store,DuckVitaScheduler *s,
                          uint8_t *rdram,recomp_func_t *func) {
    if(!store||!s||!rdram||!func)return -1;
    int id=duck_vita_scheduler_next(s);
    if(id<0)return -2;
    int i=slot(s,(unsigned)id);
    if(i<0||!store->initialized[i])return -3;
    recomp_context *ctx=&store->contexts[i];
    ctx->r0=0;
    func(rdram,ctx);
    ctx->r0=0;
    return id;
}
