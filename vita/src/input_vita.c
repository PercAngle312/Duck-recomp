#include "duck_vita_input.h"
#include <psp2/ctrl.h>
static int8_t axis(unsigned value,int invert){
 int n=((int)value-128)*80/128;if(invert)n=-n;
 if(n>80)n=80;
 if(n< -80)n=-80;
 return (int8_t)n;
}
int duck_vita_pad_poll(void){
 SceCtrlData pad={0};sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
 int result=sceCtrlPeekBufferPositive(0,&pad,1);
 if(result<0){duck_pad_update(0,0,0,0);return -1;}
 if(!result)return 0;
 uint16_t b=0;
 if(pad.buttons&SCE_CTRL_CROSS)b|=0x8000;
 if(pad.buttons&SCE_CTRL_CIRCLE)b|=0x4000;
 if(pad.buttons&SCE_CTRL_LTRIGGER)b|=0x2000;
 if(pad.buttons&SCE_CTRL_START)b|=0x1000;
 if(pad.buttons&SCE_CTRL_UP)b|=0x0800;
 if(pad.buttons&SCE_CTRL_DOWN)b|=0x0400;
 if(pad.buttons&SCE_CTRL_LEFT)b|=0x0200;
 if(pad.buttons&SCE_CTRL_RIGHT)b|=0x0100;
 if(pad.buttons&SCE_CTRL_SELECT)b|=0x0020;
 if(pad.buttons&SCE_CTRL_RTRIGGER)b|=0x0010;
 if(pad.ry<80)b|=8;
 if(pad.ry>176)b|=4;
 if(pad.rx<80)b|=2;
 if(pad.rx>176)b|=1;
 duck_pad_update(1,b,axis(pad.lx,0),axis(pad.ly,1));return 1;
}
