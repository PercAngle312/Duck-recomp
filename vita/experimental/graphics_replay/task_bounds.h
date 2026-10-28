#pragma once
#include <stdint.h>
/* Pure guest-memory span validator shared by renderer and host tests. */
static inline int duck_task_span_valid(uint32_t guest, uint32_t bytes,
                                       uint32_t alignment, uint32_t ram_size) {
    uint32_t physical = guest & 0x1fffffffu;
    uint32_t segment = guest & 0xe0000000u;
    if (segment != 0 && segment != 0x80000000u && segment != 0xa0000000u) return 0;
    if (!alignment || (alignment & (alignment - 1u))) return 0;
    return physical < ram_size && (physical & (alignment - 1u)) == 0 &&
           bytes <= ram_size - physical;
}
