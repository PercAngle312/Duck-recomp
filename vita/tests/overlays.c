#include "duck_vita_overlays.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static void resident(uint8_t *r,recomp_context *c){(void)r;c->r2=1;}
static void alpha(uint8_t *r,recomp_context *c){(void)r;c->r2=2;}
static void beta(uint8_t *r,recomp_context *c){(void)r;c->r2=3;}
int main(void){
 uint8_t *ram=calloc(1,0x800000);assert(ram);
 DuckCodeFunction res[]={{0x80000400,resident}},a[]={{0,alpha},{0x20,beta}},b[]={{0,beta}};
 DuckCodeModule mods[]={{1,0x40,0x20,1,0x80100000,a,2},{2,0x20,0x20,2,0x80100000,b,1}};
 DuckCodeCatalog c={res,1,mods,2,0x80000400,0x80038000,1};
 c.allocation_observer=0;assert(duck_vita_overlays_init(ram,&c)==-1);c.allocation_observer=1;
 assert(!duck_vita_overlays_init(ram,&c));assert(section_addresses[0]==(int32_t)0x80000400u);
 assert(duck_vita_overlay_find(0xa0000400)==resident&&!duck_vita_overlay_find(0x80050000));
 assert(!duck_vita_overlay_load(1,0x80050000,0x8004fff8,0x40,0x20));
 assert(section_addresses[1]==(int32_t)0x80050000u);
 assert(duck_vita_overlay_find(0xa0050020)==beta&&!duck_vita_overlay_find(0x80050004));
 assert(duck_vita_overlay_load(1,0x80060000,0x8005fff8,0x40,0x20)==-1);
 assert(duck_vita_overlay_load(2,0x80050040,0x80050038,0x20,0x20)==-1); // overlapping BSS
 assert(duck_vita_overlay_free(0x80070000)==0); // real data-only free
 assert(duck_vita_overlay_free(0xa004fff8)==1);
 assert(duck_vita_overlay_find(0x80050000)==alpha); // destructor after free
 assert(!duck_vita_overlay_reuse(0x80050060,8)); // adjacent half-open range
 assert(duck_vita_overlay_find(0x80050000)==alpha);
 assert(!duck_vita_overlay_reuse(0x80050020,4));
 assert(!duck_vita_overlay_find(0x80050000)&&section_addresses[1]==(int32_t)0x80100000u);
 assert(!duck_vita_overlay_load(2,0x80050020,0x80050018,0x20,0x20));
 assert(duck_vita_overlay_find(0x80050020)==beta);
 assert(duck_vita_overlay_reuse(0x80050020,4)==-1); // active code cannot be reused
 assert(duck_vita_overlay_load(1,0x807ffff0,0x807fffe0,0x40,0x20)==-1);
 assert(duck_vita_overlay_load(1,0x80001000,0x80000ff8,0x40,0x20)==-1);
 assert(duck_vita_overlay_load(1,0x80060000,0x8005fff8,0x44,0x20)==-1);
 assert(duck_vita_overlay_free(0x80050018)==1);
 assert(!duck_vita_overlay_load(2,0x80060000,0x8005fff8,0x20,0x20));
 assert(!duck_vita_overlay_find(0x80050020)&&duck_vita_overlay_find(0x80060000)==beta);
 free(ram);puts("PASS: relocated lookup, section bases, checked module sizes, active overlap, retained cleanup and allocation-reuse invalidation");return 0;
}
