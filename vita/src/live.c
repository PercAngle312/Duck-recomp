/* Actual guest runtime services and cooperative thread execution.
 * Unlike ABI placeholder bridges, failure to initialize a real subsystem
 * aborts setup; queued PI work and VI/timers advance at scheduler boundaries.
 */
#include "duck_vita_live.h"
#include "duck_vita_os_io.h"
#include "duck_vita_input.h"
#include <string.h>
static DuckVitaLive *active_live;
static int service(void){
 DuckVitaLive *live=active_live;
 if(!live||!live->initialized)return -1;
 int pi=duck_vita_os_pi_service();
 if(pi<0)return -1;
 live->pi_transfers+=(unsigned)pi;
 if(duck_vita_pad_poll()<0)return -1;
 if(duck_pad_service()<0)return -1;
 if(duck_vita_time_poll()<0)return -1;
 live->scheduler_polls++;
 return 0;
}
int duck_vita_live_init(DuckVitaLive *live,uint8_t *rdram,
 const uint8_t *rom,size_t rom_size,DuckThreadLookup lookup,
 DuckVitaTickClock clock,void *clock_opaque,uint64_t vi_period,
 DuckVitaGraphicsSubmit renderer){
 if(!live||!rdram||!rom||!rom_size||!lookup||!clock||!vi_period||!renderer||
    active_live)return -1;
 memset(live,0,sizeof(*live));
 duck_pad_reset();
 if(duck_vita_os_io_reset(rdram,rom,rom_size)!=0)return -2;
 if(duck_vita_time_bind(rdram,clock,clock_opaque,vi_period)!=0)return -3;
 if(duck_vita_sp_runtime_attach(&live->sp,&live->graphics_events,rdram,renderer)!=0)return -4;
 live->rdram=rdram;live->rom=rom;live->rom_size=rom_size;
 active_live=live;
 if(duck_vita_threads_init(rdram,lookup,service)!=0){
  duck_vita_sp_runtime_detach(&live->sp);
  active_live=0;
  return -5;
 }
 live->initialized=1;
 return 0;
}
int duck_vita_live_boot(DuckVitaLive *live,uint32_t entry,uint32_t stack,
 uint32_t arg,int priority){
 if(!live||live!=active_live||!live->initialized)return -1;
 return duck_vita_threads_boot(entry,stack,arg,priority);
}
int duck_vita_live_run(DuckVitaLive *live,unsigned dispatch_budget){
 if(!live||live!=active_live||!live->initialized||!dispatch_budget)return DUCK_THREADS_ERROR;
 return duck_vita_threads_run(dispatch_budget);
}
