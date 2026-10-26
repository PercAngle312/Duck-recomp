#include "duck_vita_ai.h"
#include <psp2/audioout.h>
#include <string.h>
static int port=-1;
static DuckAudioResampler resampler;
_Alignas(64) static int16_t pcm[256*2];
static int output(void *opaque,const uint8_t *ram,uint32_t address,unsigned frames,unsigned rate){
 (void)opaque;int done=0;
 while(!done){
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
void duck_vita_ai_vita_close(void){
 if(port>=0){sceAudioOutOutput(port,0);sceAudioOutReleasePort(port);port=-1;}
}
int duck_vita_ai_vita_init(uint8_t *ram,unsigned clock){
 if(!ram||!clock||port>=0)return -1;
 port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,256,48000,SCE_AUDIO_OUT_MODE_STEREO);
 if(port<0)return port;
 memset(&resampler,0,sizeof resampler);
 int result=duck_vita_ai_init(ram,clock,output,0);
 if(result)duck_vita_ai_vita_close();
 return result;
}
