#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
uint64_t sceKernelGetProcessTimeWide(void);
int sceKernelDelayThread(unsigned);
#include "../src/audio_vita.c"
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
static atomic_uint ticks,output_count,events;
static atomic_int output_failure;
static int reject_high,reject_all,last_priority;
static pthread_t thread;
static int (*thread_entry)(unsigned,void*);
static void nap(void){struct timespec t={0,1000000};nanosleep(&t,0);}
uint64_t sceKernelGetProcessTimeWide(void){return atomic_load(&ticks);}
int sceKernelDelayThread(unsigned us){(void)us;nap();return 0;}
static void *run(void *x){(void)x;thread_entry(0,0);return 0;}
int sceKernelCreateThread(const char *n,int (*f)(unsigned,void*),int p,unsigned s,unsigned a,int c,const void *o){(void)n;(void)s;(void)a;(void)c;(void)o;last_priority=p;if(reject_all||(reject_high&&p==0x10000080))return -42;thread_entry=f;return 1;}
int sceKernelStartThread(int id,unsigned n,void *p){(void)id;(void)n;(void)p;return pthread_create(&thread,0,run,0);}
int sceKernelWaitThreadEnd(int id,int *r,unsigned *t){(void)id;(void)r;(void)t;return pthread_join(thread,0);}
int sceKernelDeleteThread(int id){(void)id;return 0;}
int sceAudioOutOpenPort(SceAudioOutPortType t,int f,int r,SceAudioOutMode m){assert(t==SCE_AUDIO_OUT_PORT_TYPE_MAIN&&f==256&&r==48000&&m==SCE_AUDIO_OUT_MODE_STEREO);return 2;}
int sceAudioOutSetVolume(int p,SceAudioOutChannelFlag f,int *v){(void)p;(void)f;(void)v;return 0;}
int sceAudioOutOutput(int p,const void *data){(void)p;if(data){const int16_t *s=data;for(unsigned i=0;i<256;i++){assert(s[i*2]>=0&&s[i*2]<=12000);assert(s[i*2+1]==-s[i*2]);}atomic_fetch_add(&output_count,1);nap();}return atomic_load(&output_failure);}
int sceAudioOutReleasePort(int p){(void)p;return 0;}
int duck_vita_os_event_emit(unsigned e){assert(e==6);atomic_fetch_add(&events,1);return 0;}
static void fill(uint8_t *ram,unsigned frames){for(unsigned i=0;i<frames;i++){unsigned l=12000,r=(unsigned)(uint16_t)-12000;ram[(i*4)^3]=l>>8;ram[(i*4+1)^3]=l;ram[(i*4+2)^3]=r>>8;ram[(i*4+3)^3]=r;}}
static void enqueue(uint8_t *ram){recomp_context c={0};c.r4=0x80000000u;c.r5=552*4;osAiSetNextBuffer_recomp(ram,&c);assert(c.r2==0);}
int main(void){
 uint8_t *ram=calloc(1,0x800000);assert(ram);fill(ram,552);
 assert(!duck_vita_ai_vita_init(ram,48681812));assert(last_priority==0x10000080);
 recomp_context c={0};c.r4=24000;osAiSetFrequency_recomp(ram,&c);assert(c.r2==24004);
 enqueue(ram);enqueue(ram);for(int i=0;i<5;i++)nap();assert(!atomic_load(&output_count));
 assert(write_dma-read_dma==2);assert(!poll(0));
 atomic_store(&ticks,23000);assert(poll(0)==1);assert(!poll(0));
 /* Direct sink poll above advanced only backend; restart clean for integration. */
 duck_vita_ai_vita_close();reject_high=1;assert(!duck_vita_ai_vita_init(ram,48681812));assert(last_priority==0x10000100);reject_high=0;c.r4=24000;osAiSetFrequency_recomp(ram,&c);
 enqueue(ram);enqueue(ram);atomic_store(&ticks,46000);assert(duck_vita_ai_service()==1);assert(atomic_load(&events)==1);
 enqueue(ram); /* Third DMA supplies lookahead; guest progresses before speakers. */
 for(int i=0;i<100&&!atomic_load(&output_count);i++)nap();assert(atomic_load(&output_count));
 memset(ram,0,552*4); /* Ring already owns PCM; reuse cannot alter queued audio. */
 duck_vita_ai_vita_close();
 /* One short DMA starts after the timeout without a second DMA. */
 atomic_store(&ticks,0);atomic_store(&output_count,0);fill(ram,552);
 assert(!duck_vita_ai_vita_init(ram,48681812));c.r4=24000;osAiSetFrequency_recomp(ram,&c);enqueue(ram);
 for(int i=0;i<5;i++)nap();assert(!atomic_load(&output_count));atomic_store(&ticks,80001);
 for(int i=0;i<100&&!atomic_load(&output_count);i++)nap();assert(atomic_load(&output_count));
 duck_vita_ai_vita_close();
 /* A device failure remains a fault, never a successful completion. */
 atomic_store(&output_failure,-77);atomic_store(&ticks,0);atomic_store(&output_count,0);
 assert(!duck_vita_ai_vita_init(ram,48681812));c.r4=24000;osAiSetFrequency_recomp(ram,&c);enqueue(ram);
 atomic_store(&ticks,80001);for(int i=0;i<100&&!atomic_load(&failure);i++)nap();
 assert(poll(0)==-77);duck_vita_ai_vita_close();
 reject_all=1;assert(duck_vita_ai_vita_init(ram,48681812)==-42);assert(port==-1);reject_all=0;
 /* High water applies backpressure without dropping queued PCM. */
 atomic_store(&failure,0);duck_audio_stream_reset(&stream);read_dma=0;write_dma=1;
 dma[0].end=1;atomic_store(&stream.write,DUCK_AUDIO_HIGH_WATER+1);assert(!poll(0));
 atomic_store(&stream.read,1);assert(poll(0)==1);
 free(ram);puts("buffered audio: timed completions, prefill, stereo, RDRAM ownership, short sound and shutdown passed");
}
