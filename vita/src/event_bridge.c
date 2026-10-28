#include "duck_vita_event_bridge.h"
#include <string.h>
void duck_vita_event_bridge_init(DuckVitaEventBridge *bridge,
 DuckVitaScheduler *scheduler,DuckVitaQueue *queue,uint32_t message) {
 if(!bridge)return;
 memset(bridge,0,sizeof(*bridge));
 bridge->scheduler=scheduler;bridge->queue=queue;bridge->message=message;
}
void duck_vita_event_bridge_irq(void *opaque,int deliverable) {
 DuckVitaEventBridge *b=opaque;
 if(!b||!deliverable||!b->scheduler||!b->queue)return;
 if(duck_vita_scheduler_send(b->scheduler,b->queue,b->message,0)==DUCK_QUEUE_OK)
  b->delivered++;
 else b->dropped++;
}
