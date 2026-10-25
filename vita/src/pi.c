#include "duck_vita_pi.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* Deterministic synchronous PI read-DMA checkpoint, not cycle-accurate PI.
 * Register semantics: RD_LEN at +0x08 transfers (value & 0xFFFFFF)+1 bytes
 * from cartridge to RDRAM. PI write-to-cart DMA and domain timing are pending.
 */
int duck_vita_pi_init(DuckVitaPi *pi, DuckVitaBus *bus, DuckVitaRom rom,
                       DuckPiInterrupt interrupt, void *opaque) {
    if (!pi || !bus || !rom.data || !rom.size) return DUCK_GUEST_RAM_BAD_ARG;
    memset(pi, 0, sizeof(*pi));
    pi->bus = bus; pi->rom = rom;
    pi->interrupt = interrupt; pi->interrupt_opaque = opaque;
    DuckVitaMmioRegion region = { DUCK_PI_BASE, DUCK_PI_END,
                                 duck_vita_pi_read, duck_vita_pi_write, pi };
    return duck_vita_bus_register(bus, region);
}
int duck_vita_pi_read(void *opaque, uint32_t physical, uint32_t *value) {
    DuckVitaPi *pi = opaque;
    if (!pi || !value || physical < DUCK_PI_BASE || physical >= DUCK_PI_END || (physical & 3))
        return DUCK_GUEST_RAM_BAD_ARG;
    switch (physical - DUCK_PI_BASE) {
        case 0: *value = pi->dram_addr; return 0;
        case 4: *value = pi->cart_addr; return 0;
        case 0x10: *value = pi->status; return 0;
        default: *value = 0; return 0;
    }
}
int duck_vita_pi_write(void *opaque, uint32_t physical, uint32_t value) {
    DuckVitaPi *pi = opaque;
    if (!pi || physical < DUCK_PI_BASE || physical >= DUCK_PI_END || (physical & 3))
        return DUCK_GUEST_RAM_BAD_ARG;
    switch (physical - DUCK_PI_BASE) {
        case 0: pi->dram_addr = value & 0x00fffffeu; return 0;
        case 4: pi->cart_addr = value & 0xfffffffeu; return 0;
        case 0x10:
            if (value & 2) {
                pi->status &= ~DUCK_PI_STATUS_INTERRUPT;
                if (pi->interrupt) pi->interrupt(pi->interrupt_opaque, 0);
            }
            return 0;
        case 8: {
            /* Protect the register state and memory on invalid DMA. */
            size_t count = (size_t)(value & 0x00ffffffu) + 1;
            if (pi->cart_addr < DUCK_PI_CART_BASE) return DUCK_GUEST_RAM_UNMAPPED;
            size_t rom_offset = (size_t)(pi->cart_addr - DUCK_PI_CART_BASE);
            int rc = duck_vita_dma_rom_to_rdram(&pi->rom, rom_offset,
                &pi->bus->rdram, 0xa0000000ull | pi->dram_addr, count);
            if (rc) return rc;
            pi->dram_addr += (uint32_t)count;
            pi->cart_addr += (uint32_t)count;
            pi->status |= DUCK_PI_STATUS_INTERRUPT;
            if (pi->interrupt) pi->interrupt(pi->interrupt_opaque, 1);
            return 0;
        }
        /* WR_LEN and PI timing/control registers need actual emulation. */
        default: return DUCK_GUEST_RAM_UNMAPPED;
    }
}
