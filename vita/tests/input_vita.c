#include "duck_vita_input.h"
#include <psp2/ctrl.h>
#include <assert.h>
#include <stdio.h>
static SceCtrlData sample;
static int configuration,result=1,polls,connected;
static uint16_t buttons;
static int8_t x,y;
int sceCtrlSetSamplingMode(int mode){assert(mode==SCE_CTRL_MODE_ANALOG);return configuration;}
int sceCtrlPeekBufferPositive(int port,SceCtrlData *out,int count){assert(!port&&count==1);polls++;*out=sample;return result;}
void duck_pad_update(int present,uint16_t b,int8_t sx,int8_t sy){connected=present;buttons=b;x=sx;y=sy;}
int main(void){
 sample=(SceCtrlData){.lx=128,.ly=128,.rx=128,.ry=128};
 assert(duck_vita_pad_poll()==1&&connected&&!buttons&&!x&&!y);
 unsigned vita[]={SCE_CTRL_CROSS,SCE_CTRL_CIRCLE,SCE_CTRL_LTRIGGER,SCE_CTRL_START,SCE_CTRL_UP,SCE_CTRL_DOWN,SCE_CTRL_LEFT,SCE_CTRL_RIGHT,SCE_CTRL_SELECT,SCE_CTRL_RTRIGGER};
 unsigned n64[]={0x8000,0x4000,0x2000,0x1000,0x800,0x400,0x200,0x100,0x20,0x10};
 for(unsigned i=0;i<10;i++){sample.buttons=vita[i];assert(duck_vita_pad_poll()==1&&buttons==n64[i]);}
 sample.buttons=0;
 for(unsigned i=116;i<=140;i++){sample.lx=sample.ly=(uint8_t)i;duck_vita_pad_poll();assert(!x&&!y);}
 int previous=-81;
 for(unsigned i=0;i<256;i++){sample.lx=sample.ly=(uint8_t)i;duck_vita_pad_poll();assert(x>=previous&&x>=-80&&x<=80&&y==-x);previous=x;}
 assert(x==80&&y==-80);sample.lx=sample.ly=0;duck_vita_pad_poll();assert(x==-80&&y==80);
 sample.rx=0;sample.ry=255;duck_vita_pad_poll();assert(buttons==(2|4));
 sample.rx=255;sample.ry=0;duck_vita_pad_poll();assert(buttons==(1|8));
 result=0;assert(!duck_vita_pad_poll()); // no new sample preserves last snapshot
 result=-1;assert(duck_vita_pad_poll()==-1&&!connected&&!buttons&&!x&&!y);
 int before=polls;configuration=-1;assert(duck_vita_pad_poll()==-1&&polls==before&&!connected);
 puts("PASS: all native button mappings, C-stick directions, neutral deadzone, full-range monotonic axes and SDK failures");return 0;
}
