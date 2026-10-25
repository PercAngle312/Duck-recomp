#include "duck_vita_pi.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { unsigned raised, cleared; } Irq;
static void irq(void *opaque, int asserted) {
    Irq *i = opaque; if (asserted) i->raised++; else i->cleared++;
}
int main(void) {
    uint8_t data[64], *memory = calloc(1, DUCK_VITA_RDRAM_BYTES);
    assert(memory);
    for (unsigned i = 0; i < 64; ++i) data[i] = (uint8_t)i;
    DuckVitaBus bus = {0}; bus.rdram = (DuckVitaRdram){memory, DUCK_VITA_RDRAM_BYTES};
    DuckVitaPi pi; Irq interrupts = {0};
    assert(duck_vita_pi_init(&pi, &bus, (DuckVitaRom){data, sizeof(data)}, irq, &interrupts) == 0);
    assert(duck_vita_bus_write32(&bus, 0xa4600000u, 0x100) == 0);
    assert(duck_vita_bus_write32(&bus, 0xa4600004u, DUCK_PI_CART_BASE + 8) == 0);
    assert(duck_vita_bus_write32(&bus, 0xa4600008u, 15) == 0);
    assert(memcmp(memory + 0x100, data + 8, 16) == 0);
    uint32_t status = 0;
    assert(duck_vita_bus_read32(&bus, 0x84600010u, &status) == 0);
    assert((status & DUCK_PI_STATUS_INTERRUPT) && interrupts.raised == 1);
    assert(duck_vita_bus_write32(&bus, 0xa4600010u, 2) == 0);
    assert(duck_vita_bus_read32(&bus, 0xa4600010u, &status) == 0);
    assert(!(status & DUCK_PI_STATUS_INTERRUPT) && interrupts.cleared == 1);
    assert(duck_vita_bus_write32(&bus, 0xa4600004u, DUCK_PI_CART_BASE + 60) == 0);
    assert(duck_vita_bus_write32(&bus, 0xa4600008u, 15) == DUCK_GUEST_RAM_RANGE);
    assert(interrupts.raised == 1);
    assert(duck_vita_bus_write32(&bus, 0xa460000cu, 7) == DUCK_GUEST_RAM_UNMAPPED);
    free(memory);
    puts("PASS: PI register MMIO, ROM read DMA, interrupt edge/ack, invalid transfers");
    return 0;
}
