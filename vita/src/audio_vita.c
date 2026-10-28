/* Guest AI time is independent of presentation buffering. The main thread
 * owns synthesis and DMA deadlines; only the worker touches the speaker. */
#include "duck_vita_ai.h"
#include <psp2/audioout.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include "duck_vita_audio_stream.h"
#include "duck_vita_audio_stats.h"
static int port=-1,worker=-1,started;
static atomic_int stopping,failure;
static DuckAudioStream stream;
static DuckAudioResampler resampler;
static unsigned submitted,completed,read_dma,write_dma;
static struct {uint64_t begin,end;unsigned frames,rate;} dma[2];
static uint64_t next_deadline;
_Alignas(64) static int16_t pcm[2][DUCK_VITA_AUDIO_BLOCK_FRAMES*2];
static int playback(unsigned args,void *opaque){
 (void)args;(void)opaque;unsigned buffer=0;int primed=0;
 DuckAudioGapFade gap={0};
 while(!atomic_load_explicit(&stopping,memory_order_acquire)){
  unsigned queued=duck_audio_stream_queued(&stream);
  if(!primed){
   /* Enough lookahead for short renderer stalls. Short isolated sounds
    * also start after 80ms, so a one-DMA producer cannot deadlock. */
   unsigned age=(unsigned)sceKernelGetProcessTimeWide()-atomic_load_explicit(&stream.first_tick,memory_order_acquire);
   if(!queued||(queued<DUCK_AUDIO_PREFILL&&age<80000)){
    sceKernelDelayThread(1000);continue;
   }
   primed=1;
  }
  unsigned real=duck_audio_stream_take(&stream,pcm[buffer],DUCK_VITA_AUDIO_BLOCK_FRAMES);
  if(real<DUCK_VITA_AUDIO_BLOCK_FRAMES){
   memset(pcm[buffer]+real*2,0,(DUCK_VITA_AUDIO_BLOCK_FRAMES-real)*2*sizeof(int16_t));
   atomic_fetch_add_explicit(&stream.underruns,1,memory_order_relaxed);
   atomic_fetch_add_explicit(&stream.padded,DUCK_VITA_AUDIO_BLOCK_FRAMES-real,memory_order_relaxed);
   /* Keep the device paced after startup. Requiring another full prefill
    * here turns a short shortage into a recurring 64-80ms silence. */
   if(!real)gap.resume=16;
  }
  duck_vita_audio_gap_fade(&gap,pcm[buffer],real,DUCK_VITA_AUDIO_BLOCK_FRAMES);
  int rc=sceAudioOutOutput(port,pcm[buffer]);
  if(rc<0){atomic_store_explicit(&failure,rc,memory_order_release);return rc;}
  buffer^=1;
 }
 return 0;
}
static int begin(void *opaque,const uint8_t *ram,uint32_t address,unsigned frames,unsigned rate){
 (void)opaque;
 if(write_dma-read_dma>=2||atomic_load_explicit(&failure,memory_order_acquire))return -1;
 if(!ram||!frames||frames>0x3fff8u/4||!rate||(address&7u)||address>0x800000u-frames*4)return -1;
 uint64_t now=sceKernelGetProcessTimeWide();
 uint64_t begin=next_deadline>now?next_deadline:now;
 uint64_t duration=((uint64_t)frames*1000000+rate-1)/rate;
 dma[write_dma&1].begin=begin;dma[write_dma&1].end=begin+duration;
 dma[write_dma&1].frames=frames;dma[write_dma&1].rate=rate;
 next_deadline=begin+duration;
 /* Synthesis copies to host-owned PCM before the guest may reuse RDRAM.
  * A large legal DMA streams in chunks under bounded backpressure. */
 _Alignas(64) int16_t block[256*2];int done=0;
 while(!done){
  unsigned n=duck_vita_audio_resample(&resampler,ram,address,frames,rate,block,256,&done),at=0;
  if(!n&&!done)return -1;
  while(at<n){
   if(atomic_load_explicit(&failure,memory_order_acquire))return -1;
   unsigned pushed=duck_audio_stream_put(&stream,block+at*2,n-at,(unsigned)sceKernelGetProcessTimeWide());
   at+=pushed;if(at<n)sceKernelDelayThread(1000);
  }
 }
 write_dma++;
 if(++submitted<=3)fprintf(stderr,"Buffered AI DMA start %u: frames=%u rate=%u\n",submitted,frames,rate);
 return 0;
}
static int poll(void *opaque){
 (void)opaque;int rc=atomic_load_explicit(&failure,memory_order_acquire);
 if(rc<0)return rc;
 if(read_dma==write_dma||sceKernelGetProcessTimeWide()<dma[read_dma&1].end)return 0;
 /* Cap latency if game production runs ahead of the device. A completion
  * remains pending instead of dropping sound or growing an unbounded queue. */
 if(duck_audio_stream_queued(&stream)>DUCK_AUDIO_HIGH_WATER)return 0;
 read_dma++;completed++;
 if(!(completed%256))fprintf(stderr,"Buffered audio: completed=%u queued_frames=%u underruns=%u padded_frames=%u\n",completed,
 duck_audio_stream_queued(&stream),atomic_load(&stream.underruns),atomic_load(&stream.padded));
 return 1;
}
static unsigned remaining_bytes(void *opaque){
 (void)opaque;if(read_dma==write_dma)return 0;
 unsigned slot=read_dma&1;uint64_t now=sceKernelGetProcessTimeWide();
 if(now>=dma[slot].end)return 0;
 uint64_t left=now<=dma[slot].begin?dma[slot].frames:
  ((dma[slot].end-now)*dma[slot].rate+999999)/1000000;
 if(left>dma[slot].frames)left=dma[slot].frames;
 return ((unsigned)left*4+7)&~7u;
}
void duck_vita_ai_vita_close(void){
 if(port<0)return;
 duck_vita_ai_detach();atomic_store_explicit(&stopping,1,memory_order_release);
 if(started){int rc=sceKernelWaitThreadEnd(worker,NULL,NULL);
  if(rc<0){fprintf(stderr,"Audio worker join failed: %d; retaining resources\n",rc);return;}started=0;}
 if(worker>=0){sceKernelDeleteThread(worker);worker=-1;}
 sceAudioOutOutput(port,NULL);
 int rc=sceAudioOutReleasePort(port);if(rc<0){fprintf(stderr,"Audio port release failed: %d\n",rc);return;}port=-1;
}
int duck_vita_ai_vita_init(uint8_t *ram,unsigned clock){
 if(!ram||!clock||port>=0)return -1;
 port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,DUCK_VITA_AUDIO_BLOCK_FRAMES,48000,SCE_AUDIO_OUT_MODE_STEREO);
 if(port<0)return port;
 int volumes[2]={SCE_AUDIO_OUT_MAX_VOL,SCE_AUDIO_OUT_MAX_VOL};
 int rc=sceAudioOutSetVolume(port,SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH,volumes);
 if(rc<0){duck_vita_ai_vita_close();return rc;}
 memset(&resampler,0,sizeof resampler);duck_audio_stream_reset(&stream);
 submitted=completed=read_dma=write_dma=0;next_deadline=0;
 atomic_store(&stopping,0);atomic_store(&failure,0);
 /* Playback only blocks in sceAudioOutOutput and copies 256-frame blocks, so it
  * can safely outrank the game/render threads. Fall back to the default
  * priority if the SDK rejects the value. */
 int priority=0x10000080;
 worker=sceKernelCreateThread("DuckAudio",playback,priority,64*1024,0,0,NULL);
 if(worker<0){
  fprintf(stderr,"Audio priority 0x%x rejected: %d; trying default\n",priority,worker);
  priority=0x10000100;worker=sceKernelCreateThread("DuckAudio",playback,priority,64*1024,0,0,NULL);
 }
 if(worker<0){rc=worker;duck_vita_ai_vita_close();return rc;}
 rc=sceKernelStartThread(worker,0,NULL);
 if(rc<0){duck_vita_ai_vita_close();return rc;}started=1;
 DuckAiAsyncSink sink={begin,poll,NULL,remaining_bytes,2};
 rc=duck_vita_ai_init_async(ram,clock,&sink);if(rc)duck_vita_ai_vita_close();
 else fprintf(stderr,"Native audio: timed AI, bounded stereo FIFO, 64ms startup prefill, immediate underrun recovery, 256-frame output, priority=0x%x\n",priority);
 return rc;
}

void duck_vita_audio_stats(DuckVitaAudioStats *stats){
 if(!stats)return;
 stats->queued_frames=duck_audio_stream_queued(&stream);
 stats->underrun_blocks=atomic_load_explicit(&stream.underruns,memory_order_relaxed);
 stats->padded_frames=atomic_load_explicit(&stream.padded,memory_order_relaxed);
}
