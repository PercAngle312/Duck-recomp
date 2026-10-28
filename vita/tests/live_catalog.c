#include "duck_vita_live_registry.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static DuckThreadLookup lookup;
static int init_result,booted;
static void resident(uint8_t *r,recomp_context *c){(void)r;(void)c;}
static void overlay(uint8_t *r,recomp_context *c){(void)r;(void)c;}
int duck_vita_live_init(DuckVitaLive *live,uint8_t *ram,const uint8_t *rom,size_t bytes,
 DuckThreadLookup find,DuckVitaTickClock clock,void *opaque,uint64_t period,DuckVitaGraphicsSubmit renderer){
 (void)live;(void)ram;(void)rom;(void)bytes;(void)clock;(void)opaque;(void)period;(void)renderer;
 lookup=find;return init_result;
}
int duck_vita_live_boot(DuckVitaLive *live,uint32_t entry,uint32_t stack,uint32_t arg,int priority){
 (void)live;(void)stack;(void)arg;(void)priority;
 assert(lookup(entry)==resident);booted++;return 0;
}
static uint64_t clock(void *opaque){(void)opaque;return 0;}
static int renderer(uint32_t task){(void)task;return -1;}
int main(void){
 uint8_t *ram=calloc(1,0x800000);assert(ram);uint8_t rom[1]={0};DuckVitaLive live={0};
 DuckCodeFunction r[]={{0x80000400,resident}},m[]={{0,overlay}};
 DuckCodeModule mods[]={{1,0x40,0,1,0x80100000,m,1}};
 DuckCodeCatalog catalog={r,1,mods,1,0x80000400,0x80010000,1};
 init_result=-8;assert(duck_vita_live_start_catalog(&live,&catalog,ram,rom,1,clock,0,781250,renderer,0x80000400,0x80020000,0,1)==-8);
 assert(!booted&&!duck_vita_overlay_find(0x80000400));
 init_result=0;assert(!duck_vita_live_start_catalog(&live,&catalog,ram,rom,1,clock,0,781250,renderer,0x80000400,0x80020000,0,1));
 assert(booted==1&&!lookup(0x80100000));assert(!duck_vita_overlay_load(1,0x80050000,0x8004fff8,0x40,0));
 assert(lookup(0x80050000)==overlay&&!lookup(0x80100000));
 assert(duck_vita_live_start_catalog(&live,&catalog,ram,rom,1,clock,0,781250,renderer,0x80000400,0x80020000,0,1)==-1);
 assert(lookup(0x80050000)==overlay); // repeated startup must not erase loaded maps
 free(ram);puts("PASS: startup lookup uses relocated catalog and preserves mappings on repeat rejection");return 0;
}
