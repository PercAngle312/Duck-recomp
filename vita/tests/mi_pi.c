#include "duck_vita_pi.h"
#include "duck_vita_mi.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { unsigned up,down;int line; } Cpu;
static void cpu(void *ctx,int line) { Cpu *c=ctx; if(line!=c->line) {if(line)c->up++;else c->down++;}c->line=line; }
static void pi_irq(void *ctx,int asserted){ duck_vita_mi_set_source((DuckVitaMi*)ctx,DUCK_MI_INTR_PI,asserted); }
int main(void) {
    uint8_t rom[32];for(unsigned i=0;i<32;i++)rom[i]=(uint8_t)(i+1);
    uint8_t *memory=calloc(1,DUCK_VITA_RDRAM_BYTES);assert(memory);
    DuckVitaBus bus={0};bus.rdram=(DuckVitaRdram){memory,DUCK_VITA_RDRAM_BYTES};
    Cpu state={0};DuckVitaMi mi;DuckVitaPi pi;
    assert(duck_vita_mi_init(&mi,&bus,cpu,&state)==0);
    assert(duck_vita_pi_init(&pi,&bus,(DuckVitaRom){rom,sizeof(rom)},pi_irq,&mi)==0);
    assert(duck_vita_bus_write32(&bus,0xa4600000u,0x100)==0);
    assert(duck_vita_bus_write32(&bus,0xa4600004u,DUCK_PI_CART_BASE)==0);
    assert(duck_vita_bus_write32(&bus,0xa460000cu,7)==0);
    assert(memcmp(memory+0x100,rom,8)==0);
    assert(mi.pending==DUCK_MI_INTR_PI && state.up==0);
    assert(duck_vita_bus_write32(&bus,0xa430000cu,1u<<9)==0);
    assert(state.up==1 && state.line==1);
    uint32_t bits=0;assert(duck_vita_bus_read32(&bus,0x8430000cu,&bits)==0 && bits==DUCK_MI_INTR_PI);
    assert(duck_vita_bus_write32(&bus,0xa4600010u,2)==0);
    assert(mi.pending==0 && state.down==1 && !state.line);
    assert(duck_vita_bus_write32(&bus,0xa430000cu,1u<<8)==0);
    assert(mi.mask==0);
    free(memory);
    puts("PASS: PI DMA -> MI pending -> mask enable -> CPU IRQ -> PI ack -> IRQ clear");
    return 0;
}
