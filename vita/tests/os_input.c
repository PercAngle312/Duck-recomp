#include "duck_vita_input.h"
#include "duck_vita_os_io.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
extern void osCreateMesgQueue_recomp(uint8_t*,recomp_context*);
static uint32_t word(uint8_t *r,unsigned p){uint32_t v;memcpy(&v,r+p,4);return v;}
int main(void){
 uint8_t *r=malloc(0x800000),rom[4]={0};assert(r);memset(r,0xa5,0x800000);
 assert(!duck_vita_os_io_reset(r,rom,sizeof rom));duck_pad_reset();assert(duck_pad_needs_poll());recomp_context c={0};
 c.r4=0x80001000;c.r5=0x80001100;c.r6=2;osCreateMesgQueue_recomp(r,&c);
 c.r4=5;c.r5=0x80001000;c.r6=0x3333;osSetEventMesg_recomp(r,&c);
 duck_pad_update(1,0x8123,-80,80);assert(!duck_pad_needs_poll());
 c.r5=0x80002001;c.r6=0x80002012;osContInit_recomp(r,&c);assert(!c.r2);
 assert(r[0x2001^3]==1&&r[0x2012^3]==0&&r[0x2013^3]==5); // unaligned byte/halfword lanes
 c.r4=0x80001000;osContStartReadData_recomp(r,&c);assert(!c.r2&&word(r,0x1008)==0&&duck_pad_needs_poll());
 osContStartQuery_recomp(r,&c);assert(c.r2==(gpr)-1); // SI access serialization
 assert(duck_pad_service()==1&&word(r,0x1008)==1&&word(r,0x1100)==0x3333&&!duck_pad_needs_poll());
 c.r4=0xa0003002;osContGetReadData_recomp(r,&c);assert(!c.r2);
 assert(r[0x3002^3]==0x81&&r[0x3003^3]==0x23&&r[0x3004^3]==(uint8_t)-80&&r[0x3005^3]==80);
 assert(r[0x3006^3]==0&&r[0x3008^3]==0&&r[0x300c^3]==8); // next port starts at +6
 assert(r[0x3001^3]==0xa5&&r[0x301a^3]==0xa5); // no write outside 24 bytes
 duck_pad_update(0,0,0,0);osContGetReadData_recomp(r,&c);assert(r[0x3002^3]==0x81); // stable latched snapshot
 c.r4=0x80001000;osContStartQuery_recomp(r,&c);assert(!c.r2);assert(duck_pad_service()==1);
 c.r4=0x80004000;osContGetQuery_recomp(r,&c);assert(!c.r2&&r[0x4003^3]==8);
 c.r4=0x807ffff0;osContGetReadData_recomp(r,&c);assert(c.r2==(gpr)-1);
 free(r);puts("PASS: SI completion, serialized queries, 4-byte status/6-byte pad ABI, unaligned word-swapped lanes and latched input");return 0;
}
