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
 int success=result==0||result==DUCK_SP_GRAPHICS_NO_DP;
 if(success)sp->completed++;
 else sp->failed++;
 sp->busy=0;
 if(sp->complete)sp->complete(sp->opaque,success?(result==DUCK_SP_GRAPHICS_NO_DP?2:1):0);
 return success?0:result;
}

int duck_vita_sp_set_audio(DuckVitaSp *sp,DuckVitaGraphicsSubmit submit,DuckVitaSpCompletion complete){
 if(!sp||!submit||!complete)return DUCK_SP_BAD_ARGUMENT;
 if(sp->busy)return DUCK_SP_BUSY;
 sp->audio_submit=submit;sp->audio_complete=complete;return 0;
}
int duck_vita_sp_start_audio(DuckVitaSp *sp,uint32_t address){
 if(!sp||!address||(address&3u))return DUCK_SP_BAD_ARGUMENT;
 if(sp->busy)return DUCK_SP_BUSY;
 if(!sp->audio_submit)return DUCK_SP_AUDIO_UNAVAILABLE;
 sp->busy=1;sp->submitted++;int result=sp->audio_submit(address);
 if(result==0)sp->completed++;else sp->failed++;
 sp->busy=0;sp->audio_complete(sp->opaque,result==0);return result;
}
