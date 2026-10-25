#include "duck_vita_scheduler.h"
#include <string.h>
void duck_vita_scheduler_init(DuckVitaScheduler *s){if(!s)return;memset(s,0,sizeof(*s));s->current=-1;}
int duck_vita_scheduler_add(DuckVitaScheduler *s,unsigned id,unsigned priority){
 if(!s||!id||s->count>=DUCK_SCHED_MAX_THREADS)return -1;
 for(size_t i=0;i<s->count;i++)if(s->threads[i].id==id)return -1;
 s->threads[s->count++]=(DuckVitaThread){id,priority,DUCK_THREAD_READY,0};return 0;
}
int duck_vita_scheduler_next(DuckVitaScheduler *s){
 if(!s)return -1;
 int best=-1;
 for(size_t i=0;i<s->count;i++)if(s->threads[i].state==DUCK_THREAD_READY &&
 (best<0||s->threads[i].priority>s->threads[best].priority))best=(int)i;
 if(best<0)return -1;
 if(s->current>=0 && s->threads[s->current].state==DUCK_THREAD_RUNNING)
 s->threads[s->current].state=DUCK_THREAD_READY;
 s->threads[best].state=DUCK_THREAD_RUNNING;s->current=best;
 return (int)s->threads[best].id;
}
int duck_vita_scheduler_wait_recv(DuckVitaScheduler *s,unsigned id,DuckVitaQueue *q){
 if(!s||!q||!q->slots)return -1;
 for(size_t i=0;i<s->count;i++)if(s->threads[i].id==id){
 if(s->threads[i].state!=DUCK_THREAD_RUNNING&&s->threads[i].state!=DUCK_THREAD_READY)return -1;
 if(q->count)return 1;
 s->threads[i].state=DUCK_THREAD_WAIT_RECV;s->threads[i].wait_queue=q;
 if(s->current==(int)i)s->current=-1;
 return 0;
 }return -1;
}
int duck_vita_scheduler_queue_ready(DuckVitaScheduler *s,DuckVitaQueue *q){
 if(!s||!q||!q->count)return 0;
 int best=-1;
 for(size_t i=0;i<s->count;i++)if(s->threads[i].state==DUCK_THREAD_WAIT_RECV&&
 s->threads[i].wait_queue==q&&(best<0||s->threads[i].priority>s->threads[best].priority))best=(int)i;
 if(best<0)return 0;
 s->threads[best].state=DUCK_THREAD_READY;s->threads[best].wait_queue=0;
 return (int)s->threads[best].id;
}
