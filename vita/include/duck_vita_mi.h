#pragma once
#include "duck_vita_bus.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { DUCK_MI_BASE=0x04300000u, DUCK_MI_END=0x04300010u,
       DUCK_MI_INTR_PI=1u<<4 };
typedef void (*DuckMiCpuIrq)(void *opaque, int asserted);
typedef struct {
    uint32_t pending, mask;
    DuckMiCpuIrq cpu_irq;
    void *opaque;
} DuckVitaMi;
int duck_vita_mi_init(DuckVitaMi *mi, DuckVitaBus *bus, DuckMiCpuIrq cb, void *opaque);
void duck_vita_mi_set_source(DuckVitaMi *mi, uint32_t source, int asserted);
int duck_vita_mi_read(void *opaque,uint32_t physical,uint32_t *value);
int duck_vita_mi_write(void *opaque,uint32_t physical,uint32_t value);
#ifdef __cplusplus
}
#endif
