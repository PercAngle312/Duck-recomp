#pragma once
#include "recomp.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Synchronous sink must return only after the PCM buffer has been output.
 * Nonzero return is a real device failure; no AI completion is then sent. */
typedef int (*DuckAiOutput)(void*,const uint8_t*,uint32_t,unsigned,unsigned);
int duck_vita_ai_init(uint8_t *rdram,unsigned video_clock,DuckAiOutput output,void *opaque);
int duck_vita_ai_service(void);
int duck_vita_ai_error(void);
void osAiSetFrequency_recomp(uint8_t*,recomp_context*);
void osAiSetNextBuffer_recomp(uint8_t*,recomp_context*);
void osAiGetLength_recomp(uint8_t*,recomp_context*);
void osAiGetStatus_recomp(uint8_t*,recomp_context*);
typedef struct {uint64_t phase;} DuckAudioResampler;
/* Produces stereo 48 kHz PCM from one word-swapped guest DMA. The caller
 * repeatedly supplies the same buffer until done; phase carries to the next. */
unsigned duck_vita_audio_resample(DuckAudioResampler*,const uint8_t*,uint32_t,unsigned,unsigned,int16_t*,unsigned,int*);
int duck_vita_ai_vita_init(uint8_t *rdram,unsigned video_clock);
void duck_vita_ai_vita_close(void);
#ifdef __cplusplus
}
#endif
