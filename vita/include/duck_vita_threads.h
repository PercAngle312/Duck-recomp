#pragma once
#include "recomp.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef recomp_func_t *(*DuckThreadLookup)(uint32_t address);
typedef int (*DuckThreadPump)(void);
enum { DUCK_THREADS_IDLE=0,DUCK_THREADS_WAITING=1,DUCK_THREADS_BUDGET=2,DUCK_THREADS_ERROR=-1 };
/* One guest CPU on one host thread. Generated function entries and backward
 * branch targets can checkpoint; an enabled-IRQ quantum returns to the host
 * service loop without running host callbacks on a guest fiber stack. */
enum {DUCK_THREADS_CHECKPOINT_QUANTUM=1024};
int duck_vita_threads_init(uint8_t *rdram,DuckThreadLookup lookup,DuckThreadPump pump);
int duck_vita_threads_boot(uint32_t entry,uint32_t stack,uint32_t argument,int priority);
int duck_vita_threads_run(unsigned dispatch_budget);
int duck_vita_threads_error(void);
void duck_vita_thread_checkpoint(uint8_t *rdram,recomp_context *ctx);
uint64_t duck_vita_threads_checkpoint_yields(void);
void osCreateThread_recomp(uint8_t*,recomp_context*);
void osStartThread_recomp(uint8_t*,recomp_context*);
void osDestroyThread_recomp(uint8_t*,recomp_context*);
void osSetThreadPri_recomp(uint8_t*,recomp_context*);
void osYieldThread_recomp(uint8_t*,recomp_context*);
void osSetIntMask_recomp(uint8_t*,recomp_context*);
uint32_t duck_vita_thread_rcp_mask(void);
void duck_vita_thread_set_rcp_mask(uint32_t mask);
/* Internal stack backend; Vita uses SceFiber, host tests use ucontext. */
int duck_fiber_create(unsigned slot,void (*entry)(unsigned));
int duck_fiber_prepare(void);
int duck_fiber_run(unsigned slot);
void duck_fiber_suspend(unsigned slot);
void duck_fiber_destroy(unsigned slot);
typedef int (*DuckQueueWait)(uint32_t queue,int sending);
typedef void (*DuckQueueWake)(uint32_t queue,int sending);
void duck_vita_os_queue_hooks(DuckQueueWait wait,DuckQueueWake wake,void (*preempt)(void));

#ifdef __cplusplus
}
#endif
