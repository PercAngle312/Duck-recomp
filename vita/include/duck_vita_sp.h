#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*DuckVitaGraphicsSubmit)(uint32_t task_address);
typedef void (*DuckVitaSpCompletion)(void *opaque, int success);
typedef struct {
 DuckVitaGraphicsSubmit submit;
 DuckVitaSpCompletion complete;
 DuckVitaGraphicsSubmit audio_submit;
 DuckVitaSpCompletion audio_complete;
 void *opaque;
 unsigned submitted;
 unsigned completed;
 unsigned failed;
 int busy;
} DuckVitaSp;
/* Renderer returns 1 for a successful task with no RDP FullSync. */
enum { DUCK_SP_GRAPHICS_NO_DP=1 };
enum { DUCK_SP_BAD_ARGUMENT=-1, DUCK_SP_BUSY=-2, DUCK_SP_RENDERER_UNAVAILABLE=-3, DUCK_SP_AUDIO_UNAVAILABLE=-4 };
void duck_vita_sp_init(DuckVitaSp *sp, DuckVitaGraphicsSubmit submit,
                       DuckVitaSpCompletion complete, void *opaque);
/* Called with a live, validated guest OSTask address from osSpTaskStartGo.
 * Synchronous renderer submission until guest SP/DP asynchronous scheduling lands.
 * Never fires successful completion when renderer rejects or fails the task.
 */
int duck_vita_sp_start_graphics(DuckVitaSp *sp, uint32_t guest_task_address);
int duck_vita_sp_set_audio(DuckVitaSp*,DuckVitaGraphicsSubmit,DuckVitaSpCompletion);
int duck_vita_sp_start_audio(DuckVitaSp*,uint32_t task_address);
#ifdef __cplusplus
}
#endif
