#include "duck_vita_queue.h"
#include <stdint.h>
int duck_vita_queue_init(DuckVitaQueue *q,uint32_t *storage,size_t capacity) {
    if(!q||!storage||!capacity||capacity>SIZE_MAX/sizeof(uint32_t))return DUCK_QUEUE_INVALID;
    *q=(DuckVitaQueue){storage,capacity,0,0};
    return DUCK_QUEUE_OK;
}
int duck_vita_queue_send(DuckVitaQueue *q,uint32_t message,int jam) {
    if(!q||!q->slots||!q->capacity||q->head>=q->capacity||q->count>q->capacity)
        return DUCK_QUEUE_INVALID;
    if(q->count==q->capacity)return DUCK_QUEUE_FULL;
    if(jam) {
        q->head=q->head ? q->head-1 : q->capacity-1;
        q->slots[q->head]=message;
    } else {
        size_t tail=q->head+q->count;
        if(tail>=q->capacity)tail-=q->capacity;
        q->slots[tail]=message;
    }
    q->count++;
    return DUCK_QUEUE_OK;
}
int duck_vita_queue_recv(DuckVitaQueue *q,uint32_t *message) {
    if(!q||!message||!q->slots||!q->capacity||q->head>=q->capacity||q->count>q->capacity)
        return DUCK_QUEUE_INVALID;
    if(!q->count)return DUCK_QUEUE_EMPTY;
    *message=q->slots[q->head];
    q->head++;
    if(q->head==q->capacity)q->head=0;
    q->count--;
    return DUCK_QUEUE_OK;
}
