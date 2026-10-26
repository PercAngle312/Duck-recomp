#include "duck_vita_sp.h"
#include "duck_vita_sp_os.h"
#include "recomp.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void osSpTaskStartGo_recomp(uint8_t*,recomp_context*);
extern void osSpTaskLoad_recomp(uint8_t*,recomp_context*);
extern void osSpTaskYield_recomp(uint8_t*,recomp_context*);
extern void osSpTaskYielded_recomp(uint8_t*,recomp_context*);
static unsigned submitted, events;
static int submit(uint32_t address){assert(address==0x80000100u);submitted++;return 0;}
static void complete(void *unused,int success){(void)unused;assert(success);events++;}
int main(void){
 uint8_t *ram=calloc(1,0x800000u);assert(ram);
 DuckVitaSp sp;duck_vita_sp_init(&sp,submit,complete,0);
 duck_vita_sp_os_bind(&sp,ram);
 recomp_context ctx;memset(&ctx,0,sizeof(ctx));ctx.r4=0x80000100u;
 osSpTaskStartGo_recomp(ram,&ctx);
 assert(submitted==0&&duck_vita_sp_os_last_result()==DUCK_SP_BAD_ARGUMENT);
 ram[0x100]=1;
 osSpTaskLoad_recomp(ram,&ctx);
 assert(duck_vita_sp_os_last_result()==0);
 osSpTaskYielded_recomp(ram,&ctx);
 assert(ctx.r2==0&&duck_vita_sp_os_last_result()==0);
 osSpTaskYield_recomp(ram,&ctx);
 assert(duck_vita_sp_os_last_result()==0);
 osSpTaskStartGo_recomp(ram,&ctx);
 assert(submitted==1&&events==1&&duck_vita_sp_os_last_result()==0);
 ctx.r4=0x80000101u;osSpTaskStartGo_recomp(ram,&ctx);
 assert(submitted==1&&duck_vita_sp_os_last_result()==DUCK_SP_BAD_ARGUMENT);
 ctx.r4=0x80000100u;osSpTaskStartGo_recomp(ram+4,&ctx);
 assert(submitted==1);
 duck_vita_sp_os_bind(0,0);osSpTaskStartGo_recomp(ram,&ctx);assert(submitted==1);
 free(ram);puts("osSpTaskStartGo guest dispatch passed");return 0;
}
