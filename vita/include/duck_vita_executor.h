#pragma once
#include "duck_vita_context.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Generated-code integration seam: caller supplies a translated instruction
 * dispatcher that operates on a guest context. No MIPS ABI compatibility is
 * implied until an actual generated function is connected and tested. */
typedef int (*DuckVitaGuestStep)(DuckVitaGuestContext *state,void *opaque);
enum { DUCK_EXEC_NO_THREAD=-1, DUCK_EXEC_NO_CONTEXT=-2, DUCK_EXEC_BAD_ARG=-3 };
int duck_vita_execute_step(DuckVitaScheduler *scheduler,DuckVitaContextStore *store,
                           DuckVitaGuestStep step,void *opaque);
#ifdef __cplusplus
}
#endif
