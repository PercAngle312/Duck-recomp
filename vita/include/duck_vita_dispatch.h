#pragma once
#include "duck_vita_recomp_bridge.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Explicit static function registry for privately generated translated code.
 * Never guesses a function for an unregistered guest VRAM address.
 */
typedef struct { uint32_t vram; recomp_func_t *function; } DuckVitaDispatchEntry;
typedef struct { const DuckVitaDispatchEntry *entries; size_t count; } DuckVitaDispatchTable;
enum { DUCK_DISPATCH_UNKNOWN=-4, DUCK_DISPATCH_BAD_TABLE=-5 };
int duck_vita_dispatch_validate(const DuckVitaDispatchTable *table);
int duck_vita_dispatch_vram(DuckVitaRecompStore *store,DuckVitaScheduler *scheduler,
    uint8_t *rdram,const DuckVitaDispatchTable *table,uint32_t vram);
#ifdef __cplusplus
}
#endif
