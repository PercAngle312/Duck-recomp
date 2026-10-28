#include "duck_vita_bus.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
typedef struct { uint32_t last_address, value; unsigned reads, writes; } Device;
static int rd(void *opaque, uint32_t addr, uint32_t *value) {
    Device *d = opaque; d->last_address = addr; d->reads++; *value = d->value; return 0;
}
static int wr(void *opaque, uint32_t addr, uint32_t value) {
    Device *d = opaque; d->last_address = addr; d->writes++; d->value = value; return 0;
}
int main(void) {
    uint8_t *memory = calloc(1, DUCK_VITA_RDRAM_BYTES);
    assert(memory);
    DuckVitaBus bus = {0};
    bus.rdram = (DuckVitaRdram){memory, DUCK_VITA_RDRAM_BYTES};
    Device d = {0};
    DuckVitaMmioRegion region = {0x04600000, 0x04600040, rd, wr, &d};
    assert(duck_vita_bus_register(&bus, region) == 0);
    assert(duck_vita_bus_register(&bus, region) != 0);
    uint32_t v = 0;
    assert(duck_vita_bus_write32(&bus, 0xffffffffa4600000ull, 0xdeadbeefu) == 0);
    assert(duck_vita_bus_read32(&bus, 0x84600000u, &v) == 0);
    assert(v == 0xdeadbeefu && d.writes == 1 && d.reads == 1);
    assert(d.last_address == 0x04600000);
    assert(duck_vita_bus_write32(&bus, 0x80000000u, 0x12345678u) == 0);
    assert(duck_vita_bus_read32(&bus, 0xa0000000u, &v) == 0 && v == 0x12345678u);
    assert(duck_vita_bus_write32(&bus, 0xa4600040u, 1) == DUCK_GUEST_RAM_UNMAPPED);
    assert(duck_vita_bus_read32(&bus, 0xa4500000u, &v) == DUCK_GUEST_RAM_UNMAPPED);
    assert(duck_vita_bus_read32(&bus, 0xa4600002u, &v) == DUCK_GUEST_RAM_ALIGN);
    assert(d.writes == 1 && d.reads == 1);
    free(memory);
    puts("PASS: dispatched PI-range MMIO, RDRAM alias, unregistered isolation, alignment");
    return 0;
}
