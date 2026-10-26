#pragma once
#include <stdint.h>
#include "duck_vita_sp.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Binding for guest libultra osSpTaskStartGo. SP owns the renderer;
 * completion events are dispatched by the owning runtime after submission.
 * The callback is void in libultra: retrieve errors through last_result.
 */
void duck_vita_sp_os_bind(DuckVitaSp *sp, uint8_t *rdram);
int duck_vita_sp_os_last_result(void);
#ifdef __cplusplus
}
#endif
