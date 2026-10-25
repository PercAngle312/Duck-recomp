#pragma once
#include "duck_vita_bus.h"
#include "duck_vita_dma.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { DUCK_PI_BASE = 0x04600000u, DUCK_PI_END = 0x04600040u,
       DUCK_PI_CART_BASE = 0x10000000u, DUCK_PI_STATUS_DMA_BUSY = 1u,
       DUCK_PI_STATUS_INTERRUPT = 8u };
typedef void (*DuckPiInterrupt)(void *opaque, int asserted);
typedef struct {
    DuckVitaBus *bus;
    DuckVitaRom rom;
    uint32_t dram_addr, cart_addr, status;
    DuckPiInterrupt interrupt;
    void *interrupt_opaque;
} DuckVitaPi;
int duck_vita_pi_init(DuckVitaPi *pi, DuckVitaBus *bus, DuckVitaRom rom,
                       DuckPiInterrupt interrupt, void *opaque);
int duck_vita_pi_read(void *opaque, uint32_t physical, uint32_t *value);
int duck_vita_pi_write(void *opaque, uint32_t physical, uint32_t value);
#ifdef __cplusplus
}
#endif
