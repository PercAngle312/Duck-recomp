#include "duck_vita_game_loop.h"
#include "duck_vita_sp_os.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned game_calls, graphics_calls, event_sp, event_dp;
static uint8_t *expected_ram;
static void game(uint8_t *ram,recomp_context *ctx) {
 assert(ram==expected_ram);game_calls++;
 ctx->r4=0x80000100u;
 extern void osSpTaskStartGo_recomp(uint8_t *,recomp_context *);
 osSpTaskStartGo_recomp(ram,ctx);
}
static int draw(uint32_t task){assert(task==0x80000100u);graphics_calls++;return 0;}
int duck_vita_os_event_emit(unsigned event){
 if(event==4)event_sp++;
 else if(event==9)event_dp++;
 else assert(0);
 return 0;
}
int main(void){
 uint8_t *ram=calloc(1,0x800000u);assert(ram);expected_ram=ram;
 ram[0x100]=1;
 DuckVitaScheduler sched;duck_vita_scheduler_init(&sched);
 assert(duck_vita_scheduler_add(&sched,7,10)==0);
 DuckVitaRecompStore contexts;duck_vita_recomp_store_init(&contexts);
 recomp_context ctx;memset(&ctx,0,sizeof(ctx));
 assert(duck_vita_recomp_set(&contexts,&sched,7,&ctx)==0);
 DuckVitaDispatchEntry entry={0x80001000u,game};
 DuckVitaDispatchTable map={&entry,1};
 DuckVitaGameLoop loop;
 assert(duck_vita_game_loop_attach(&loop,&sched,&contexts,&map,ram,draw)==0);
 assert(duck_vita_game_loop_step(&loop,0x80001000u)==7);
 assert(game_calls==1&&graphics_calls==1&&event_sp==1&&event_dp==1);
 assert(loop.sp.completed==1&&loop.events.sp_delivered==1&&loop.events.dp_delivered==1);
 assert(duck_vita_game_loop_step(&loop,0x80001200u)==DUCK_DISPATCH_UNKNOWN);
 assert(graphics_calls==1);
 duck_vita_game_loop_detach(&loop);
 assert(duck_vita_game_loop_step(&loop,0x80001000u)==-1);
 free(ram);
 puts("PASS: scheduled translated guest calls graphics SP and SP/DP completion");
 return 0;
}
