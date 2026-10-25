#include "duck_vita_event_bridge.h"
#include "duck_vita_pi.h"
#include "duck_vita_mi.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static void pi_irq(void *ctx,int level) {
 duck_vita_mi_set_source((DuckVitaMi*)ctx,DUCK_MI_INTR_PI,level);
}
int main(void) {
 uint8_t rom[32]={0},*memory=calloc(1,DUCK_VITA_RDRAM_BYTES);
 assert(memory);
 DuckVitaBus bus={0};bus.rdram=(DuckVitaRdram){memory,DUCK_VITA_RDRAM_BYTES};
 uint32_t slots[1]={0},message=0;DuckVitaQueue q;DuckVitaScheduler scheduler;
 DuckVitaEventBridge bridge;DuckVitaCpuIrq cpu;DuckVitaMi mi;DuckVitaPi pi;
 assert(duck_vita_queue_init(&q,slots,1)==0);
 duck_vita_scheduler_init(&scheduler);
 assert(duck_vita_scheduler_add(&scheduler,1,1)==0);
 assert(duck_vita_scheduler_next(&scheduler)==1);
 assert(duck_vita_scheduler_wait_recv(&scheduler,1,&q)==0);
 duck_vita_event_bridge_init(&bridge,&scheduler,&q,0x5049u);
 duck_vita_cpu_irq_init(&cpu,duck_vita_event_bridge_irq,&bridge);
 assert(duck_vita_mi_init(&mi,&bus,duck_vita_cpu_irq_mi_line,&cpu)==0);
 assert(duck_vita_pi_init(&pi,&bus,(DuckVitaRom){rom,sizeof(rom)},pi_irq,&mi)==0);
 duck_vita_cpu_irq_set_status(&cpu,DUCK_CPU_STATUS_IE|DUCK_CPU_STATUS_IM2);
 assert(duck_vita_bus_write32(&bus,0xa430000cu,1u<<9)==0);
 assert(duck_vita_bus_write32(&bus,0xa4600004u,DUCK_PI_CART_BASE)==0);
 assert(duck_vita_bus_write32(&bus,0xa4600008u,3)==0);
 assert(bridge.delivered==1 && bridge.dropped==0);
 assert(scheduler.threads[0].state==DUCK_THREAD_READY);
 assert(duck_vita_scheduler_next(&scheduler)==1);
 assert(duck_vita_queue_recv(&q,&message)==0 && message==0x5049u);
 assert(duck_vita_bus_write32(&bus,0xa4600010u,2)==0);
 assert(!duck_vita_cpu_irq_pending(&cpu));
 free(memory);
 puts("PASS: PI DMA -> MI -> CPU IRQ -> OS queue -> scheduler receiver wake");
 return 0;
}
