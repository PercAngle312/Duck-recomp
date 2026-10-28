#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {unsigned queued_frames,underrun_blocks,padded_frames;} DuckVitaAudioStats;
/* Host-thread snapshot; the playback worker never calls into the renderer. */
void duck_vita_audio_stats(DuckVitaAudioStats *stats);
#ifdef __cplusplus
}
#endif
