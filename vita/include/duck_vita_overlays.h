#pragma once
#include "recomp.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint32_t offset;recomp_func_t *function;} DuckCodeFunction;
typedef struct {uint32_t id,initialized,bss,section,default_base;const DuckCodeFunction *functions;size_t function_count;} DuckCodeModule;
typedef struct {const DuckCodeFunction *resident;size_t resident_count;const DuckCodeModule *modules;size_t module_count;uint32_t resident_begin,resident_end;int allocation_observer;} DuckCodeCatalog;
int duck_vita_overlays_init(uint8_t *rdram,const DuckCodeCatalog *catalog);
int duck_vita_overlay_load(uint32_t id,uint32_t base,uint32_t allocation,uint32_t initialized,uint32_t bss);
int duck_vita_overlay_free(uint32_t allocation);
/* Call on EVERY allocation reuse, before new contents are written. This makes
 * retained destructor dispatch expire as soon as guest code memory is reused. */
int duck_vita_overlay_reuse(uint32_t allocation,uint32_t size);
recomp_func_t *duck_vita_overlay_find(uint32_t pc);
void duck_vita_after_allocation(uint8_t *rdram,uint32_t allocation,uint32_t size);
void duck_register_loaded_module(uint8_t*,recomp_context*);
void duck_before_free(uint8_t*,recomp_context*);

#ifdef __cplusplus
}
#endif
