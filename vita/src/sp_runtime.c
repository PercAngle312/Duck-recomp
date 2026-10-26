#include "duck_vita_sp_runtime.h"
#include "duck_vita_sp_os.h"
int duck_vita_sp_runtime_attach(DuckVitaSp *sp,DuckVitaSpEventState *events,
                                uint8_t *rdram,DuckVitaGraphicsSubmit renderer){
 if(!sp||!events||!rdram||!renderer)return -1;
 duck_vita_sp_events_reset(events);
 duck_vita_sp_init(sp,renderer,duck_vita_sp_events_complete,events);
 duck_vita_sp_os_bind(sp,rdram);
 return 0;
}
void duck_vita_sp_runtime_detach(DuckVitaSp *sp){
 duck_vita_sp_os_bind(0,0);
 if(sp)duck_vita_sp_init(sp,0,0,0);
}
