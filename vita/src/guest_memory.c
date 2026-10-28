#include "duck_vita_guest_memory.h"
#include <stdint.h>
#include <stddef.h>
int duck_vita_guest_span(const DuckVitaRdram *ram, uint64_t vaddr,
                         size_t length, size_t alignment, uint8_t **out) {
    if (out) *out = NULL;
    if (!ram || !ram->bytes || !out || !alignment ||
        (alignment & (alignment - 1)) || ram->size > DUCK_VITA_RDRAM_BYTES)
        return DUCK_GUEST_RAM_BAD_ARG;
    uint32_t low = (uint32_t)vaddr;
    uint64_t canonical = (low & 0x80000000u) ? (0xffffffff00000000ull | low) : low;
    if (vaddr != canonical && vaddr != (uint64_t)low)
        return DUCK_GUEST_RAM_UNMAPPED;
    if ((low & 0xe0000000u) != 0x80000000u &&
        (low & 0xe0000000u) != 0xa0000000u)
        return DUCK_GUEST_RAM_UNMAPPED;
    size_t physical = (size_t)(low & 0x1fffffffu);
    if ((physical & (alignment - 1)) != 0)
        return DUCK_GUEST_RAM_ALIGN;
    if (physical > ram->size || length > ram->size - physical)
        return DUCK_GUEST_RAM_RANGE;
    *out = ram->bytes + physical;
    return DUCK_GUEST_RAM_OK;
}
int duck_vita_guest_read_be32(const DuckVitaRdram *ram, uint64_t addr, uint32_t *out) {
    if (!out) return DUCK_GUEST_RAM_BAD_ARG;
    uint8_t *p;
    int rc = duck_vita_guest_span(ram, addr, 4, 4, &p);
    if (rc) return rc;
    *out = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
    return DUCK_GUEST_RAM_OK;
}
int duck_vita_guest_write_be32(const DuckVitaRdram *ram, uint64_t addr, uint32_t v) {
    uint8_t *p;
    int rc = duck_vita_guest_span(ram, addr, 4, 4, &p);
    if (rc) return rc;
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
    return DUCK_GUEST_RAM_OK;
}
