#include "../src/live.c"
#include <psp2/ctrl.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static unsigned native_reads,events,ai_services,time_services;
static int result=1;
static SceCtrlData pad;
int sceCtrlSetSamplingMode(int mode){assert(mode==SCE_CTRL_MODE_ANALOG);return 0;}
int sceCtrlPeekBufferPositive(int port,SceCtrlData *out,int count){assert(!port&&count==1);native_reads++;*out=pad;return result;}
int duck_vita_sp_os_last_result(void){return 0;}
int duck_vita_os_pi_service(void){return 0;}
int duck_vita_ai_service(void){ai_services++;return 0;}
int duck_vita_time_poll(void){time_services++;return 0;}
int duck_vita_os_event_emit(unsigned event){assert(event==5);events++;return 0;}
int main(void){
 DuckVitaLive live={0};live.initialized=1;active_live=&live;duck_pad_reset();
 pad.lx=pad.ly=pad.rx=pad.ry=128;
 for(unsigned i=0;i<10000;i++)assert(!service());
 assert(native_reads==1&&ai_services==10000&&time_services==10000);
 uint8_t *ram=calloc(1,0x800000);assert(ram);recomp_context c={0};c.r4=0x80001000;
 pad.buttons=SCE_CTRL_CROSS;osContStartReadData_recomp(ram,&c);assert(!c.r2&&duck_pad_needs_poll());
 assert(!service()&&native_reads==2&&events==1&&!duck_pad_needs_poll());
 c.r4=0x80003000;osContGetReadData_recomp(ram,&c);assert(!c.r2&&ram[0x3000^3]==0x80);
 for(unsigned i=0;i<10000;i++)assert(!service());assert(native_reads==2&&events==1);
 // One native read per guest query, preserving normal SI delivery.
 c.r4=0x80001000;osContStartQuery_recomp(ram,&c);assert(!c.r2);assert(!service()&&native_reads==3&&events==2);
 // Host failure remains a runtime failure; don't invent a completion.
 osContStartReadData_recomp(ram,&c);assert(!c.r2);result=-1;
 assert(service()==-1&&native_reads==4&&events==2);
 free(ram);puts("PASS 20000 idle scheduler services: two native reads; SI reads/queries and failure preserved; audio/timers still serviced");
}
