#include "duck_vita_bus.h"
#include <stdint.h>
#include <stddef.h>
static int translate(uint64_t vaddr, uint32_t *physical) {
    uint32_t low = (uint32_t)vaddr;
    uint64_t canonical = (low & 0x80000000u) ? (0xffffffff00000000ull | low) : low;
    if ((vaddr != canonical && vaddr != (uint64_t)low) ||
        ((low & 0xe0000000u) != 0x80000000u &&
         (low & 0xe0000000u) != 0xa0000000u))
        return DUCK_GUEST_RAM_UNMAPPED;
    *physical = low & 0x1fffffffu;
    if (*physical & 3) return DUCK_GUEST_RAM_ALIGN;
    return 0;
}
int duck_vita_bus_register(DuckVitaBus *bus, DuckVitaMmioRegion region) {
    if (!bus || !region.read32 || !region.write32 ||
        region.start >= region.end ||
        (region.start & 3) || (region.end & 3) ||
        bus->region_count >= DUCK_VITA_MAX_MMIO_REGIONS)
        return DUCK_GUEST_RAM_BAD_ARG;
    for (unsigned i = 0; i < bus->region_count; ++i)
        if (region.start < bus->regions[i].end &&
            bus->regions[i].start < region.end)
            return DUCK_GUEST_RAM_BAD_ARG;
    bus->regions[bus->region_count++] = region;
    return 0;
}
int duck_vita_bus_read32(DuckVitaBus *bus, uint64_t vaddr, uint32_t *value) {
    if (!bus || !value) return DUCK_GUEST_RAM_BAD_ARG;
    uint32_t physical;
    int rc = translate(vaddr, &physical);
    if (rc) return rc;
    if (physical < bus->rdram.size)
        return duck_vita_guest_read_be32(&bus->rdram, vaddr, value);
    for (unsigned i = 0; i < bus->region_count; ++i)
        if (physical >= bus->regions[i].start &&
            physical < bus->regions[i].end)
            return bus->regions[i].read32(bus->regions[i].opaque, physical, value);
    return DUCK_GUEST_RAM_UNMAPPED;
}
int duck_vita_bus_write32(DuckVitaBus *bus, uint64_t vaddr, uint32_t value) {
    if (!bus) return DUCK_GUEST_RAM_BAD_ARG;
    uint32_t physical;
    int rc = translate(vaddr, &physical);
    if (rc) return rc;
    if (physical < bus->rdram.size)
        return duck_vita_guest_write_be32(&bus->rdram, vaddr, value);
    for (unsigned i = 0; i < bus->region_count; ++i)
        if (physical >= bus->regions[i].start &&
            physical < bus->regions[i].end)
            return bus->regions[i].write32(bus->regions[i].opaque, physical, value);
    return DUCK_GUEST_RAM_UNMAPPED;
}
