#define _POSIX_C_SOURCE 200809L
#include "duck_vita_eeprom.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static int reject(void *opaque,const uint8_t *data,unsigned bytes){(void)opaque;(void)data;(void)bytes;return -1;}
static int commits;
static int (*real_commit)(void*,const uint8_t*,unsigned);
static int counted(void *opaque,const uint8_t *data,unsigned bytes){commits++;return real_commit(opaque,data,bytes);}
int main(void){
 char directory[]="/tmp/duck-eeprom-XXXXXX",path[256];assert(mkdtemp(directory));snprintf(path,sizeof path,"%s/game.eep",directory);
 uint8_t *ram=calloc(1,0x800000);assert(ram);DuckEepromFile file;DuckEepromStorage storage;
 assert(!duck_vita_eeprom_file(&file,path,&storage));real_commit=storage.commit;storage.commit=counted;
 assert(!duck_vita_eeprom_init(512,&storage));
 recomp_context c={0};osEepromProbe_recomp(ram,&c);assert(c.r2==1);
 c.r5=63;c.r6=0xffffffff80002001ull;c.r7=8;
 osEepromLongRead_recomp(ram,&c);assert(!c.r2);for(unsigned i=0;i<8;i++)assert(ram[(0x2001+i)^3u]==255);
 osEepromLongWrite_recomp(ram,&c);assert(!c.r2&&commits==1); // first blank write still persists
 for(unsigned i=0;i<8;i++)ram[(0x2001+i)^3u]=(uint8_t)(i+1);
 osEepromLongWrite_recomp(ram,&c);assert(!c.r2&&commits==2);
 osEepromLongWrite_recomp(ram,&c);assert(!c.r2&&commits==2); // no redundant SD rewrite
 duck_vita_eeprom_detach();assert(!duck_vita_eeprom_init(512,&storage));memset(ram+0x2000,0,16);
 osEepromLongRead_recomp(ram,&c);for(unsigned i=0;i<8;i++)assert(ram[(0x2001+i)^3u]==i+1);
 DuckEepromStorage failing=storage;failing.commit=reject;assert(!duck_vita_eeprom_init(512,&failing));
 ram[0x2001^3u]=99;osEepromLongWrite_recomp(ram,&c);assert(c.r2==(gpr)-1);
 osEepromLongRead_recomp(ram,&c);assert(!c.r2&&ram[0x2001^3u]==1);
 assert(!duck_vita_eeprom_init(512,&storage));ram[0x2001^3u]=2;osEepromLongWrite_recomp(ram,&c);assert(!c.r2);
 assert(!unlink(path));assert(!duck_vita_eeprom_init(512,&storage)); // recover interrupted commit from backup
 osEepromLongRead_recomp(ram,&c);assert(ram[0x2001^3u]==1);
 FILE *bad=fopen(path,"wb");assert(bad);assert(fwrite("bad",1,3,bad)==3);assert(!fclose(bad));
 assert(duck_vita_eeprom_init(512,&storage)==-1); // malformed main is never silently reset
 c.r5=64;osEepromLongRead_recomp(ram,&c);assert(c.r2==(gpr)-1);
 c.r5=0;c.r6=0x807fffffull;c.r7=8;osEepromLongRead_recomp(ram,&c);assert(c.r2==(gpr)-1);
 c.r6=0x80002000;c.r7=7;osEepromLongRead_recomp(ram,&c);assert(c.r2==(gpr)-1);
 duck_vita_eeprom_detach();osEepromProbe_recomp(ram,&c);assert(!c.r2);
 unlink(path);unlink(file.backup);unlink(file.temporary);rmdir(directory);free(ram);
 puts("PASS: EEPROM byte lanes, end-of-image bounds, persistent reload, rejected writes, backup recovery and malformed-image protection");return 0;
}
