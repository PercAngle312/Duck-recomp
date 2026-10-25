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
int duck_vita_scheduler_wait_recv(DuckVitaScheduler *s,unsigned id,DuckVitaQueue *queue);
int duck_vita_scheduler_queue_ready(DuckVitaScheduler *s,DuckVitaQueue *queue);
#ifdef __cplusplus
}
#endif
