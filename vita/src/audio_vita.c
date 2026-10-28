#include "duck_vita_ai.h"
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
static int port=-1,worker=-1,work=-1,started;
static atomic_int stopping,failure;
/* Host owns tail/head. Worker owns rendering cursor; finished publishes only
 * samples which the device has finished playing, never merely queued data. */
static atomic_uint head,tail,finished,padded_frames,idle_drains;
static unsigned submitted,completed;
static struct {
 unsigned frames,rate;
 atomic_uint remaining;
 _Alignas(64) uint8_t snapshot[0x40000];
} jobs[2];
static DuckAudioResampler resampler;
_Alignas(64) static int16_t pcm[2][DUCK_VITA_AUDIO_BLOCK_FRAMES*2];
typedef struct {unsigned sequence,remaining;int done;} Mark;
static void publish(const Mark *marks,unsigned count){
 for(unsigned i=0;i<count;i++){
  const Mark *m=&marks[i];
  atomic_store_explicit(&jobs[m->sequence&1].remaining,m->remaining,memory_order_release);
  if(m->done)atomic_store_explicit(&finished,m->sequence+1,memory_order_release);
 }
}
static int playback(unsigned args,void *opaque){
 (void)args;(void)opaque;
 unsigned cursor=0,buffer=0,previous_count=0;
 int active=0;
 Mark previous[2];
 DuckAudioGapFade gap={0};
 for(;;){
  if(atomic_load_explicit(&stopping,memory_order_acquire))return 0;
  if(atomic_load_explicit(&failure,memory_order_acquire))return -1;
  if(!active&&cursor==atomic_load_explicit(&tail,memory_order_acquire)){
   int rc=sceKernelWaitSema(work,1,NULL);
   if(rc<0){atomic_store_explicit(&failure,rc,memory_order_release);return rc;}
   /* A consumed wake token belongs to the next job. No condition-variable
    * wakeups are assumed: each begin signals exactly one semaphore token. */
   if(atomic_load_explicit(&stopping,memory_order_acquire))return 0;
   if(atomic_load_explicit(&failure,memory_order_acquire))return -1;
   if(cursor==atomic_load_explicit(&tail,memory_order_acquire))continue;
   active=1;
  }
  unsigned filled=0,count=0;
  Mark marks[2];
  while(filled<DUCK_VITA_AUDIO_BLOCK_FRAMES){
   if(!active){
    if(cursor==atomic_load_explicit(&tail,memory_order_acquire))break;
    int rc=sceKernelWaitSema(work,1,NULL);
    if(rc<0){atomic_store_explicit(&failure,rc,memory_order_release);return rc;}
    if(atomic_load_explicit(&stopping,memory_order_acquire))return 0;
    if(atomic_load_explicit(&failure,memory_order_acquire))return -1;
    active=1;
   }
   unsigned slot=cursor&1,frames=jobs[slot].frames;int done=0;
   // Tail release/acquire publishes an immutable next snapshot. Never wait
   // for a second DMA: a single-buffer producer may need this completion.
   const uint8_t *next_frame=atomic_load_explicit(&tail,memory_order_acquire)-cursor>1?
       jobs[(cursor+1)&1].snapshot:NULL;
   unsigned n=duck_vita_audio_resample_joined(&resampler,jobs[slot].snapshot,0,
       frames,jobs[slot].rate,pcm[buffer]+filled*2,DUCK_VITA_AUDIO_BLOCK_FRAMES-filled,&done,next_frame);
   if(!n&&!done){atomic_store_explicit(&failure,-1,memory_order_release);return -1;}
   filled+=n;
   unsigned consumed=(unsigned)(resampler.phase/48000);
   unsigned bytes=done?0:(frames-(consumed>frames?frames:consumed))*4;
   marks[count++]=(Mark){cursor,(bytes+7u)&~7u,done};
   if(done){cursor++;active=0;}else break;
  }
  if(!filled){
   // At high guest rates a short DMA can produce no new 48 kHz sample.
   // Its time is covered by the prior sample; retire it after queued output,
   // rather than losing its completion markers or manufacturing extra PCM.
   if(count){
    if(previous_count){
     int rc=sceAudioOutOutput(port,NULL);
     if(rc<0){atomic_store_explicit(&failure,rc,memory_order_release);return rc;}
     publish(previous,previous_count);previous_count=0;
    }
    publish(marks,count);
   }
   continue;
  }
  /* Fixed-size output joins adjacent queued DMAs in the same native block.
   * Pad only on actual producer starvation, not at every guest boundary. */
  if(filled<DUCK_VITA_AUDIO_BLOCK_FRAMES){
   memset(pcm[buffer]+filled*2,0,(DUCK_VITA_AUDIO_BLOCK_FRAMES-filled)*2*sizeof(int16_t));
   atomic_fetch_add_explicit(&padded_frames,DUCK_VITA_AUDIO_BLOCK_FRAMES-filled,memory_order_relaxed);
  }
  duck_vita_audio_gap_fade(&gap,pcm[buffer],filled,DUCK_VITA_AUDIO_BLOCK_FRAMES);
  int rc=sceAudioOutOutput(port,pcm[buffer]);
  if(rc<0){atomic_store_explicit(&failure,rc,memory_order_release);return rc;}
  publish(previous,previous_count);
  memcpy(previous,marks,count*sizeof(Mark));previous_count=count;buffer^=1;
  if(!active&&cursor==atomic_load_explicit(&tail,memory_order_acquire)){
   /* No queued guest work remains. Drain the last block so its completion
    * can wake the guest; do not invent completion just to keep audio moving. */
   rc=sceAudioOutOutput(port,NULL);
   if(rc<0){atomic_store_explicit(&failure,rc,memory_order_release);return rc;}
   atomic_fetch_add_explicit(&idle_drains,1,memory_order_relaxed);
   publish(previous,previous_count);previous_count=0;
  }
 }
}
static int begin(void *opaque,const uint8_t *ram,uint32_t address,unsigned frames,unsigned rate){
 (void)opaque;
 unsigned next=atomic_load_explicit(&tail,memory_order_relaxed);
 if(next-atomic_load_explicit(&head,memory_order_acquire)>=2||
    atomic_load_explicit(&failure,memory_order_acquire))return -1;
 if(!ram||!frames||frames>0x3fff8u/4||!rate||(address&7u)||address>0x800000u-frames*4)return -1;
 unsigned slot=next&1;
 memcpy(jobs[slot].snapshot,ram+address,frames*4);jobs[slot].frames=frames;jobs[slot].rate=rate;
 atomic_store_explicit(&jobs[slot].remaining,frames*4,memory_order_release);
 atomic_store_explicit(&tail,next+1,memory_order_release);
 int rc=sceKernelSignalSema(work,1);
 if(rc<0){atomic_store_explicit(&failure,rc,memory_order_release);return rc;}
 if(++submitted<=3)fprintf(stderr,"Native AI DMA start %u: frames=%u rate=%u\n",submitted,frames,rate);
 return 0;
}
static int poll(void *opaque){
 (void)opaque;
 unsigned first=atomic_load_explicit(&head,memory_order_relaxed);
 if(first!=atomic_load_explicit(&finished,memory_order_acquire)){
  atomic_store_explicit(&head,first+1,memory_order_release);
  if(++completed<=3)fprintf(stderr,"Native AI DMA complete %u\n",completed);
  if(!(completed%256))fprintf(stderr,"Audio stream: completed=%u padded_frames=%u idle_drains=%u queued=%u\n",
    completed,atomic_load(&padded_frames),atomic_load(&idle_drains),atomic_load(&tail)-(first+1));
  return 1;
 }
 int rc=atomic_load_explicit(&failure,memory_order_acquire);
 if(rc<0)fprintf(stderr,"Native audio playback failed: %d\n",rc);
 return rc;
}
static unsigned remaining_bytes(void *opaque){
 (void)opaque;unsigned first=atomic_load_explicit(&head,memory_order_relaxed);
 return atomic_load_explicit(&jobs[first&1].remaining,memory_order_acquire);
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
 port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,DUCK_VITA_AUDIO_BLOCK_FRAMES,48000,SCE_AUDIO_OUT_MODE_STEREO);
 if(port<0)return port;
 int volumes[2]={SCE_AUDIO_OUT_MAX_VOL,SCE_AUDIO_OUT_MAX_VOL};
 int volume_result=sceAudioOutSetVolume(port,SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH,volumes);
 if(volume_result<0){duck_vita_ai_vita_close();return volume_result;}
 memset(&resampler,0,sizeof resampler);submitted=completed=0;
 atomic_store(&stopping,0);atomic_store(&failure,0);
 atomic_store(&head,0);atomic_store(&tail,0);atomic_store(&finished,0);
 atomic_store(&padded_frames,0);atomic_store(&idle_drains,0);
 work=sceKernelCreateSema("DuckAudioWork",0,0,2,NULL);
 if(work<0){int rc=work;duck_vita_ai_vita_close();return rc;}
 worker=sceKernelCreateThread("DuckAudio",playback,0x10000100,64*1024,0,0,NULL);
 if(worker<0){int rc=worker;duck_vita_ai_vita_close();return rc;}
 int result=sceKernelStartThread(worker,0,NULL);
 if(result<0){duck_vita_ai_vita_close();return result;}
 started=1;
 DuckAiAsyncSink sink={begin,poll,NULL,remaining_bytes,2};
 result=duck_vita_ai_init_async(ram,clock,&sink);
 if(result)duck_vita_ai_vita_close();
 else fprintf(stderr,"Native audio worker ready: two-DMA stream, fixed 64-frame stereo output\n");
 return result;
}
