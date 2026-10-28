#include "duck_vita_dispatch.h"
#include <assert.h>
#include <stdio.h>
static void f1(uint8_t *r,recomp_context *c){c->r1+=r[0];}
static void f2(uint8_t *r,recomp_context *c){c->r2+=r[1];}
int main(void) {
    DuckVitaScheduler s;DuckVitaRecompStore store;recomp_context state={0};
    uint8_t rdram[4]={5,7};
    const DuckVitaDispatchEntry entries[]={{0x80001000u,f1},{0x80002000u,f2}};
    DuckVitaDispatchTable table={entries,2};
    duck_vita_scheduler_init(&s);duck_vita_recomp_store_init(&store);
    assert(duck_vita_scheduler_add(&s,1,8)==0);
    assert(duck_vita_recomp_set(&store,&s,1,&state)==0);
    assert(duck_vita_dispatch_validate(&table)==0);
    assert(duck_vita_dispatch_vram(&store,&s,rdram,&table,0x80000000u)==DUCK_DISPATCH_UNKNOWN);
    assert(duck_vita_dispatch_vram(&store,&s,rdram,&table,0x80001000u)==1);
    assert(duck_vita_dispatch_vram(&store,&s,rdram,&table,0x80002000u)==1);
    assert(duck_vita_recomp_get(&store,&s,1,&state)==0);
    assert(state.r1==5 && state.r2==7);
    const DuckVitaDispatchEntry duplicate[]={{0x80001000u,f1},{0x80001000u,f2}};
    table.entries=duplicate;
    assert(duck_vita_dispatch_validate(&table)==DUCK_DISPATCH_BAD_TABLE);
    assert(duck_vita_dispatch_vram(&store,&s,rdram,&table,0x80001000u)==DUCK_DISPATCH_BAD_TABLE);
    puts("PASS: pinned ABI function table, exact VRAM dispatch, unmapped rejection, duplicate guard");
    return 0;
}
