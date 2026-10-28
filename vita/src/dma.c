#include "duck_vita_dma.h"
#include <string.h>
int duck_vita_dma_rom_to_rdram(const DuckVitaRom *rom, size_t rom_offset,
    const DuckVitaRdram *ram, uint64_t rdram_addr, size_t count) {
    if (!rom || !rom->data || !ram || !ram->bytes)
        return DUCK_GUEST_RAM_BAD_ARG;
    if (rom_offset > rom->size || count > rom->size - rom_offset)
        return DUCK_GUEST_RAM_RANGE;
    uint8_t *destination = 0;
    int result = duck_vita_guest_span(ram, rdram_addr, count, 1, &destination);
    if (result) return result;
    /* The caller must supply a ROM allocation disjoint from RDRAM. memmove
     * additionally gives deterministic behavior for accidental overlap. */
    memmove(destination, rom->data + rom_offset, count);
    return DUCK_GUEST_RAM_OK;
}
