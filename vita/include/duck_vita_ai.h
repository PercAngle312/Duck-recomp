#pragma once
#include "recomp.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Synchronous sink must return only after the PCM buffer has been output.
 * Nonzero return is a real device failure; no AI completion is then sent. */
typedef int (*DuckAiOutput)(void*,const uint8_t*,uint32_t,unsigned,unsigned);
int duck_vita_ai_init(uint8_t *rdram,unsigned video_clock,DuckAiOutput output,void *opaque);
/* start copies/owns the input before returning 0; never retains live RDRAM.
 * poll: 0=pending, 1=playback complete, negative=device failure. Callbacks run
 * only on the guest host thread; a sink may have its own native output worker. */
typedef struct {
 DuckAiOutput start;int (*poll)(void*);void *opaque;
 /* Optional active-DMA progress, in guest bytes, aligned to eight bytes.
  * Excludes the queued second DMA. Zero is valid before IRQ service. */
 unsigned (*remaining_bytes)(void*);
 /* Zero/one keeps the single-active-DMA contract. Two allows start to own
  * both FIFO entries; poll still retires exactly one in playback order. */
 unsigned capacity;
} DuckAiAsyncSink;
int duck_vita_ai_init_async(uint8_t *rdram,unsigned video_clock,const DuckAiAsyncSink *sink);
/* Discard pending DMAs without completion IRQs on the guest host thread,
 * never from inside a sink call. The owner must separately stop/join any native
 * worker before releasing that worker's buffers or device resources. */
void duck_vita_ai_detach(void);
int duck_vita_ai_service(void);
int duck_vita_ai_error(void);
void osAiSetFrequency_recomp(uint8_t*,recomp_context*);
void osAiSetNextBuffer_recomp(uint8_t*,recomp_context*);
void osAiGetLength_recomp(uint8_t*,recomp_context*);
void osAiGetStatus_recomp(uint8_t*,recomp_context*);
enum { DUCK_VITA_AUDIO_BLOCK_FRAMES = 64 };
typedef struct {uint64_t phase;unsigned phase_rate;} DuckAudioResampler;
/* Produces stereo 48 kHz PCM from one word-swapped guest DMA. The caller
 * repeatedly supplies the same buffer until done; phase carries to the next. */
unsigned duck_vita_audio_resample(DuckAudioResampler*,const uint8_t*,uint32_t,unsigned,unsigned,int16_t*,unsigned,int*);
/* Optional already-owned next DMA's first stereo frame, word-swapped. It
 * supplies interpolation at the boundary; it never delays DMA completion. */
unsigned duck_vita_audio_resample_joined(DuckAudioResampler*,const uint8_t*,uint32_t,unsigned,unsigned,int16_t*,unsigned,int*,const uint8_t*);
int duck_vita_ai_vita_init(uint8_t *rdram,unsigned video_clock);
void duck_vita_ai_vita_close(void);
#ifdef __cplusplus
}
#endif
