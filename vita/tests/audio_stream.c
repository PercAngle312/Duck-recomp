#include "duck_vita_audio_stream.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
static DuckAudioStream s;
static void *produce(void *p){(void)p;for(unsigned i=0;i<1000000;){int16_t v[2]={(int16_t)i,(int16_t)~i};if(duck_audio_stream_put(&s,v,1,i))i++;}return 0;}
int main(void){duck_audio_stream_reset(&s);atomic_store(&s.read,UINT32_MAX-100);atomic_store(&s.write,UINT32_MAX-100);
 pthread_t t;assert(!pthread_create(&t,0,produce,0));for(unsigned i=0;i<1000000;){int16_t v[2];if(duck_audio_stream_take(&s,v,1)){assert(v[0]==(int16_t)i&&v[1]==(int16_t)~i);i++;}}assert(!pthread_join(t,0));assert(!duck_audio_stream_queued(&s));puts("stereo FIFO: 1000000 concurrent frames, ring and uint32 cursor wrap passed");}
