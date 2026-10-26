#define _POSIX_C_SOURCE 200809L
#include "duck_vita_ai.h"
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static pthread_t host,thread;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static DuckTestThreadEntry entry;
static int sem_count,sema_live,thread_live,thread_started,port_live;
static int gate,gate_waiting,open_error,config_error,output_error,drain_error;
static int create_sema_error,create_thread_error,start_error,signal_error,join_error;
static int volume_error;
static const void *last_native_buffer;
static unsigned frames,released;
static atomic_uint configured,outputs,drains,interrupts;
int sceKernelCreateSema(const char *name,unsigned attr,int initial,int max,void *opt){
 assert(name&&!attr&&!initial&&max==1&&!opt&&!sema_live);
 if(create_sema_error)return create_sema_error;
 sema_live=1;sem_count=0;return 20;
}
int sceKernelWaitSema(int id,int count,unsigned *timeout){
 assert(id==20&&count==1&&!timeout);assert(!pthread_mutex_lock(&mutex));
 while(!sem_count)assert(!pthread_cond_wait(&changed,&mutex));
 sem_count--;assert(!pthread_mutex_unlock(&mutex));return 0;
}
int sceKernelSignalSema(int id,int count){
 assert(id==20&&count==1);if(signal_error)return signal_error;
 assert(!pthread_mutex_lock(&mutex));int rc=sem_count?-1:0;
 if(!rc){sem_count=1;assert(!pthread_cond_broadcast(&changed));}
 assert(!pthread_mutex_unlock(&mutex));return rc;
}
int sceKernelDeleteSema(int id){assert(id==20&&sema_live&&!thread_live);sema_live=0;return 0;}
int sceKernelCreateThread(const char *name,DuckTestThreadEntry fn,int priority,unsigned stack,unsigned attr,int affinity,const void *opt){
 assert(name&&priority==0x10000100&&stack>=0x10000&&!attr&&!affinity&&!opt&&!thread_live);
 if(create_thread_error)return create_thread_error;
 entry=fn;thread_live=1;return 21;
}
static void *run(void *arg){(void)arg;entry(0,NULL);return NULL;}
int sceKernelStartThread(int id,unsigned length,void *args){
 assert(id==21&&thread_live&&!length&&!args&&!thread_started);if(start_error)return start_error;
 assert(!pthread_create(&thread,NULL,run,NULL));thread_started=1;return 0;
}
int sceKernelWaitThreadEnd(int id,int *result,unsigned *timeout){
 assert(pthread_equal(pthread_self(),host)&&id==21&&!result&&!timeout&&thread_started);
 if(join_error)return join_error;
 assert(!pthread_join(thread,NULL));thread_started=0;return 0;
}
int sceKernelDeleteThread(int id){assert(id==21&&thread_live&&!thread_started);thread_live=0;return 0;}
int sceAudioOutOpenPort(SceAudioOutPortType type,int len,int rate,SceAudioOutMode mode){
 assert(!port_live&&type==SCE_AUDIO_OUT_PORT_TYPE_MAIN&&len==256&&rate==48000&&mode==SCE_AUDIO_OUT_MODE_STEREO);
 if(open_error)return open_error;
 port_live=1;return 7;
}
int sceAudioOutSetConfig(int port,unsigned len,int rate,SceAudioOutMode mode){
 assert(port==7&&port_live&&len&&len<=256&&!(len%64)&&rate==-1);
 assert(!pthread_equal(pthread_self(),host));assert(mode==SCE_AUDIO_OUT_MODE_STEREO);
 atomic_fetch_add(&configured,1);frames=len;return config_error;
}
int sceAudioOutSetVolume(int port,SceAudioOutChannelFlag flags,int *volumes){
 assert(port==7&&port_live&&flags==(SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH));
 assert(volumes[0]==SCE_AUDIO_OUT_MAX_VOL&&volumes[1]==SCE_AUDIO_OUT_MAX_VOL);return volume_error;
}
int sceAudioOutOutput(int port,const void *buffer){
 assert(port==7&&port_live);if(!buffer){last_native_buffer=NULL;atomic_fetch_add(&drains,1);return drain_error;}
 assert(buffer!=last_native_buffer);last_native_buffer=buffer;
 assert(!pthread_equal(pthread_self(),host)&&!((uintptr_t)buffer%64));
 assert(!pthread_mutex_lock(&mutex));
 if(gate){gate_waiting=1;assert(!pthread_cond_broadcast(&changed));}
 while(gate)assert(!pthread_cond_wait(&changed,&mutex));
 assert(!pthread_mutex_unlock(&mutex));const int16_t *pcm=buffer;
 assert(pcm[0]==12000||pcm[0]==6000);
 for(unsigned i=0;i<frames;i++)assert((pcm[i*2]==12000&&pcm[i*2+1]==-12000)||(pcm[i*2]==6000&&pcm[i*2+1]==-6000)||(!pcm[i*2]&&!pcm[i*2+1]));
 atomic_fetch_add(&outputs,1);return output_error;
}
int sceAudioOutReleasePort(int port){assert(port==7&&port_live&&!thread_live&&!sema_live);port_live=0;released++;return 0;}
int duck_vita_os_event_emit(unsigned event){assert(event==6&&pthread_equal(pthread_self(),host));atomic_fetch_add(&interrupts,1);return 0;}
static void pattern(uint8_t *ram,unsigned at,unsigned count,int left){
 for(unsigned i=0;i<count;i++)for(unsigned ch=0;ch<2;ch++){
  unsigned value=(uint16_t)(ch?-left:left),p=at+i*4+ch*2;
  ram[p^3u]=(uint8_t)(value>>8);ram[(p+1)^3u]=(uint8_t)value;
 }
}
static void queue(uint8_t *ram,unsigned at,unsigned bytes){
 recomp_context c={0};c.r4=32000;osAiSetFrequency_recomp(ram,&c);assert(c.r2>31000&&c.r2<33000);
 c.r4=0x80000000u+at;c.r5=bytes;osAiSetNextBuffer_recomp(ram,&c);assert(!c.r2);
}
static int complete(void){
 struct timespec pause={0,1000000};
 for(unsigned i=0;i<2000;i++){int rc=duck_vita_ai_service();if(rc)return rc;nanosleep(&pause,NULL);}
 assert(!"audio worker timeout");return -1;
}
int main(void){
 assert(sizeof(SceAudioOutMode)==1);host=pthread_self();uint8_t *ram=calloc(1,0x800000);assert(ram);
 open_error=-11;assert(duck_vita_ai_vita_init(ram,48681812)==-11);open_error=0;
 int *errors[]={&volume_error,&create_sema_error,&create_thread_error,&start_error};
 for(unsigned i=0;i<4;i++){
  *errors[i]=-20-(int)i;assert(duck_vita_ai_vita_init(ram,48681812)==*errors[i]);*errors[i]=0;
  assert(!port_live&&!sema_live&&!thread_live);
 }
 pattern(ram,0x1000,1024,12000);pattern(ram,0x3000,1024,6000);
 assert(!duck_vita_ai_vita_init(ram,48681812));assert(duck_vita_ai_vita_init(ram,48681812)==-1);
 queue(ram,0x1000,4096);queue(ram,0x3000,4096);gate=1;assert(!duck_vita_ai_service());
 assert(!pthread_mutex_lock(&mutex));while(!gate_waiting)assert(!pthread_cond_wait(&changed,&mutex));assert(!pthread_mutex_unlock(&mutex));
 // Playback is blocked, yet the host services AI without blocking or early IRQ.
 pattern(ram,0x1000,1024,-8000); // must not alter the worker's owned snapshot
 for(unsigned i=0;i<1000;i++)assert(!duck_vita_ai_service()&&!atomic_load(&interrupts));
 assert(!pthread_mutex_lock(&mutex));gate=0;assert(!pthread_cond_broadcast(&changed));assert(!pthread_mutex_unlock(&mutex));
 assert(complete()==1&&atomic_load(&interrupts)==1);assert(complete()==1&&atomic_load(&interrupts)==2);
 assert(atomic_load(&outputs)>2&&atomic_load(&drains)>=2);
 queue(ram,0x3000,4096);assert(!duck_vita_ai_service());unsigned irq=atomic_load(&interrupts);
 duck_vita_ai_vita_close();assert(!duck_vita_ai_service()&&atomic_load(&interrupts)==irq);
 assert(!port_live&&!thread_live&&!sema_live);pattern(ram,0x1000,1024,12000);
 int *failures[]={&config_error,&output_error,&drain_error,&signal_error};
 for(unsigned i=0;i<4;i++){
  assert(!duck_vita_ai_vita_init(ram,48681812));*failures[i]=-30-(int)i;queue(ram,0x1000,4096);irq=atomic_load(&interrupts);
  assert(complete()==-1&&duck_vita_ai_error()==*failures[i]&&atomic_load(&interrupts)==irq);
  *failures[i]=0;duck_vita_ai_vita_close();assert(!port_live&&!thread_live&&!sema_live);
 }
 assert(!duck_vita_ai_vita_init(ram,48681812));join_error=-44;duck_vita_ai_vita_close();
 assert(port_live&&thread_live&&sema_live&&duck_vita_ai_vita_init(ram,48681812)==-1);
 join_error=0;duck_vita_ai_vita_close();assert(!port_live&&!thread_live&&!sema_live);
 unsigned before=released;duck_vita_ai_vita_close();assert(released==before);free(ram);
 puts("PASS: concurrent native worker, nonblocking host, owned stereo snapshot, two-DMA completion order, device errors and joined shutdown");return 0;
}
