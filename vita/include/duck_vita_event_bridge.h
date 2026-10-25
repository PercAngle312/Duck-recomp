#pragma once
#include "duck_vita_cpu_irq.h"
#include "duck_vita_scheduler.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Deterministic event handoff model: a CPU interrupt-ready edge queues
 * an OS event and wakes its receiver. Does not execute translated callbacks,
 * save guest registers, or implement libultra exception dispatch. */
typedef struct {
 DuckVitaScheduler *scheduler;
 DuckVitaQueue *queue;
 uint32_t message;
 unsigned delivered, dropped;
} DuckVitaEventBridge;
void duck_vita_event_bridge_init(DuckVitaEventBridge *bridge,
 DuckVitaScheduler *scheduler,DuckVitaQueue *queue,uint32_t message);
void duck_vita_event_bridge_irq(void *opaque,int deliverable);
#ifdef __cplusplus
}
#endif
