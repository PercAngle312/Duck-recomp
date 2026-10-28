#include "duck_vita_queue.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
    uint32_t storage[3]={0},value=0;
    DuckVitaQueue q;
    assert(duck_vita_queue_init(&q,storage,3)==0);
    assert(duck_vita_queue_recv(&q,&value)==DUCK_QUEUE_EMPTY);
    assert(duck_vita_queue_send(&q,11,0)==0);
    assert(duck_vita_queue_send(&q,22,0)==0);
    assert(duck_vita_queue_send(&q,33,1)==0);
    assert(duck_vita_queue_send(&q,44,0)==DUCK_QUEUE_FULL);
    assert(duck_vita_queue_recv(&q,&value)==0 && value==33);
    assert(duck_vita_queue_recv(&q,&value)==0 && value==11);
    assert(duck_vita_queue_send(&q,44,0)==0);
    assert(duck_vita_queue_recv(&q,&value)==0 && value==22);
    assert(duck_vita_queue_recv(&q,&value)==0 && value==44);
    assert(duck_vita_queue_recv(&q,&value)==DUCK_QUEUE_EMPTY);
    assert(duck_vita_queue_init(&q,storage,0)==DUCK_QUEUE_INVALID);
    puts("PASS: bounded queue, jam priority, wraparound, full/empty behavior");
    return 0;
}
