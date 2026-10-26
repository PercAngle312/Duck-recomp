#include "duck_vita_live_registry.h"
static const DuckVitaDispatchTable *registry;
static int catalog_bound;
int duck_vita_live_registry_bind(const DuckVitaDispatchTable *table){
 if(registry||catalog_bound||duck_vita_dispatch_validate(table)!=0)return -1;
 registry=table;return 0;
}
void duck_vita_live_registry_unbind(void){registry=0;}
recomp_func_t *duck_vita_live_registry_lookup(uint32_t vram){
 if(!registry || (vram&3u))return 0;
 size_t lo=0,hi=registry->count;
 while(lo<hi){size_t mid=lo+(hi-lo)/2;
  if(registry->entries[mid].vram<vram)lo=mid+1;else hi=mid;
 }
 return lo<registry->count&&registry->entries[lo].vram==vram?
  registry->entries[lo].function:0;
}
int duck_vita_live_start_registered(DuckVitaLive *live,
 const DuckVitaDispatchTable *table,
 uint8_t *rdram,const uint8_t *rom,size_t rom_size,
 DuckVitaTickClock clock,void *clock_opaque,uint64_t vi_period,
 DuckVitaGraphicsSubmit renderer,uint32_t entry,uint32_t stack,
 uint32_t arg,int priority){
 /* A prior registry must never be silently reused for a new game. */
 if(duck_vita_live_registry_bind(table)!=0)return -1;
 if(!duck_vita_live_registry_lookup(entry)){
  duck_vita_live_registry_unbind();
  return -2;
 }
 int rc=duck_vita_live_init(live,rdram,rom,rom_size,
   duck_vita_live_registry_lookup,clock,clock_opaque,vi_period,renderer);
 if(rc!=0){duck_vita_live_registry_unbind();return rc;}
 return duck_vita_live_boot(live,entry,stack,arg,priority);
}

/* The static registry cannot represent several unloaded overlays sharing the
 * same default VRAM. Use the checked relocated catalog for production lookup. */
int duck_vita_live_start_catalog(DuckVitaLive *live,const DuckCodeCatalog *catalog,
 uint8_t *rdram,const uint8_t *rom,size_t rom_size,
 DuckVitaTickClock clock,void *clock_opaque,uint64_t vi_period,
 DuckVitaGraphicsSubmit renderer,uint32_t entry,uint32_t stack,
 uint32_t arg,int priority){
 if(registry||catalog_bound||!catalog||(entry&3u))return -1;
 /* live_init rejects an already-active runtime before any mappings change.
  * Thread initialization stores lookup but does not execute it until boot. */
 int rc=duck_vita_live_init(live,rdram,rom,rom_size,duck_vita_overlay_find,
                          clock,clock_opaque,vi_period,renderer);
 if(rc!=0)return rc;
 if(duck_vita_overlays_init(rdram,catalog)!=0)return -2;
 catalog_bound=1;
 if(!duck_vita_overlay_find(entry))return -3;
 return duck_vita_live_boot(live,entry,stack,arg,priority);
}
