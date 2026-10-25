#include "duck_vita_scheduler.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 DuckVitaScheduler s;DuckVitaQueue q;uint32_t items[2]={0},value;
 duck_vita_scheduler_init(&s);
 assert(duck_vita_queue_init(&q,items,2)==0);
 assert(duck_vita_scheduler_add(&s,1,1)==0);
 assert(duck_vita_scheduler_add(&s,2,10)==0);
 assert(duck_vita_scheduler_add(&s,2,20)==-1);
 assert(duck_vita_scheduler_next(&s)==2);
 /* Thread 1 is ready, but may not preempt running priority-10 thread 2. */
 assert(duck_vita_scheduler_next(&s)==2);
 assert(s.threads[0].state==DUCK_THREAD_READY);
 assert(duck_vita_scheduler_wait_recv(&s,2,&q)==0);
 assert(duck_vita_scheduler_next(&s)==1);
 assert(duck_vita_scheduler_send(&s,&q,0x1234,0)==0);
 assert(s.threads[1].state==DUCK_THREAD_READY);
 assert(duck_vita_scheduler_next(&s)==2);
 assert(duck_vita_queue_recv(&q,&value)==0&&value==0x1234);
 assert(duck_vita_scheduler_wait_recv(&s,2,&q)==0);
 assert(duck_vita_scheduler_next(&s)==1);
 assert(duck_vita_scheduler_queue_ready(&s,&q)==0);
 assert(duck_vita_scheduler_send(&s,&q,0xbeef,0)==0);
 assert(s.threads[1].state==DUCK_THREAD_READY);
 assert(duck_vita_scheduler_send(&s,&q,0xface,0)==0);
 assert(duck_vita_scheduler_send(&s,&q,0xdead,0)==DUCK_QUEUE_FULL);
 puts("PASS: priority, blocking receive, queue wake, duplicate thread guard");
 return 0;
}
