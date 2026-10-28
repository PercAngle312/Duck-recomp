#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Deterministic nonblocking message queue primitive. No thread suspension,
 * preemption or libultra replacement is implied by this host-tested adapter. */
typedef struct {
    uint32_t *slots;
    size_t capacity, head, count;
} DuckVitaQueue;
enum { DUCK_QUEUE_OK=0, DUCK_QUEUE_INVALID=-1,
       DUCK_QUEUE_FULL=-2, DUCK_QUEUE_EMPTY=-3 };
int duck_vita_queue_init(DuckVitaQueue *q,uint32_t *storage,size_t capacity);
int duck_vita_queue_send(DuckVitaQueue *q,uint32_t message,int jam);
int duck_vita_queue_recv(DuckVitaQueue *q,uint32_t *message);
#ifdef __cplusplus
}
#endif
