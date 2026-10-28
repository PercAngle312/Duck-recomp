#include "duck_vita_context.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 DuckVitaScheduler scheduler;DuckVitaContextStore store;
 DuckVitaGuestContext one={0},two={0},loaded={0};
 duck_vita_scheduler_init(&scheduler);
 duck_vita_context_init(&store);
 assert(duck_vita_scheduler_add(&scheduler,1,1)==0);
 assert(duck_vita_scheduler_add(&scheduler,2,9)==0);
 assert(duck_vita_context_load(&store,&scheduler,1,&loaded)==-1);
 one.gpr[0]=0xffff;one.gpr[1]=0x123456789abcdef0ull;one.pc=0xffffffff80001000ull;one.status=0x401;
 two.gpr[3]=0xface;two.pc=0xffffffff80002000ull;two.cause=0x400;
 assert(duck_vita_context_save(&store,&scheduler,1,&one)==0);
 assert(duck_vita_context_save(&store,&scheduler,2,&two)==0);
 assert(duck_vita_context_load(&store,&scheduler,1,&loaded)==0);
 assert(loaded.gpr[0]==0 && loaded.gpr[1]==one.gpr[1] && loaded.pc==one.pc && loaded.status==one.status);
 assert(duck_vita_context_load(&store,&scheduler,2,&loaded)==0);
 assert(loaded.gpr[3]==two.gpr[3] && loaded.pc==two.pc && loaded.cause==two.cause);
 assert(duck_vita_context_load(&store,&scheduler,99,&loaded)==-1);
 puts("PASS: isolated MIPS64 contexts, GPR0 invariant, PC and CP0 snapshot roundtrip");
 return 0;
}
