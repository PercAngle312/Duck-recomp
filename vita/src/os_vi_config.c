/* N64 VI configuration state bridge. This records validated guest configuration;
 * Time service schedules retrace events; graphics and mode decoding are separate. */
#include "duck_vita_time.h"
#include <stdint.h>

static DuckViConfig config;
static uint32_t generation;
uint32_t duck_vi_config_generation(void){return generation;}
static int guest_ram_pointer(uint64_t address, uint32_t alignment) {
 uint32_t a=(uint32_t)address, segment=a&0xe0000000u;
 return (address==(uint64_t)a || address==(uint64_t)(int64_t)(int32_t)a) &&
  (segment==0x80000000u||segment==0xa0000000u) &&
  ((a&0x1fffffffu)<0x800000u) && !(a&(alignment-1u));
}
void duck_vi_config_reset(void) {config=(DuckViConfig){0};generation++;}
int duck_vi_config_snapshot(DuckViConfig *out) {if(!out)return -1;*out=config;return 0;}
void osViSetMode_recomp(uint8_t *rdram,recomp_context *ctx) {
 (void)rdram;
 if(ctx && guest_ram_pointer(ctx->r4,4u))config.mode_address=(uint32_t)ctx->r4;
}
void osViSetSpecialFeatures_recomp(uint8_t *rdram,recomp_context *ctx) {
 (void)rdram;
 if(ctx)config.special_features=(uint32_t)ctx->r4;
}
void osViSetEvent_recomp(uint8_t *rdram,recomp_context *ctx) {
 (void)rdram;
 if(!ctx)return;
 /* A zero count disables retrace delivery; no event is silently delivered. */
 if(ctx->r6>0xffffffffull)return;
 if(ctx->r6 && !guest_ram_pointer(ctx->r4,4u))return;
 config.event_queue=(uint32_t)ctx->r4;
 config.event_message=(uint32_t)ctx->r5;
 config.retrace_count=(uint32_t)ctx->r6;generation++;
}
