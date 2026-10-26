#include "duck_vita_input.h"
#include <psp2/ctrl.h>
static uint16_t c_buttons;
/* Separate press/release thresholds prevent camera chatter near the edge.
 * Opposite directions release before engaging, including a one-sample flip. */
static uint16_t c_axis(unsigned value,uint16_t previous,uint16_t low,uint16_t high){
 if(value<80)return low;
 if(value>176)return high;
 if((previous&low)&&value<96)return low;
 if((previous&high)&&value>160)return high;
 return 0;
}
static int8_t axis(unsigned value,int invert){
 /* Ignore small physical-stick drift, then retain the entire N64 range.
  * Vita's positive half has 127 steps, its negative half has 128. */
 int n=(int)value-128,negative=n<0;
 int magnitude=negative?-n:n;
 if(magnitude<=12)return 0;
 n=(magnitude-12)*80/((negative?128:127)-12);
 if(n>80)n=80;
 if(negative)n=-n;
 if(invert)n=-n;
 return (int8_t)n;
}
int duck_vita_pad_poll(void){
 SceCtrlData pad={0};
 if(sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG)<0){c_buttons=0;duck_pad_update(0,0,0,0);return -1;}
 int result=sceCtrlPeekBufferPositive(0,&pad,1);
 if(result<0){c_buttons=0;duck_pad_update(0,0,0,0);return -1;}
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
 c_buttons=c_axis(pad.ry,c_buttons,8,4)|c_axis(pad.rx,c_buttons,2,1);
 b|=c_buttons;
 duck_pad_update(1,b,axis(pad.lx,0),axis(pad.ly,1));return 1;
}
