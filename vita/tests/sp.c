#include "duck_vita_sp.h"
#include <assert.h>
#include <stdio.h>
static int calls,events,ok,forced;
static int submit(uint32_t addr){assert(addr==0x80004000u);calls++;return forced;}
static void complete(void *unused,int success){(void)unused;events++;ok=success;}
int main(void){
 DuckVitaSp sp;
 duck_vita_sp_init(&sp,submit,complete,0);
 assert(duck_vita_sp_start_graphics(&sp,0)==DUCK_SP_BAD_ARGUMENT);
 assert(duck_vita_sp_start_graphics(&sp,3)==DUCK_SP_BAD_ARGUMENT);
 assert(duck_vita_sp_start_graphics(&sp,0x80004000u)==0);
 assert(calls==1&&events==1&&ok==1&&sp.completed==1&&!sp.busy);
 forced=-8;
 assert(duck_vita_sp_start_graphics(&sp,0x80004000u)==-8);
 assert(calls==2&&events==2&&ok==0&&sp.failed==1);
 sp.busy=1;
 assert(duck_vita_sp_start_graphics(&sp,0x80004000u)==DUCK_SP_BUSY);
 sp.busy=0;sp.submit=0;
 assert(duck_vita_sp_start_graphics(&sp,0x80004000u)==DUCK_SP_RENDERER_UNAVAILABLE);
 puts("SP graphics task dispatch and completion semantics passed");
 return 0;
}
