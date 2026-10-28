#pragma once
#include "duck_vita_guest_memory.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Bus distinguishes memory, MMIO and invalid regions; MMIO requires a
 * registered device rather than silently aliasing RDRAM. */
typedef int (*DuckVitaMmioRead)(void *opaque, uint32_t physical, uint32_t *value);
typedef int (*DuckVitaMmioWrite)(void *opaque, uint32_t physical, uint32_t value);
typedef struct {
    uint32_t start, end; /* physical address interval [start,end) */
    DuckVitaMmioRead read32;
    DuckVitaMmioWrite write32;
    void *opaque;
} DuckVitaMmioRegion;
enum { DUCK_VITA_MAX_MMIO_REGIONS = 16 };
typedef struct {
    DuckVitaRdram rdram;
    DuckVitaMmioRegion regions[DUCK_VITA_MAX_MMIO_REGIONS];
    unsigned region_count;
} DuckVitaBus;
int duck_vita_bus_register(DuckVitaBus *bus, DuckVitaMmioRegion region);
int duck_vita_bus_read32(DuckVitaBus *bus, uint64_t vaddr, uint32_t *value);
int duck_vita_bus_write32(DuckVitaBus *bus, uint64_t vaddr, uint32_t value);
#ifdef __cplusplus
}
#endif
