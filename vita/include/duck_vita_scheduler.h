#pragma once
#include "duck_vita_queue.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { DUCK_SCHED_MAX_THREADS=16 };
typedef enum { DUCK_THREAD_STOPPED=0, DUCK_THREAD_READY=1, DUCK_THREAD_RUNNING=2, DUCK_THREAD_WAIT_RECV=3 } DuckThreadState;
typedef struct { unsigned id,priority; DuckThreadState state; DuckVitaQueue *wait_queue; } DuckVitaThread;
typedef struct { DuckVitaThread threads[DUCK_SCHED_MAX_THREADS]; size_t count; int current; } DuckVitaScheduler;
void duck_vita_scheduler_init(DuckVitaScheduler *s);
int duck_vita_scheduler_add(DuckVitaScheduler *s,unsigned id,unsigned priority);
int duck_vita_scheduler_next(DuckVitaScheduler *s);
/* Cooperative lifecycle operations; do not save ARM/MIPS registers. */
int duck_vita_scheduler_stop(DuckVitaScheduler *s,unsigned id);
int duck_vita_scheduler_resume(DuckVitaScheduler *s,unsigned id);
int duck_vita_scheduler_set_priority(DuckVitaScheduler *s,unsigned id,unsigned priority);
int duck_vita_scheduler_yield(DuckVitaScheduler *s);
int duck_vita_scheduler_wait_recv(DuckVitaScheduler *s,unsigned id,DuckVitaQueue *queue);
int duck_vita_scheduler_queue_ready(DuckVitaScheduler *s,DuckVitaQueue *queue);
/* Atomic in this single-threaded cooperative model: enqueue then wake one
 * highest-priority receiver, never wake if the enqueue fails. */
int duck_vita_scheduler_send(DuckVitaScheduler *s,DuckVitaQueue *queue,
                             uint32_t message,int jam);
#ifdef __cplusplus
}
#endif
