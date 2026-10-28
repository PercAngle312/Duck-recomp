#include "duck_vita_sp.h"
#include "duck_vita_sp_events.h"
#include <assert.h>
#include <stdio.h>
static int sp_rc,dp_rc,render_rc,submits,sp_events,dp_events;
int duck_vita_os_event_emit(unsigned event){
 if(event==DUCK_VITA_OS_EVENT_SP){sp_events++;return sp_rc;}
 if(event==DUCK_VITA_OS_EVENT_DP){dp_events++;return dp_rc;}
 assert(0 && "unexpected event");return -1;
}
static int submit(uint32_t task){assert(task==0x80000100u);submits++;return render_rc;}
int main(void){
 DuckVitaSpEventState state;
 duck_vita_sp_events_reset(&state);
 DuckVitaSp sp;
 duck_vita_sp_init(&sp,submit,duck_vita_sp_events_complete,&state);
 assert(duck_vita_sp_start_graphics(&sp,0x80000100u)==0);
 assert(sp_events==1&&dp_events==1&&state.sp_delivered==1&&state.dp_delivered==1);
 render_rc=-7;
 assert(duck_vita_sp_start_graphics(&sp,0x80000100u)==-7);
 assert(sp_events==1&&dp_events==1&&state.render_failures==1);
 render_rc=0;sp_rc=-1;
 assert(duck_vita_sp_start_graphics(&sp,0x80000100u)==0);
 assert(sp_events==2&&dp_events==2&&state.sp_undelivered==1&&state.dp_delivered==2);
 sp_rc=0;dp_rc=-1;
 assert(duck_vita_sp_start_graphics(&sp,0x80000100u)==0);
 assert(state.sp_delivered==2&&state.dp_undelivered==1&&submits==4);
 render_rc=DUCK_SP_GRAPHICS_NO_DP;sp_rc=dp_rc=0;
 int dp_before=dp_events,sp_before=sp_events;
 assert(!duck_vita_sp_start_graphics(&sp,0x80000100u));
 assert(dp_events==dp_before&&sp_events==sp_before+1);
 assert(!duck_vita_sp_set_audio(&sp,submit,duck_vita_sp_events_complete_audio));
 render_rc=0;assert(!duck_vita_sp_start_audio(&sp,0x80000100u));
 assert(dp_events==dp_before&&state.audio_delivered==1);
 render_rc=-9;assert(duck_vita_sp_start_audio(&sp,0x80000100u)==-9);
 assert(dp_events==dp_before&&state.audio_failures==1);
 puts("SP/DP completion events delivered and failures accounted");
 return 0;
}
