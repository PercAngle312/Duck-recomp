#pragma once
#include "duck_vita_guest_memory.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Cartridge ROM is a caller-owned read-only view. No proprietary ROM data is
 * bundled in the repository or diagnostic binary. This primitive implements
 * only a validated byte transfer, not PI timings, registers or interrupts. */
typedef struct { const uint8_t *data; size_t size; } DuckVitaRom;
int duck_vita_dma_rom_to_rdram(const DuckVitaRom *rom, size_t rom_offset,
    const DuckVitaRdram *ram, uint64_t rdram_addr, size_t count);
#ifdef __cplusplus
}
#endif
