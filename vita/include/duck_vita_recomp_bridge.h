#pragma once
#include "duck_vita_scheduler.h"
#include "recomp.h"
/* Use the pinned N64Recomp ABI rather than a homegrown register layout.
 * Caller must dispatch a known, verified translated function. No automatic
 * guest-PC dispatcher, syscall implementation or MMIO translation is implied.
 */
typedef struct {
    recomp_context contexts[DUCK_SCHED_MAX_THREADS];
    unsigned char initialized[DUCK_SCHED_MAX_THREADS];
} DuckVitaRecompStore;
void duck_vita_recomp_store_init(DuckVitaRecompStore *store);
int duck_vita_recomp_set(DuckVitaRecompStore *store,const DuckVitaScheduler *sched,
                         unsigned id,const recomp_context *context);
int duck_vita_recomp_get(const DuckVitaRecompStore *store,const DuckVitaScheduler *sched,
                         unsigned id,recomp_context *context);
int duck_vita_recomp_call(DuckVitaRecompStore *store,DuckVitaScheduler *sched,
                          uint8_t *rdram,recomp_func_t *func);
