#pragma once
#include "duck_vita_scheduler.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Guest MIPS64 state snapshot. This is data-only; no Vita ARM context
 * switching or translated function invocation occurs here. */
typedef struct {
 uint64_t gpr[32],hi,lo;
 uint64_t pc;
 uint32_t status,cause;
} DuckVitaGuestContext;
typedef struct {
 DuckVitaGuestContext contexts[DUCK_SCHED_MAX_THREADS];
 uint8_t initialized[DUCK_SCHED_MAX_THREADS];
} DuckVitaContextStore;
void duck_vita_context_init(DuckVitaContextStore *store);
int duck_vita_context_save(DuckVitaContextStore *store,const DuckVitaScheduler *scheduler,
 unsigned thread_id,const DuckVitaGuestContext *state);
int duck_vita_context_load(const DuckVitaContextStore *store,const DuckVitaScheduler *scheduler,
 unsigned thread_id,DuckVitaGuestContext *state);
#ifdef __cplusplus
}
#endif
