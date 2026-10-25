#include "duck_vita_cpu_irq.h"
#include <string.h>
static void evaluate(DuckVitaCpuIrq *cpu) {
    int enabled = (cpu->status & DUCK_CPU_STATUS_IE) &&
                  !(cpu->status & (DUCK_CPU_STATUS_EXL|DUCK_CPU_STATUS_ERL));
    int next = !!(enabled && (cpu->status & cpu->cause & DUCK_CPU_STATUS_IM2));
    if(next != cpu->deliverable) {
        cpu->deliverable=next;
        if(cpu->on_change)cpu->on_change(cpu->opaque,next);
    }
}
void duck_vita_cpu_irq_init(DuckVitaCpuIrq *cpu,
    void (*on_change)(void*,int),void *opaque) {
    if(!cpu)return;
    memset(cpu,0,sizeof(*cpu));cpu->on_change=on_change;cpu->opaque=opaque;
}
void duck_vita_cpu_irq_set_status(DuckVitaCpuIrq *cpu,uint32_t status) {
    if(!cpu)return;
    cpu->status=status;evaluate(cpu);
}
void duck_vita_cpu_irq_mi_line(void *opaque,int asserted) {
    DuckVitaCpuIrq *cpu=opaque;
    if(!cpu)return;
    if(asserted)cpu->cause |= DUCK_CPU_CAUSE_IP2;
    else cpu->cause &= ~DUCK_CPU_CAUSE_IP2;
    evaluate(cpu);
}
int duck_vita_cpu_irq_pending(const DuckVitaCpuIrq *cpu) {
    return cpu ? cpu->deliverable : 0;
}
