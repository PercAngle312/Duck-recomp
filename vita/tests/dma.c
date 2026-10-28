#include "duck_vita_dma.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    uint8_t rom_data[32], *rdram = calloc(1, DUCK_VITA_RDRAM_BYTES);
    assert(rdram);
    for (size_t i = 0; i < sizeof(rom_data); ++i) rom_data[i] = (uint8_t)(i + 1);
    DuckVitaRom rom = {rom_data, sizeof(rom_data)};
    DuckVitaRdram ram = {rdram, DUCK_VITA_RDRAM_BYTES};
    assert(duck_vita_dma_rom_to_rdram(&rom, 4, &ram, 0xffffffffa0000100ull, 8) == 0);
    assert(memcmp(rdram + 0x100, rom_data + 4, 8) == 0);
    assert(duck_vita_dma_rom_to_rdram(&rom, 0, &ram, 0x807ffffcu, 4) == 0);
    assert(memcmp(rdram + DUCK_VITA_RDRAM_BYTES - 4, rom_data, 4) == 0);
    uint8_t snapshot[8]; memcpy(snapshot, rdram + 0x100, sizeof(snapshot));
    assert(duck_vita_dma_rom_to_rdram(&rom, 30, &ram, 0x80000100u, 4) == DUCK_GUEST_RAM_RANGE);
    assert(duck_vita_dma_rom_to_rdram(&rom, 0, &ram, 0x807ffffcu, 8) == DUCK_GUEST_RAM_RANGE);
    assert(duck_vita_dma_rom_to_rdram(&rom, 0, &ram, 0xa4600000u, 4) == DUCK_GUEST_RAM_RANGE);
    assert(duck_vita_dma_rom_to_rdram(&rom, 0, &ram, 0xc0000100u, 4) == DUCK_GUEST_RAM_UNMAPPED);
    assert(memcmp(snapshot, rdram + 0x100, sizeof(snapshot)) == 0);
    assert(duck_vita_dma_rom_to_rdram(&rom, 32, &ram, 0x80000100u, 0) == 0);
    free(rdram);
    puts("PASS: ROM DMA byte transfer, aliases, end-of-RAM, atomic failure and MMIO isolation");
    return 0;
}
