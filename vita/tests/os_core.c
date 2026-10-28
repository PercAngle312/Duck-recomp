#include "recomp.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
void __osDisableInt_recomp(uint8_t*,recomp_context*);
void __osRestoreInt_recomp(uint8_t*,recomp_context*);
void osVirtualToPhysical_recomp(uint8_t*,recomp_context*);
void osCreateMesgQueue_recomp(uint8_t*,recomp_context*);
static uint32_t rd(uint8_t *r,unsigned p){return (uint32_t)r[p]|((uint32_t)r[p+1]<<8)|((uint32_t)r[p+2]<<16)|((uint32_t)r[p+3]<<24);}
int main(void) {
 uint8_t *ram=calloc(1,0x800000);assert(ram);recomp_context c={0};
 c.status_reg=0xdbeef401u;
 __osDisableInt_recomp(ram,&c);assert(c.r2==1&&c.status_reg==0xdbeef400u);
 __osDisableInt_recomp(ram,&c);assert(c.r2==0&&c.status_reg==0xdbeef400u);
 c.r4=0x80000001u;__osRestoreInt_recomp(ram,&c);assert(c.status_reg==0xdbeef401u);
 c.r4=0xffffffffa0123450ull;osVirtualToPhysical_recomp(ram,&c);assert(c.r2==0x123450u);
 c.r4=0x70000000;osVirtualToPhysical_recomp(ram,&c);assert(c.r2==0xffffffffu);
 c.r4=0xffffffff80001000ull;c.r5=0x80002000u;c.r6=2;
 osCreateMesgQueue_recomp(ram,&c);
 assert(rd(ram,0x1010)==2&&rd(ram,0x1014)==0x80002000u&&rd(ram,0x1008)==0);
 c.r4=0x180001000ull;c.r6=99;osCreateMesgQueue_recomp(ram,&c);assert(rd(ram,0x1010)==2);
 c.r4=0x807ffff0u;osCreateMesgQueue_recomp(ram,&c);
 c.r4=0x80001000u;c.r5=0x80001004u;osCreateMesgQueue_recomp(ram,&c);assert(rd(ram,0x1010)==2);
 free(ram);puts("PASS: guest Status preservation, KSEG translation and bounded nonblocking queue initialization");
}
