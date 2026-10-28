// Host test declarations only. Native builds use actual VitaSDK headers.
#pragma once
using SceUID = int;
using SceUInt = unsigned int;
extern "C" {
SceUID sceKernelCreateSema(const char*, SceUInt, int, int, void*);
int sceKernelDeleteSema(SceUID);
int sceKernelWaitSema(SceUID, int, SceUInt*);
int sceKernelPollSema(SceUID, int);
int sceKernelSignalSema(SceUID, int);
}
