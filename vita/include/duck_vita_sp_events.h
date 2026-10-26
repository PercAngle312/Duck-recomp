#pragma once
#include "duck_vita_sp.h"
#ifdef __cplusplus
extern "C" {
#endif
/* libultra: OS_EVENT_SP = 4, OS_EVENT_DP = 9.
 * Install this callback when initializing the runtime's DuckVitaSp.
 * A completed synchronous graphics task sends SP, and DP only with FullSync, through the
 * existing guest OS event registration and message queue implementation.
 */
enum { DUCK_VITA_OS_EVENT_SP = 4, DUCK_VITA_OS_EVENT_DP = 9 };
typedef struct {
 unsigned sp_delivered, dp_delivered;
 unsigned sp_undelivered, dp_undelivered;
 unsigned render_failures;
 unsigned audio_delivered,audio_undelivered,audio_failures;
} DuckVitaSpEventState;
void duck_vita_sp_events_reset(DuckVitaSpEventState *state);
void duck_vita_sp_events_complete(void *opaque, int success);
void duck_vita_sp_events_complete_audio(void *opaque,int success);
#ifdef __cplusplus
}
#endif
