#pragma once
#include <stddef.h>
#include <stdint.h>
#include "recomp.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Single guest instance, serviced on its CPU thread. ROM remains caller-owned
 * and big endian; RDRAM uses N64Recomp's word-swapped byte lanes. Reset before
 * replacing either allocation. No worker thread or cycle timing is implied. */
int duck_vita_os_io_reset(uint8_t *rdram, const uint8_t *rom, size_t rom_size);
/* Process at most one queued cartridge read. Returns 1 on transfer, 0 idle,
 * -1 malformed job/failed completion. Never fabricates successful DMA. */
int duck_vita_os_pi_service(void);
/* Send an explicitly raised hardware event using the registered guest queue.
 * Returns 0 delivered, -1 unregistered/full/invalid. No interrupt is invented. */
int duck_vita_os_event_emit(unsigned event);
void osCreatePiManager_recomp(uint8_t*, recomp_context*);
void osPiReadIo_recomp(uint8_t*, recomp_context*);
void osPiStartDma_recomp(uint8_t*, recomp_context*);
void osSetEventMesg_recomp(uint8_t*, recomp_context*);
#ifdef __cplusplus
}
#endif
