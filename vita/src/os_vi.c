/* Minimal N64 VI state transitions for the ARM translated-code bringup.
 * Explicit host retrace required; no rendering, VI event or timing is implied. */
#include "recomp.h"
#include <stdint.h>
typedef struct { uint32_t current, next; uint8_t black, pending; } DuckVi;
static DuckVi vi;
static int valid_buffer(uint64_t value) {
    uint32_t addr=(uint32_t)value,seg=addr&0xe0000000u,phys=addr&0x1fffffffu;
    return (seg==0x80000000u||seg==0xa0000000u) && !(phys&7u) && phys<0x800000u;
}
void duck_vi_reset(void) {vi=(DuckVi){0,0,1,0};}
/* Called on the emulated VI retrace edge; should later drive OS_EVENT_VI. */
void duck_vi_retrace(void) {if(vi.pending){vi.current=vi.next;vi.pending=0;}}
unsigned duck_vi_is_black(void) {return vi.black;}
void osViBlack_recomp(uint8_t *ram,recomp_context *ctx) {
 (void)ram; if(ctx)vi.black=ctx->r4?1:0;
}
void osViSwapBuffer_recomp(uint8_t *ram,recomp_context *ctx) {
 (void)ram; if(!ctx)return;
 if(!valid_buffer(ctx->r4))return; /* Do not enqueue invalid guest pointers. */
 vi.next=(uint32_t)ctx->r4;vi.pending=1;
}
void osViGetCurrentFramebuffer_recomp(uint8_t *ram,recomp_context *ctx) {
 (void)ram;if(ctx)ctx->r2=(uint64_t)(int64_t)(int32_t)vi.current;
}
void osViGetNextFramebuffer_recomp(uint8_t *ram,recomp_context *ctx) {
 (void)ram;if(ctx)ctx->r2=(uint64_t)(int64_t)(int32_t)vi.next;
}
