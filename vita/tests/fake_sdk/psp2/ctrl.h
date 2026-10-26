#pragma once
#include <stdint.h>
/* Only the fields and constants used by the host controller contract test. */
typedef struct {uint32_t buttons;uint8_t lx,ly,rx,ry;} SceCtrlData;
enum {SCE_CTRL_MODE_ANALOG=1,SCE_CTRL_SELECT=1,SCE_CTRL_START=8,
 SCE_CTRL_UP=0x10,SCE_CTRL_RIGHT=0x20,SCE_CTRL_DOWN=0x40,SCE_CTRL_LEFT=0x80,
 SCE_CTRL_LTRIGGER=0x100,SCE_CTRL_RTRIGGER=0x200,SCE_CTRL_CIRCLE=0x2000,SCE_CTRL_CROSS=0x4000};
int sceCtrlSetSamplingMode(int);
int sceCtrlPeekBufferPositive(int,SceCtrlData*,int);
