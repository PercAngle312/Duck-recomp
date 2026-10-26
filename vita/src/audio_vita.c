#include "duck_vita_ai.h"
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
static int port=-1,worker=-1,work=-1,started;
enum {IDLE,RUNNING,DONE};
static atomic_int state,stopping;
static int completion;
static unsigned job_frames,job_rate;
static unsigned submitted,completed;
_Alignas(64) static uint8_t snapshot[0x40000];
static DuckAudioResampler resampler;
_Alignas(64) static int16_t pcm[256*2];
static int output(void *opaque,const uint8_t *ram,uint32_t address,unsigned frames,unsigned rate){
 (void)opaque;int done=0;
 while(!done){
  if(atomic_load_explicit(&stopping,memory_order_acquire))return -1;
  unsigned n=duck_vita_audio_resample(&resampler,ram,address,frames,rate,pcm,256,&done);
  if(!n)continue;
  unsigned aligned=(n+63u)&~63u;
  memset(pcm+n*2,0,(aligned-n)*2*sizeof(int16_t));
  /* VitaSDK uses small enums: casting -1 to this enum passes 255, not the
   * signed unchanged sentinel expected by the kernel. This port is stereo. */
  int result=sceAudioOutSetConfig(port,(int)aligned,-1,SCE_AUDIO_OUT_MODE_STEREO);if(result<0)return result;
  result=sceAudioOutOutput(port,pcm);if(result<0)return result;
 }
 // Wait for the final native buffer before releasing the guest DMA and IRQ.
 return sceAudioOutOutput(port,0);
}
static int playback(unsigned args,void *opaque){
 (void)args;(void)opaque;
 for(;;){
  int rc=sceKernelWaitSema(work,1,NULL);
  if(rc<0){completion=rc;atomic_store_explicit(&state,DONE,memory_order_release);return rc;}
  if(atomic_load_explicit(&stopping,memory_order_acquire))return 0;
  // Acquire job metadata and the owned copy published before semaphore wakeup.
  if(atomic_load_explicit(&state,memory_order_acquire)!=RUNNING)continue;
  completion=output(NULL,snapshot,0,job_frames,job_rate);
  atomic_store_explicit(&state,DONE,memory_order_release);
 }
}
static int begin(void *opaque,const uint8_t *ram,uint32_t address,unsigned frames,unsigned rate){
 (void)opaque;
 if(atomic_load_explicit(&state,memory_order_acquire)!=IDLE)return -1;
 if(!ram||!frames||frames>0x3fff8u/4||!rate||(address&7u)||address>0x800000u-frames*4)return -1;
 memcpy(snapshot,ram+address,frames*4);job_frames=frames;job_rate=rate;
 atomic_store_explicit(&state,RUNNING,memory_order_release);
 int rc=sceKernelSignalSema(work,1);
 if(rc<0){atomic_store_explicit(&state,IDLE,memory_order_release);return rc;}
 if(++submitted<=3)fprintf(stderr,"Native AI DMA start %u: frames=%u rate=%u\n",submitted,frames,rate);
 return 0;
}
static int poll(void *opaque){
 (void)opaque;if(atomic_load_explicit(&state,memory_order_acquire)!=DONE)return 0;
 int rc=completion;atomic_store_explicit(&state,IDLE,memory_order_release);
 if(rc<0)fprintf(stderr,"Native audio playback failed: %d\n",rc);
 else if(++completed<=3)fprintf(stderr,"Native AI DMA complete %u\n",completed);
 return rc<0?rc:1;
}
void duck_vita_ai_vita_close(void){
 if(port>=0){
  duck_vita_ai_detach();
  atomic_store_explicit(&stopping,1,memory_order_release);
  if(started){
   sceKernelSignalSema(work,1);
   int rc=sceKernelWaitThreadEnd(worker,NULL,NULL);
   if(rc<0){fprintf(stderr,"Audio worker join failed: %d; retaining its resources\n",rc);return;}
   started=0;
  }
  if(worker>=0){sceKernelDeleteThread(worker);worker=-1;}
  if(work>=0){sceKernelDeleteSema(work);work=-1;}
  sceAudioOutOutput(port,0);
  int rc=sceAudioOutReleasePort(port);
  if(rc<0){fprintf(stderr,"Audio port release failed: %d\n",rc);return;}
  port=-1;
 }
}
int duck_vita_ai_vita_init(uint8_t *ram,unsigned clock){
 if(!ram||!clock||port>=0)return -1;
 port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,256,48000,SCE_AUDIO_OUT_MODE_STEREO);
 if(port<0)return port;
 memset(&resampler,0,sizeof resampler);
 submitted=completed=0;
 atomic_store(&state,IDLE);atomic_store(&stopping,0);
 work=sceKernelCreateSema("DuckAudioWork",0,0,1,NULL);
 if(work<0){int rc=work;duck_vita_ai_vita_close();return rc;}
 worker=sceKernelCreateThread("DuckAudio",playback,0x10000100,64*1024,0,0,NULL);
 if(worker<0){int rc=worker;duck_vita_ai_vita_close();return rc;}
 int result=sceKernelStartThread(worker,0,NULL);
 if(result<0){duck_vita_ai_vita_close();return result;}
 started=1;
 DuckAiAsyncSink sink={begin,poll,NULL};
 result=duck_vita_ai_init_async(ram,clock,&sink);
 if(result)duck_vita_ai_vita_close();
 else fprintf(stderr,"Native audio worker ready: guest playback is asynchronous\n");
 return result;
}
