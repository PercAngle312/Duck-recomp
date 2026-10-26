#pragma once
#include "recomp.h"
#ifdef __cplusplus
extern "C" {
#endif
/* load: 0=loaded, 1=no image yet, -1=I/O/format failure. commit: 0=durable.
 * A write is acknowledged only after the backend commits the complete image. */
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
