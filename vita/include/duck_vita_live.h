#pragma once
#include <stddef.h>
#include <stdint.h>
#include "duck_vita_threads.h"
#include "duck_vita_sp_runtime.h"
#include "duck_vita_time.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Production guest services: one RDRAM, one ROM, one guest CPU/fiber host.
 * Call after Fast3D has initialized against exactly this RDRAM pointer.
 * Dispatch uses real translated functions supplied by the private registry.
 */
typedef struct {
 uint8_t *rdram;
 const uint8_t *rom;
 size_t rom_size;
 DuckVitaSp sp;
 DuckVitaSpEventState graphics_events;
 unsigned pi_transfers;
 unsigned scheduler_polls;
 int initialized;
} DuckVitaLive;
int duck_vita_live_init(DuckVitaLive *live,uint8_t *rdram,
 const uint8_t *rom,size_t rom_size,DuckThreadLookup lookup,
 DuckVitaTickClock clock,void *clock_opaque,uint64_t vi_period,
 DuckVitaGraphicsSubmit renderer);
int duck_vita_live_boot(DuckVitaLive *live,uint32_t entry,uint32_t stack,
 uint32_t arg,int priority);
int duck_vita_live_run(DuckVitaLive *live,unsigned dispatch_budget);
#ifdef __cplusplus
}
#endif
