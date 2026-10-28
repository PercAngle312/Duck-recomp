#pragma once
#include "recomp.h"
#ifdef __cplusplus
extern "C" {
#endif
/* load: 0=loaded, 1=no image yet, -1=I/O/format failure. commit: 0=durable.
 * -2 means the new main image may be visible but durability is uncertain;
 * detach the guest device until an explicit reload rather than serve stale RAM.
 * A write is acknowledged only after the backend commits the complete image. */
enum {DUCK_EEPROM_COMMIT_UNCERTAIN=-2};
typedef struct {
 int (*load)(void *opaque,uint8_t *image,unsigned bytes);
 int (*commit)(void *opaque,const uint8_t *image,unsigned bytes);
 void *opaque;
} DuckEepromStorage;
int duck_vita_eeprom_init(unsigned bytes,const DuckEepromStorage *storage);
void duck_vita_eeprom_detach(void);
void osEepromProbe_recomp(uint8_t*,recomp_context*);
void osEepromLongRead_recomp(uint8_t*,recomp_context*);
void osEepromLongWrite_recomp(uint8_t*,recomp_context*);
/* Caller owns this path structure for the lifetime of the binding. */
typedef struct {char path[512],backup[520],temporary[520];} DuckEepromFile;
int duck_vita_eeprom_file(DuckEepromFile *file,const char *path,DuckEepromStorage *storage);
#ifdef __cplusplus
}
#endif
