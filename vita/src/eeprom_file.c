/* Whole-image EEPROM transaction: synced temporary -> backup -> main.
 * Load may recover the backup when main is absent after an interrupted rename.
 * A malformed existing main is rejected instead of silently erased/replaced.
 * Raw Vita I/O handles must not be confused with newlib's descriptor table. */
#include "duck_vita_eeprom.h"
#include <string.h>
#include <stdio.h>
#ifdef __vita__
#include <psp2/io/fcntl.h>
#define SAVE_OPEN_R(p) sceIoOpen(p,SCE_O_RDONLY,0)
#define SAVE_OPEN_W(p) sceIoOpen(p,SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666)
#define SAVE_READ sceIoRead
#define SAVE_WRITE sceIoWrite
#define SAVE_CLOSE sceIoClose
#define SAVE_REMOVE sceIoRemove
#define SAVE_RENAME sceIoRename
#define SAVE_SYNC(fd) sceIoSyncByFd(fd,0)
static int absent(int result){return result==(int)0x80010002u;}
#else
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define SAVE_OPEN_R(p) open(p,O_RDONLY)
#define SAVE_OPEN_W(p) open(p,O_WRONLY|O_CREAT|O_TRUNC,0600)
#define SAVE_READ read
#define SAVE_WRITE write
#define SAVE_CLOSE close
#define SAVE_REMOVE unlink
#define SAVE_RENAME rename
#define SAVE_SYNC fsync
static int absent(int result){return result<0&&errno==ENOENT;}
#endif
static int read_image(const char *path,uint8_t *image,unsigned bytes){
 int fd=SAVE_OPEN_R(path);if(fd<0)return absent(fd)?1:-1;
 unsigned done=0;int error=0;
 while(done<bytes){int n=(int)SAVE_READ(fd,image+done,bytes-done);if(n<=0){error=1;break;}done+=(unsigned)n;}
 uint8_t extra;if(!error&&SAVE_READ(fd,&extra,1)!=0)error=1;
 if(SAVE_CLOSE(fd)<0)error=1;
 return error?-1:0;
}
static int load(void *opaque,uint8_t *image,unsigned bytes){
 DuckEepromFile *file=opaque;int result=read_image(file->path,image,bytes);
 return result==1?read_image(file->backup,image,bytes):result;
}
static int sync_directory(const char *path){
 char dir[512];size_t n=strlen(path);if(n>=sizeof dir)return -1;memcpy(dir,path,n+1);
#ifdef __vita__
 char *colon=strchr(dir,':');if(!colon)return -1;colon[1]=0;return sceIoSync(dir,0);
#else
 char *slash=strrchr(dir,'/');if(slash){if(slash==dir)slash[1]=0;else *slash=0;}else strcpy(dir,".");
 int fd=open(dir,O_RDONLY);if(fd<0)return -1;int result=fsync(fd);if(close(fd)<0)result=-1;return result;
#endif
}
static int commit(void *opaque,const uint8_t *image,unsigned bytes){
 DuckEepromFile *file=opaque;
 int fd=SAVE_OPEN_W(file->temporary);if(fd<0)return -1;
 unsigned done=0;int error=0;
 while(done<bytes){int n=(int)SAVE_WRITE(fd,image+done,bytes-done);if(n<=0){error=1;break;}done+=(unsigned)n;}
 if(!error&&SAVE_SYNC(fd)<0)error=1;
 if(SAVE_CLOSE(fd)<0)error=1;
 if(error){SAVE_REMOVE(file->temporary);return -1;}
 int old=SAVE_OPEN_R(file->path);
 if(old<0&&!absent(old)){SAVE_REMOVE(file->temporary);return -1;}
 if(old>=0){
  if(SAVE_CLOSE(old)<0){SAVE_REMOVE(file->temporary);return -1;}
  int result=SAVE_REMOVE(file->backup);
  if(result<0&&!absent(result)){SAVE_REMOVE(file->temporary);return -1;}
  if(SAVE_RENAME(file->path,file->backup)<0){SAVE_REMOVE(file->temporary);return -1;}
 }
 if(SAVE_RENAME(file->temporary,file->path)<0){
  if(old>=0)SAVE_RENAME(file->backup,file->path);
  SAVE_REMOVE(file->temporary);return -1;
 }
 return sync_directory(file->path)<0?-1:0;
}
int duck_vita_eeprom_file(DuckEepromFile *file,const char *path,DuckEepromStorage *storage){
 if(!file||!path||!storage||!path[0]||strlen(path)>=sizeof file->path)return -1;
 strcpy(file->path,path);snprintf(file->backup,sizeof file->backup,"%s.bak",path);
 snprintf(file->temporary,sizeof file->temporary,"%s.tmp",path);
 *storage=(DuckEepromStorage){load,commit,file};return 0;
}
