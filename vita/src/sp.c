#include "duck_vita_sp.h"
#include <string.h>
void duck_vita_sp_init(DuckVitaSp *sp, DuckVitaGraphicsSubmit submit,
                       DuckVitaSpCompletion complete, void *opaque) {
 if(!sp)return;
 memset(sp,0,sizeof(*sp));
 sp->submit=submit;
 sp->complete=complete;
 sp->opaque=opaque;
}
int duck_vita_sp_start_graphics(DuckVitaSp *sp, uint32_t address) {
 if(!sp || !address || (address&3u))return DUCK_SP_BAD_ARGUMENT;
 if(sp->busy)return DUCK_SP_BUSY;
 if(!sp->submit)return DUCK_SP_RENDERER_UNAVAILABLE;
 sp->busy=1;
 sp->submitted++;
 int result=sp->submit(address);
 if(result==0)sp->completed++;
 else sp->failed++;
 sp->busy=0;
 if(sp->complete)sp->complete(sp->opaque,result==0);
 return result;
}
