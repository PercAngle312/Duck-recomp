#include "duck_vita_ai.h"
#include <psp2/audioout.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static int open_error,config_error,output_error,drain_error;
static unsigned configured,outputs,drains,released,interrupts,frames;
int sceAudioOutOpenPort(SceAudioOutPortType type,int len,int rate,SceAudioOutMode mode){
 assert(type==SCE_AUDIO_OUT_PORT_TYPE_MAIN&&len==256&&rate==48000&&mode==SCE_AUDIO_OUT_MODE_STEREO);
 return open_error?open_error:7;
}
int sceAudioOutSetConfig(int port,unsigned len,int rate,SceAudioOutMode mode){
 assert(port==7&&len&&len<=256&&!(len%64)&&rate==-1);
 /* A cast of -1 to the small SDK enum becomes 255: this caught a real bug. */
 assert(mode==SCE_AUDIO_OUT_MODE_STEREO);
 configured++;frames=len;return config_error;
}
int sceAudioOutOutput(int port,const void *buffer){
 assert(port==7);
 if(!buffer){drains++;return drain_error;}
 assert(!((uintptr_t)buffer%64));
 const int16_t *pcm=buffer;
 /* Constant stereo survives resampling; padding must be silent. */
 assert(pcm[0]==12000&&pcm[1]==-12000);
 for(unsigned i=2;i<frames;i++)assert((pcm[i*2]==12000&&pcm[i*2+1]==-12000)||(!pcm[i*2]&&!pcm[i*2+1]));
 outputs++;return output_error;
}
int sceAudioOutReleasePort(int port){assert(port==7);released++;return 0;}
int duck_vita_os_event_emit(unsigned event){assert(event==6);interrupts++;return 0;}
static void queue(uint8_t *ram){
 recomp_context c={0};c.r4=32000;osAiSetFrequency_recomp(ram,&c);assert(c.r2>31000&&c.r2<33000);
 c.r4=0xffffffff80001000ull;c.r5=16;osAiSetNextBuffer_recomp(ram,&c);assert(!c.r2);
}
int main(void){
 assert(sizeof(SceAudioOutMode)==1);
 uint8_t *ram=calloc(1,0x800000);assert(ram);
 for(unsigned i=0;i<4;i++)for(unsigned ch=0;ch<2;ch++){
  unsigned value=(uint16_t)(ch?-12000:12000),at=0x1000+i*4+ch*2;
  ram[at^3u]=(uint8_t)(value>>8);ram[(at+1)^3u]=(uint8_t)value;
 }
 open_error=-11;assert(duck_vita_ai_vita_init(ram,48681812)==-11);open_error=0;
 assert(!duck_vita_ai_vita_init(ram,48681812));
 assert(duck_vita_ai_vita_init(ram,48681812)==-1);
 queue(ram);assert(duck_vita_ai_service()==1);
 assert(configured==1&&outputs==1&&drains==1&&interrupts==1);
 duck_vita_ai_vita_close();assert(released==1);
 for(unsigned failure=0;failure<3;failure++){
  assert(!duck_vita_ai_vita_init(ram,48681812));queue(ram);
  unsigned previous_irq=interrupts;
  config_error=failure==0?-12:0;output_error=failure==1?-13:0;drain_error=failure==2?-14:0;
  assert(duck_vita_ai_service()==-1&&interrupts==previous_irq);
  assert(duck_vita_ai_error()==-(12+(int)failure));
  duck_vita_ai_vita_close();config_error=output_error=drain_error=0;
 }
 assert(released==4);duck_vita_ai_vita_close();assert(released==4);
 free(ram);puts("PASS: native audio small-enum ABI, PCM/padding, completion ordering, device errors and cleanup");return 0;
}
