#include "duck_vita_framebuffer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
    const uint8_t input[] = {
        0x11,0x22,0x33,0x44, 0xaa,0xbb,0xcc,0xdd,
        0x10,0x20,0x30,0x40, 0xfe,0xdc,0xba,0x98
    };
    uint32_t dst[16] = {0};
    assert(duck_vita_scale_rgba_to_abgr(dst,16,4,4,4,input,2,2,8)==0);
    assert(dst[0]==0x44332211 && dst[1]==0x44332211);
    assert(dst[2]==0xddccbbaa && dst[3]==0xddccbbaa);
    assert(dst[8]==0x40302010 && dst[15]==0x98badcfe);
    assert(duck_vita_scale_rgba_to_abgr(dst,15,4,4,4,input,2,2,8)!=0);
    assert(duck_vita_scale_rgba_to_abgr(dst,16,4,4,4,input,2,2,7)!=0);
    assert(duck_vita_scale_rgba_to_abgr(dst,16,4,4,4,NULL,2,2,8)!=0);
    assert(duck_vita_scale_rgba_to_abgr(dst,16,4,4,3,input,2,2,8)!=0);
    puts("PASS: nearest-neighbor scale, RGBA->ABGR, bounds and invalid inputs");
    return 0;
}
