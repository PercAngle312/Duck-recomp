#pragma once
#include "recomp.h"
#ifdef __cplusplus
extern "C" {
#endif
void duck_vita_zero_bulk(uint8_t*,recomp_context*);
void duck_vita_copy_bulk(uint8_t*,recomp_context*);
void duck_vita_fill_bulk(uint8_t*,recomp_context*);
#ifdef __cplusplus
}
#endif
