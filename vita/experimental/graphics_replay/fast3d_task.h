#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Production-facing Fast3D adapter entry points. Runtime owns guest RDRAM.
 * Submit exactly one graphics OSTask. Return 0 after rendering with FullSync,
 * or 1 after rendering without FullSync; negative values are failures; never acknowledge an unsupported task.
 * The guest SP/DP scheduler still needs to invoke this interface.
 */
int duck_fast3d_init(uint8_t *rdram);
int duck_fast3d_submit_task(uint32_t task_guest_address);
/* Historical fixed-snapshot caller, retained until it is removed. */
int duck_fast3d_replay(uint32_t task_guest_address);
#ifdef __cplusplus
}
#endif
