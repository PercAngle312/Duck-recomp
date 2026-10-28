#define _POSIX_C_SOURCE 200809L
#include "duck_vita_eeprom.h"
#include <psp2/io/fcntl.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* Execute eeprom_file.c's __vita__ branch against real temporary files.
 * SDK handles differ from POSIX fds; reads/writes are deliberately short. */
enum {NONE,WRITE,SYNC_FILE,CLOSE_FILE,REMOVE_BACKUP,RENAME_OLD,RENAME_NEW,SYNC_DIRECTORY};
static int fault;
static char directory[]="/tmp/duck-vita-eeprom-XXXXXX";
static int fail(int operation){if(fault!=operation)return 0;fault=NONE;return 1;}
static int error(int result){return result<0?(errno==ENOENT?(int)0x80010002u:-5):result;}
static void path(const char *guest,char out[640]){assert(!strncmp(guest,"ux0:",4));snprintf(out,640,"%s%s",directory,guest+4);}
int sceIoOpen(const char *guest,int flags,unsigned mode){
 char p[640];path(guest,p);int f=flags&SCE_O_WRONLY?O_WRONLY:O_RDONLY;
 if(flags&SCE_O_CREAT)f|=O_CREAT;
 if(flags&SCE_O_TRUNC)f|=O_TRUNC;
 int fd=open(p,f,mode);return fd<0?error(fd):fd+1000;
}
int sceIoRead(int fd,void *data,unsigned bytes){assert(fd>=1000);return error((int)read(fd-1000,data,bytes>7?7:bytes));}
int sceIoWrite(int fd,const void *data,unsigned bytes){assert(fd>=1000);if(fail(WRITE))return -5;return error((int)write(fd-1000,data,bytes>7?7:bytes));}
int sceIoClose(int fd){assert(fd>=1000);int rc=close(fd-1000);return fail(CLOSE_FILE)?-5:error(rc);}
int sceIoRemove(const char *guest){
 char p[640];path(guest,p);if(strstr(guest,".bak")&&fail(REMOVE_BACKUP))return -5;return error(unlink(p));
}
int sceIoRename(const char *from,const char *to){
 char a[640],b[640];path(from,a);path(to,b);
 if(strstr(from,".tmp")&&fail(RENAME_NEW))return -5;
 if(strstr(to,".bak")&&fail(RENAME_OLD))return -5;
 return error(rename(a,b));
}
int sceIoSyncByFd(int fd,int flags){assert(fd>=1000&&!flags);return fail(SYNC_FILE)?-5:error(fsync(fd-1000));}
int sceIoSync(const char *guest,int flags){
 assert(!strcmp(guest,"ux0:")&&!flags);if(fail(SYNC_DIRECTORY))return -5;
 int fd=open(directory,O_RDONLY);assert(fd>=0);int rc=fsync(fd);assert(!close(fd));return error(rc);
}
static void write_pattern(uint8_t *ram,recomp_context *c,unsigned value){
 for(unsigned i=0;i<16;i++)ram[(0x1000+i)^3u]=(uint8_t)value;
 c->r5=0;c->r6=0xffffffff80001000ull;c->r7=16;osEepromLongWrite_recomp(ram,c);
}
static void read_pattern(uint8_t *ram,recomp_context *c,unsigned value){
 c->r5=0;c->r6=0x80001000;c->r7=16;osEepromLongRead_recomp(ram,c);assert(!c->r2);
 for(unsigned i=0;i<16;i++)assert(ram[(0x1000+i)^3u]==value);
}
int main(void){
 assert(mkdtemp(directory));uint8_t *ram=calloc(1,0x800000);assert(ram);recomp_context c={0};
 DuckEepromFile file;DuckEepromStorage storage;assert(!duck_vita_eeprom_file(&file,"ux0:/save.eep",&storage));
 assert(!duck_vita_eeprom_init(512,&storage));write_pattern(ram,&c,17);assert(!c.r2);
 for(int operation=WRITE;operation<SYNC_DIRECTORY;operation++){
  fault=operation;write_pattern(ram,&c,34);assert(c.r2==(gpr)-1&&!fault);
  read_pattern(ram,&c,17); // rejected transaction never changes active data
  assert(!duck_vita_eeprom_init(512,&storage));read_pattern(ram,&c,17);
 }
 fault=SYNC_DIRECTORY;write_pattern(ram,&c,34);assert(c.r2==(gpr)-1&&!fault);
 osEepromProbe_recomp(ram,&c);assert(!c.r2); // installed main may differ: detach stale RAM
 osEepromLongRead_recomp(ram,&c);assert(c.r2==(gpr)-1);
 assert(!duck_vita_eeprom_init(512,&storage));read_pattern(ram,&c,34);
 char native[640];path(file.path,native);assert(!unlink(native));
 assert(!duck_vita_eeprom_init(512,&storage));read_pattern(ram,&c,17); // recover backup
 FILE *bad=fopen(native,"wb");assert(bad);assert(fwrite("bad",1,3,bad)==3);assert(!fclose(bad));
 assert(duck_vita_eeprom_init(512,&storage)==-1); // do not replace malformed main
 assert(!unlink(native));path(file.backup,native);unlink(native);path(file.temporary,native);unlink(native);
 assert(!duck_vita_eeprom_file(&file,"ux0:/save16.eep",&storage));assert(!duck_vita_eeprom_init(2048,&storage));
 for(unsigned i=0;i<8;i++)ram[(0x1000+i)^3u]=(uint8_t)(i+70);
 c.r5=255;c.r6=0x80001000;c.r7=8;osEepromLongWrite_recomp(ram,&c);assert(!c.r2);
 assert(!duck_vita_eeprom_init(2048,&storage));osEepromLongRead_recomp(ram,&c);assert(!c.r2);
 for(unsigned i=0;i<8;i++)assert(ram[(0x1000+i)^3u]==i+70);
 path(file.path,native);assert(!unlink(native));assert(!rmdir(directory));free(ram);
 puts("PASS: native EEPROM short I/O, raw handles, six transaction failures, uncertain-sync detach, backup recovery and 16-Kbit last block");return 0;
}
