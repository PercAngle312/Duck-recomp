#include "duck_vita_native_memory.h"
#include <string.h>
static int span(gpr address,uint32_t bytes,uint32_t *physical){
 uint32_t a=(uint32_t)address,p=a&0x1fffffffu;
 if(address!=(gpr)a&&address!=(gpr)(int32_t)a)return 0;
 if((a&0xe0000000u)!=0x80000000u&&(a&0xe0000000u)!=0xa0000000u)return 0;
 if(p>0x800000u||bytes>0x800000u-p)return 0;
 *physical=p;return 1;
}
void duck_vita_zero_bulk(uint8_t *ram,recomp_context *c){
 uint32_t p,n=(uint32_t)(c->r7-c->r4);
 if(!ram||n<64||(n&31u)||!span(c->r4,n,&p)||(p&3u))return;
 uint32_t skip=n-32;if(skip>256)skip=256;memset(ram+p,0,skip);c->r4=ADD32(c->r4,skip);
}
void duck_vita_copy_bulk(uint8_t *ram,recomp_context *c){
 uint32_t src,dst,n=(uint32_t)c->r6;
 if(!ram||c->r6!=(gpr)n||n<64||!span(c->r4,n,&src)||!span(c->r5,n,&dst)||((src|dst)&3u))return;
 // The translated routine owns overlap direction. Accelerate only disjoint
 // spans, then execute its final complete 32-byte block and tail unchanged.
 if(src<dst+n&&dst<src+n)return;
 uint32_t skip=(n&~31u)-32;if(skip>256)skip=256;memcpy(ram+dst,ram+src,skip);
 c->r4=ADD32(c->r4,skip);c->r5=ADD32(c->r5,skip);c->r6=SUB32(c->r6,skip);
}
void duck_vita_fill_bulk(uint8_t *ram,recomp_context *c){
 uint32_t p,n=(uint32_t)(c->r6-c->r2);
 if(!ram||c->r6>0x800000u||c->r2>=c->r6||n<16||!span(c->r3,n,&p))return;
 uint32_t skip=n-1,i=0;if(skip>256)skip=256;uint8_t value=(uint8_t)c->r5;
 while(i<skip&&((p+i)&3u)){ram[(p+i)^3u]=value;i++;}
 uint32_t words=(skip-i)&~3u;memset(ram+p+i,value,words);i+=words;
 while(i<skip){ram[(p+i)^3u]=value;i++;}
 c->r2=ADD32(c->r2,skip);c->r3=ADD32(c->r3,skip);
}
