/* osSetIntMask's CPU Status and per-thread RCP mask ABI. This models masks,
 * not the full N64 exception/MI device-register dispatch path. Cooperative
 * kernel calls can still switch threads, as libultra dispatch calls do. */
#include "duck_vita_threads.h"
void osSetIntMask_recomp(uint8_t *rdram,recomp_context *ctx){
 (void)rdram;if(!ctx)return;
 uint32_t previous=(ctx->status_reg&0xff01u)|(duck_vita_thread_rcp_mask()<<16);
 uint32_t requested=(uint32_t)ctx->r4;
 ctx->status_reg=(ctx->status_reg&~0xff00u)|(requested&0xff00u);
 duck_vita_thread_set_rcp_mask((requested>>16)&0x3f);
 ctx->r2=(gpr)(int32_t)previous;
}
