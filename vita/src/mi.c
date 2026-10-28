#include "duck_vita_mi.h"
#include <string.h>
static void update(DuckVitaMi *mi) {
    if(mi->cpu_irq) mi->cpu_irq(mi->opaque, (mi->pending & mi->mask)!=0);
}
int duck_vita_mi_init(DuckVitaMi *mi,DuckVitaBus *bus,DuckMiCpuIrq cb,void *opaque) {
    if(!mi||!bus)return DUCK_GUEST_RAM_BAD_ARG;
    memset(mi,0,sizeof(*mi));mi->cpu_irq=cb;mi->opaque=opaque;
    DuckVitaMmioRegion r={DUCK_MI_BASE,DUCK_MI_END,duck_vita_mi_read,duck_vita_mi_write,mi};
    return duck_vita_bus_register(bus,r);
}
void duck_vita_mi_set_source(DuckVitaMi *mi,uint32_t source,int asserted) {
    if(!mi)return;
    source &= 0x3fu;
    if(asserted)mi->pending|=source;
    else mi->pending &= ~source;
    update(mi);
}
int duck_vita_mi_read(void *opaque,uint32_t a,uint32_t *value) {
    DuckVitaMi *mi=opaque;
    if(!mi||!value||a<DUCK_MI_BASE||a>=DUCK_MI_END||(a&3))return DUCK_GUEST_RAM_BAD_ARG;
    switch(a-DUCK_MI_BASE) {
        case 0x08:*value=mi->pending;return 0;
        case 0x0c:*value=mi->mask;return 0;
        default:*value=0;return 0;
    }
}
int duck_vita_mi_write(void *opaque,uint32_t a,uint32_t value) {
    DuckVitaMi *mi=opaque;
    if(!mi||a<DUCK_MI_BASE||a>=DUCK_MI_END||(a&3))return DUCK_GUEST_RAM_BAD_ARG;
    if(a-DUCK_MI_BASE!=0x0c)return DUCK_GUEST_RAM_UNMAPPED;
    for(unsigned i=0;i<6;i++) {
        if(value&(1u<<(i*2)))mi->mask &= ~(1u<<i);
        if(value&(1u<<(i*2+1)))mi->mask |= 1u<<i;
    }
    update(mi);
    return 0;
}
