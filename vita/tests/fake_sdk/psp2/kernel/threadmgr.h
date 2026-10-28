#pragma once
typedef int (*DuckTestThreadEntry)(unsigned,void*);
#ifdef __cplusplus
extern "C" {
#endif
int sceKernelCreateSema(const char*,unsigned,int,int,void*);
int sceKernelWaitSema(int,int,unsigned*);
int sceKernelSignalSema(int,int);
int sceKernelDeleteSema(int);
int sceKernelCreateThread(const char*,DuckTestThreadEntry,int,unsigned,unsigned,int,const void*);
int sceKernelStartThread(int,unsigned,void*);
int sceKernelWaitThreadEnd(int,int*,unsigned*);
int sceKernelDeleteThread(int);
#ifdef __cplusplus
}
#endif
