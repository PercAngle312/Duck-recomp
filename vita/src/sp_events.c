/* Guest SP/DP event handoff for synchronously completed graphics tasks.
 * No event is sent for a failed renderer submission. An absent registration,
 * or a full queue, is recorded explicitly instead of claiming delivery.
 */
#include "duck_vita_sp_events.h"
#include <string.h>
extern int duck_vita_os_event_emit(unsigned event);
void duck_vita_sp_events_reset(DuckVitaSpEventState *state){
 if(state)memset(state,0,sizeof(*state));
}
void duck_vita_sp_events_complete(void *opaque,int success){
 DuckVitaSpEventState *state=(DuckVitaSpEventState*)opaque;
 if(!state)return;
 if(!success){state->render_failures++;return;}
 if(duck_vita_os_event_emit(DUCK_VITA_OS_EVENT_SP)==0)state->sp_delivered++;
 else state->sp_undelivered++;
 if(success==2)return; /* Rendered graphics without an RDP FullSync. */
 if(duck_vita_os_event_emit(DUCK_VITA_OS_EVENT_DP)==0)state->dp_delivered++;
 else state->dp_undelivered++;
}

void duck_vita_sp_events_complete_audio(void *opaque,int success){
 DuckVitaSpEventState *state=opaque;if(!state)return;
 if(!success){state->audio_failures++;return;}
 if(duck_vita_os_event_emit(DUCK_VITA_OS_EVENT_SP)==0)state->audio_delivered++;
 else state->audio_undelivered++;
}
