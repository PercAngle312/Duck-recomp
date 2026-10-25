#include "duck_vita_guest_memory.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
int main(void) {
    uint8_t *data = calloc(1, DUCK_VITA_RDRAM_BYTES);
    assert(data);
    DuckVitaRdram ram = { data, DUCK_VITA_RDRAM_BYTES };
    uint8_t *ptr = NULL;
    uint32_t v = 0;
    assert(duck_vita_guest_write_be32(&ram, 0xffffffff80001000ull, 0x12345678) == 0);
    assert(duck_vita_guest_read_be32(&ram, 0xA0001000u, &v) == 0 && v == 0x12345678);
    assert(data[0x1000] == 0x12 && data[0x1003] == 0x78);
    assert(duck_vita_guest_span(&ram, 0x807ffffcu, 4, 4, &ptr) == 0);
    assert(ptr == data + DUCK_VITA_RDRAM_BYTES - 4);
    assert(duck_vita_guest_span(&ram, 0x807ffffcu, 8, 4, &ptr) == DUCK_GUEST_RAM_RANGE && !ptr);
    assert(duck_vita_guest_span(&ram, 0x80800000u, 1, 1, &ptr) == DUCK_GUEST_RAM_RANGE);
    assert(duck_vita_guest_span(&ram, 0xa4600000u, 4, 4, &ptr) == DUCK_GUEST_RAM_RANGE);
    assert(duck_vita_guest_span(&ram, 0x00001000u, 4, 4, &ptr) == DUCK_GUEST_RAM_UNMAPPED);
    assert(duck_vita_guest_span(&ram, 0xC0001000u, 4, 4, &ptr) == DUCK_GUEST_RAM_UNMAPPED);
    assert(duck_vita_guest_span(&ram, 0x0000000180001000ull, 4, 4, &ptr) == DUCK_GUEST_RAM_UNMAPPED);
    assert(duck_vita_guest_span(&ram, 0x80001002u, 4, 4, &ptr) == DUCK_GUEST_RAM_ALIGN);
    assert(duck_vita_guest_span(&ram, 0x80000000u, 4, 3, &ptr) == DUCK_GUEST_RAM_BAD_ARG);
    free(data);
    puts("PASS: KSEG0/KSEG1 aliases, endian, RDRAM bounds, MMIO isolation and alignment");
    return 0;
}
