#include "duck_vita_executor.h"
#include <assert.h>
#include <stdio.h>
typedef struct {unsigned calls;uint64_t previous;} Trace;
static int mock_step(DuckVitaGuestContext *ctx,void *opaque) {
 Trace *t=opaque;t->calls++;t->previous=ctx->gpr[1];
 ctx->gpr[1]+=5;ctx->gpr[0]=0xBAD;ctx->pc+=4;
 return 0;
}
int main(void) {
 DuckVitaScheduler s;DuckVitaContextStore store;DuckVitaGuestContext state={0};Trace trace={0};
 duck_vita_scheduler_init(&s);duck_vita_context_init(&store);
 assert(duck_vita_scheduler_add(&s,1,10)==0);
 assert(duck_vita_scheduler_add(&s,2,20)==0);
 assert(duck_vita_execute_step(&s,&store,mock_step,&trace)==DUCK_EXEC_NO_CONTEXT);
 assert(trace.calls==0);
 state.gpr[1]=100;state.pc=0x80001000u;
 assert(duck_vita_context_save(&store,&s,2,&state)==0);
 assert(duck_vita_execute_step(&s,&store,mock_step,&trace)==0);
 assert(trace.calls==1 && trace.previous==100);
 assert(duck_vita_context_load(&store,&s,2,&state)==0);
 assert(state.gpr[1]==105 && state.gpr[0]==0 && state.pc==0x80001004u);
 assert(duck_vita_scheduler_stop(&s,2)==0);
 state.gpr[1]=7;state.pc=0x80002000u;
 assert(duck_vita_context_save(&store,&s,1,&state)==0);
 assert(duck_vita_execute_step(&s,&store,mock_step,&trace)==0);
 assert(trace.calls==2 && trace.previous==7);
 assert(duck_vita_context_load(&store,&s,1,&state)==0);
 assert(state.gpr[1]==12 && state.pc==0x80002004u);
 puts("PASS: context-backed dispatch, no uninitialized call, thread isolation, PC/GPR persistence");
 return 0;
}
