/* Translate the real libultra OSTask pointer into a Fast3D task submission.
 * Guest RAM is word-swapped and the OSTask type word is little-endian in RAM.
 * This does NOT implement SP task yield, asynchronous RSP execution or events.
 */
#include "recomp.h"
#include "duck_vita_sp_os.h"
#include <stdint.h>
static DuckVitaSp *active_sp;
static uint8_t *active_ram;
static int last_result;
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
 active_sp=sp;active_ram=rdram;last_result=DUCK_SP_RENDERER_UNAVAILABLE;
}
int duck_vita_sp_os_last_result(void){return last_result;}
void osSpTaskStartGo_recomp(uint8_t *rdram,recomp_context *ctx) {
 uint32_t p;
 last_result=DUCK_SP_BAD_ARGUMENT;
 if(!ctx||!rdram||rdram!=active_ram||!active_sp||task_offset(ctx->r4,&p))return;
 /* OSTask task.t.type == M_GFXTASK (1). No audio task may reach Fast3D. */
 if(word(rdram,p)!=1u)return;
 last_result=duck_vita_sp_start_graphics(active_sp,(uint32_t)ctx->r4);
}
