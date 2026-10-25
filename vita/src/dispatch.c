#include "duck_vita_dispatch.h"
int duck_vita_dispatch_validate(const DuckVitaDispatchTable *table) {
    if(!table||!table->entries||!table->count)return DUCK_DISPATCH_BAD_TABLE;
    for(size_t i=0;i<table->count;i++) {
        if(!table->entries[i].function || (table->entries[i].vram&3))
            return DUCK_DISPATCH_BAD_TABLE;
        if(i && table->entries[i-1].vram>=table->entries[i].vram)
            return DUCK_DISPATCH_BAD_TABLE;
    }
    return 0;
}
int duck_vita_dispatch_vram(DuckVitaRecompStore *store,DuckVitaScheduler *s,
    uint8_t *rdram,const DuckVitaDispatchTable *table,uint32_t vram) {
    if(!store||!s||!rdram)return -1;
    if(duck_vita_dispatch_validate(table))return DUCK_DISPATCH_BAD_TABLE;
    size_t left=0,right=table->count;
    while(left<right) {
        size_t mid=left+(right-left)/2;
        if(table->entries[mid].vram<vram)left=mid+1;
        else right=mid;
    }
    if(left==table->count||table->entries[left].vram!=vram)
        return DUCK_DISPATCH_UNKNOWN;
    return duck_vita_recomp_call(store,s,rdram,table->entries[left].function);
}
