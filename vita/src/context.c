#include "duck_vita_context.h"
#include <string.h>
static int index_for(const DuckVitaScheduler *scheduler,unsigned id) {
 if(!scheduler||!id||scheduler->count>DUCK_SCHED_MAX_THREADS)return -1;
 for(size_t i=0;i<scheduler->count;i++)
  if(scheduler->threads[i].id==id)return (int)i;
 return -1;
}
void duck_vita_context_init(DuckVitaContextStore *store) {
 if(store)memset(store,0,sizeof(*store));
}
int duck_vita_context_save(DuckVitaContextStore *store,const DuckVitaScheduler *scheduler,
 unsigned id,const DuckVitaGuestContext *state) {
 int i=index_for(scheduler,id);
 if(!store||!state||i<0)return -1;
 store->contexts[i]=*state;
 store->contexts[i].gpr[0]=0;
 store->initialized[i]=1;
 return 0;
}
int duck_vita_context_load(const DuckVitaContextStore *store,const DuckVitaScheduler *scheduler,
 unsigned id,DuckVitaGuestContext *state) {
 int i=index_for(scheduler,id);
 if(!store||!state||i<0||!store->initialized[i])return -1;
 *state=store->contexts[i];
 state->gpr[0]=0;
 return 0;
}
