#pragma once
#include "duck_vita_live.h"
#include "duck_vita_dispatch.h"
#include "duck_vita_overlays.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Validated private function registry for the fiber scheduler.
 * Set before duck_vita_live_init; the caller retains registry storage.
 * Global because the existing guest fiber lookup ABI has no opaque argument.
 */
int duck_vita_live_registry_bind(const DuckVitaDispatchTable *table);
void duck_vita_live_registry_unbind(void);
recomp_func_t *duck_vita_live_registry_lookup(uint32_t guest_vram);
/* Initializes all live guest services against exactly one RDRAM allocation,
 * then boots a verified translated entry on a real guest fiber.
 */
int duck_vita_live_start_registered(DuckVitaLive *live,
 const DuckVitaDispatchTable *table,
 uint8_t *rdram,const uint8_t *rom,size_t rom_size,
 DuckVitaTickClock clock,void *clock_opaque,uint64_t vi_period,
 DuckVitaGraphicsSubmit renderer,uint32_t entry,uint32_t stack,
 uint32_t arg,int priority);
/* Production overlay startup. The caller supplies the verified private catalog
 * and allocator-observed resident code, and binds saves/audio before this call.
 * A failed startup is terminal for that runtime instance. */
int duck_vita_live_start_catalog(DuckVitaLive *live,const DuckCodeCatalog *catalog,
 uint8_t *rdram,const uint8_t *rom,size_t rom_size,
 DuckVitaTickClock clock,void *clock_opaque,uint64_t vi_period,
 DuckVitaGraphicsSubmit renderer,uint32_t entry,uint32_t stack,
 uint32_t arg,int priority);
#ifdef __cplusplus
}
#endif
