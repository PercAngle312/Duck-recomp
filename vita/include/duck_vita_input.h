#pragma once
#include "recomp.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void duck_pad_reset(void);
void duck_pad_update(int connected,uint16_t buttons,int8_t x,int8_t y);
/* Poll real Vita controls, then complete one pending SI snapshot from the host
 * CPU service loop. A completion is emitted even for disconnected controllers. */
int duck_vita_pad_poll(void);
int duck_pad_service(void);
void osContInit_recomp(uint8_t*,recomp_context*);
void osContStartQuery_recomp(uint8_t*,recomp_context*);
void osContGetQuery_recomp(uint8_t*,recomp_context*);
void osContStartReadData_recomp(uint8_t*,recomp_context*);
void osContGetReadData_recomp(uint8_t*,recomp_context*);

#ifdef __cplusplus
}
#endif
