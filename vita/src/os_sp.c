/* Translate the real libultra OSTask pointer into a Fast3D task submission.
 * Guest RAM is word-swapped and the OSTask type word is little-endian in RAM.
 * HLE graphics execution is synchronous: a completed task cannot be yielded.
 * SP/DP completion is sent by the owning runtime only after actual rendering.
 */
#include "recomp.h"
#include "duck_vita_sp_os.h"
#include <stdint.h>
static DuckVitaSp *active_sp;
static uint8_t *active_ram;
static int last_result;
static uint32_t loaded_task;
static int task_loaded;
static int task_offset(uint64_t address,uint32_t *out) {
 uint32_t a=(uint32_t)address,p=a&0x1fffffffu;
 if(address!=(uint64_t)a && address!=(uint64_t)(int64_t)(int32_t)a)return -1;
 if((a&0xe0000000u)!=0x80000000u && (a&0xe0000000u)!=0xa0000000u)return -1;
 if((p&7u)||p>0x800000u-64u)return -1;
 *out=p;return 0;
}
static uint32_t word(const uint8_t *ram,uint32_t p) {
 return (uint32_t)ram[p]|((uint32_t)ram[p+1]<<8)|
        ((uint32_t)ram[p+2]<<16)|((uint32_t)ram[p+3]<<24);
}
void duck_vita_sp_os_bind(DuckVitaSp *sp,uint8_t *rdram) {
 active_sp=sp;active_ram=rdram;last_result=sp&&rdram?0:DUCK_SP_RENDERER_UNAVAILABLE;
 loaded_task=0;task_loaded=0;
}
int duck_vita_sp_os_last_result(void){return last_result;}
/* libultra OSTask load stages a validated task for the SP. The HLE backend
 * executes it on StartGo rather than uploading microcode to physical IMEM. */
void osSpTaskLoad_recomp(uint8_t *rdram,recomp_context *ctx) {
 uint32_t p;
 last_result=DUCK_SP_BAD_ARGUMENT;
 if(!ctx||!active_sp||rdram!=active_ram||task_offset(ctx->r4,&p))return;
 uint32_t type=word(rdram,p);
 if(type!=1u&&type!=2u)return;
 if(type==2u&&!active_sp->audio_submit){last_result=DUCK_SP_AUDIO_UNAVAILABLE;return;}
 if(active_sp->busy){last_result=DUCK_SP_BUSY;return;}
 loaded_task=(uint32_t)ctx->r4;
 task_loaded=1;
 last_result=0;
}
/* Synchronous HLE tasks are finished before this request can run.
 * Yielded=0 is meaningful here, not a fake successful RSP yield. */
void osSpTaskYield_recomp(uint8_t *rdram,recomp_context *ctx) {
 last_result=(!ctx||rdram!=active_ram||!active_sp)?
    DUCK_SP_BAD_ARGUMENT:(active_sp->busy?DUCK_SP_BUSY:0);
}
void osSpTaskYielded_recomp(uint8_t *rdram,recomp_context *ctx) {
 if(!ctx)return;
 last_result=(rdram!=active_ram||!active_sp)?DUCK_SP_BAD_ARGUMENT:0;
 /* A synchronous renderer never yields an in-flight task. A caller must
  * not mistake an invalid RDRAM binding for a successful guest syscall. */
 ctx->r2=last_result==0?0:(gpr)-1;
}
void osSpTaskStartGo_recomp(uint8_t *rdram,recomp_context *ctx) {
 uint32_t p;
 last_result=DUCK_SP_BAD_ARGUMENT;
 if(!ctx||!rdram||rdram!=active_ram||!active_sp||task_offset(ctx->r4,&p))return;
 /* OSTask task.t.type == M_GFXTASK (1). No audio task may reach Fast3D. */
 uint32_t type=word(rdram,p);if(type!=1u&&type!=2u)return;
 /* Reject a different StartGo address while an explicit task is staged. */
 if(task_loaded && loaded_task!=(uint32_t)ctx->r4)return;
 last_result=type==1u?duck_vita_sp_start_graphics(active_sp,(uint32_t)ctx->r4):
                       duck_vita_sp_start_audio(active_sp,(uint32_t)ctx->r4);
 if(last_result!=DUCK_SP_BUSY)task_loaded=0;
}
