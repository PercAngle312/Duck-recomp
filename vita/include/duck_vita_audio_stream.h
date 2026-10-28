#pragma once
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
/* One producer and one consumer. Cursor release/acquire transfers ownership;
 * neither side ever reads a slot which the other side is modifying. */
enum {DUCK_AUDIO_RING=16384,DUCK_AUDIO_PREFILL=3072,DUCK_AUDIO_HIGH_WATER=6144};
typedef struct {
 atomic_uint read,write,first_tick,underruns,padded;
 int16_t data[DUCK_AUDIO_RING*2];
} DuckAudioStream;
static inline void duck_audio_stream_reset(DuckAudioStream *s){
 atomic_store(&s->read,0);atomic_store(&s->write,0);atomic_store(&s->first_tick,0);
 atomic_store(&s->underruns,0);atomic_store(&s->padded,0);
}
static inline unsigned duck_audio_stream_queued(DuckAudioStream *s){
 unsigned r=atomic_load_explicit(&s->read,memory_order_acquire);
 unsigned w=atomic_load_explicit(&s->write,memory_order_acquire);return w-r;
}
static inline unsigned duck_audio_stream_put(DuckAudioStream *s,const int16_t *pcm,unsigned count,unsigned tick){
 unsigned w=atomic_load_explicit(&s->write,memory_order_relaxed);
 unsigned r=atomic_load_explicit(&s->read,memory_order_acquire);
 unsigned free=DUCK_AUDIO_RING-(w-r);if(count>free)count=free;
 if(w==r)atomic_store_explicit(&s->first_tick,tick,memory_order_release);
 for(unsigned i=0;i<count;i++){
  unsigned at=(w+i)&(DUCK_AUDIO_RING-1);s->data[at*2]=pcm[i*2];s->data[at*2+1]=pcm[i*2+1];
 }
 atomic_store_explicit(&s->write,w+count,memory_order_release);return count;
}
static inline unsigned duck_audio_stream_take(DuckAudioStream *s,int16_t *pcm,unsigned count){
 unsigned r=atomic_load_explicit(&s->read,memory_order_relaxed);
 unsigned w=atomic_load_explicit(&s->write,memory_order_acquire);if(count>w-r)count=w-r;
 for(unsigned i=0;i<count;i++){
  unsigned at=(r+i)&(DUCK_AUDIO_RING-1);pcm[i*2]=s->data[at*2];pcm[i*2+1]=s->data[at*2+1];
 }
 atomic_store_explicit(&s->read,r+count,memory_order_release);return count;
}
