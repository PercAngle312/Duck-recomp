#pragma once
#include "duck_vita_sp_runtime.h"
#include "duck_vita_dispatch.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Live game runtime owner: its RDRAM, scheduler and translated contexts must
 * refer to the same game instance. No fabricated function pointer fallback.
 */
typedef struct {
 DuckVitaScheduler *scheduler;
 DuckVitaRecompStore *contexts;
 const DuckVitaDispatchTable *functions;
 uint8_t *rdram;
 DuckVitaSp sp;
 DuckVitaSpEventState events;
 int attached;
} DuckVitaGameLoop;
/* Renderer must already be initialized for the supplied RDRAM. */
int duck_vita_game_loop_attach(DuckVitaGameLoop *loop, DuckVitaScheduler *scheduler,
 DuckVitaRecompStore *contexts, const DuckVitaDispatchTable *functions,
 uint8_t *rdram, DuckVitaGraphicsSubmit renderer);
/* Invokes only a verified translated address on the selected guest thread.
 * The dispatch layer handles choosing a ready thread and saving its context.
 */
int duck_vita_game_loop_step(DuckVitaGameLoop *loop, uint32_t verified_vram);
void duck_vita_game_loop_detach(DuckVitaGameLoop *loop);
#ifdef __cplusplus
}
#endif
