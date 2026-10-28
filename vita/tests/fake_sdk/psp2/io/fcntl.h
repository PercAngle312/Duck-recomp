#pragma once
enum {SCE_O_RDONLY=1,SCE_O_WRONLY=2,SCE_O_CREAT=0x200,SCE_O_TRUNC=0x400};
int sceIoOpen(const char*,int,unsigned);
int sceIoRead(int,void*,unsigned);
int sceIoWrite(int,const void*,unsigned);
int sceIoClose(int);
int sceIoRemove(const char*);
int sceIoRename(const char*,const char*);
int sceIoSyncByFd(int,int);
int sceIoSync(const char*,int);
