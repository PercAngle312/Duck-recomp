#include "duck_vita_cpu_irq.h"
#include "duck_vita_pi.h"
#include "duck_vita_mi.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
typedef struct {int line;unsigned up,down;} Sink;
static void changed(void *ctx,int line) {
    Sink *s=ctx;
    assert(line!=s->line);
    s->line=line;
    if(line)s->up++;else s->down++;
}
static void pi_irq(void *ctx,int asserted) {
    duck_vita_mi_set_source((DuckVitaMi*)ctx,DUCK_MI_INTR_PI,asserted);
}
int main(void) {
    uint8_t data[32]={0},*memory=calloc(1,DUCK_VITA_RDRAM_BYTES);
    assert(memory);
    DuckVitaBus bus={0};bus.rdram=(DuckVitaRdram){memory,DUCK_VITA_RDRAM_BYTES};
    DuckVitaCpuIrq cpu;Sink sink={0};DuckVitaMi mi;DuckVitaPi pi;
    duck_vita_cpu_irq_init(&cpu,changed,&sink);
    assert(duck_vita_mi_init(&mi,&bus,duck_vita_cpu_irq_mi_line,&cpu)==0);
    assert(duck_vita_pi_init(&pi,&bus,(DuckVitaRom){data,sizeof(data)},pi_irq,&mi)==0);
    assert(duck_vita_bus_write32(&bus,0xa4600004u,DUCK_PI_CART_BASE)==0);
    assert(duck_vita_bus_write32(&bus,0xa4600008u,7)==0);
    assert(!duck_vita_cpu_irq_pending(&cpu));
    assert(duck_vita_bus_write32(&bus,0xa430000cu,1u<<9)==0);
    assert(cpu.cause & DUCK_CPU_CAUSE_IP2);
    assert(!duck_vita_cpu_irq_pending(&cpu));
    duck_vita_cpu_irq_set_status(&cpu,DUCK_CPU_STATUS_IE|DUCK_CPU_STATUS_IM2);
    assert(duck_vita_cpu_irq_pending(&cpu) && sink.up==1);
    duck_vita_cpu_irq_set_status(&cpu,DUCK_CPU_STATUS_IE|DUCK_CPU_STATUS_IM2|DUCK_CPU_STATUS_EXL);
    assert(!duck_vita_cpu_irq_pending(&cpu) && sink.down==1);
    duck_vita_cpu_irq_set_status(&cpu,DUCK_CPU_STATUS_IE|DUCK_CPU_STATUS_IM2);
    assert(duck_vita_cpu_irq_pending(&cpu) && sink.up==2);
    assert(duck_vita_bus_write32(&bus,0xa4600010u,2)==0);
    assert(!(cpu.cause & DUCK_CPU_CAUSE_IP2));
    assert(!duck_vita_cpu_irq_pending(&cpu) && sink.down==2);
    free(memory);
    puts("PASS: PI -> MI -> CPU IP2, IE/IM/EXL gating, edge callbacks, ack");
    return 0;
}
