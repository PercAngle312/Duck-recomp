#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Minimal CPU interrupt-line bridge. It is NOT the N64 CPU scheduler.
 * MI drives IP2 (Cause bit 10); delivery requires Status IE, !EXL, !ERL,
 * and the matching Status interrupt mask. */
enum { DUCK_CPU_STATUS_IE=1u, DUCK_CPU_STATUS_EXL=2u, DUCK_CPU_STATUS_ERL=4u,
       DUCK_CPU_CAUSE_IP2=1u<<10, DUCK_CPU_STATUS_IM2=1u<<10 };
typedef struct {
    uint32_t status, cause;
    int deliverable;
    void (*on_change)(void *opaque,int deliverable);
    void *opaque;
} DuckVitaCpuIrq;
void duck_vita_cpu_irq_init(DuckVitaCpuIrq *cpu,
    void (*on_change)(void*,int), void *opaque);
void duck_vita_cpu_irq_set_status(DuckVitaCpuIrq *cpu,uint32_t status);
void duck_vita_cpu_irq_mi_line(void *opaque,int asserted);
int duck_vita_cpu_irq_pending(const DuckVitaCpuIrq *cpu);
#ifdef __cplusplus
}
#endif
