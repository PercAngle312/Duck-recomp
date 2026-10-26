#pragma once
#include <stdint.h>
#include "duck_vita_sp.h"
#include "duck_vita_sp_events.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Call only after initializing guest RDRAM and libultra's guest event queues.
 * The renderer must already be initialized with this same RDRAM allocation.
 * Installs SP/DP completion and binds osSpTaskStartGo_recomp to the service.
 */
int duck_vita_sp_runtime_attach(DuckVitaSp *sp, DuckVitaSpEventState *events,
                                uint8_t *rdram, DuckVitaGraphicsSubmit renderer);
void duck_vita_sp_runtime_detach(DuckVitaSp *sp);
#ifdef __cplusplus
}
#endif
