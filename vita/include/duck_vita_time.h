#pragma once
#include "recomp.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef uint64_t (*DuckVitaTickClock)(void *opaque);
typedef struct {uint32_t mode_address,special_features,event_queue,event_message,retrace_count;} DuckViConfig;
typedef struct {uint64_t timer_messages,vi_messages,dropped_messages,retraces;} DuckVitaTimeStats;
/* Clock returns monotonic N64 Count ticks (46,875,000/s). A configured VI
 * period is an HLE timing choice, not a cycle-accurate video mode decoder. */
int duck_vita_time_bind(uint8_t *rdram,DuckVitaTickClock clock,void *opaque,uint64_t vi_period);
int duck_vita_time_poll(void);
int duck_vita_time_error(void);
void duck_vita_time_stats(DuckVitaTimeStats *out);
uint64_t duck_vita_clock_ticks(void *unused);
void osInitialize_recomp(uint8_t*,recomp_context*);
void osSetTimer_recomp(uint8_t*,recomp_context*);
void osStopTimer_recomp(uint8_t*,recomp_context*);
void osCreateViManager_recomp(uint8_t*,recomp_context*);
void osGetCount_recomp(uint8_t*,recomp_context*);
void osGetTime_recomp(uint8_t*,recomp_context*);
void __osSetCompare_recomp(uint8_t*,recomp_context*);
void duck_cp0_reset(uint32_t count);
void duck_cp0_advance(uint32_t ticks);
uint32_t duck_cp0_timer_pending(void);
void duck_vi_config_reset(void);
int duck_vi_config_snapshot(DuckViConfig*);
uint32_t duck_vi_config_generation(void);
void duck_vi_reset(void);
void duck_vi_retrace(void);

#ifdef __cplusplus
}
#endif
