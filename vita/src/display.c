/* Native Vita display presenter. No N64 RDP emulation is implied. */
#include "duck_vita_framebuffer.h"
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <stdlib.h>
#include <string.h>
struct DuckVitaDisplay {
    SceUID memblock;
    uint32_t *pixels;
};
int duck_vita_display_open(DuckVitaDisplay **out) {
    if (!out) return -1;
    *out = NULL;
    DuckVitaDisplay *d = calloc(1, sizeof(*d));
    if (!d) return -2;
    d->memblock = -1;
    const size_t bytes = DUCK_VITA_SCREEN_PITCH * DUCK_VITA_SCREEN_HEIGHT * 4;
    d->memblock = sceKernelAllocMemBlock("duck_vita_display",
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, bytes, NULL);
    if (d->memblock < 0) { free(d); return -3; }
    void *base = NULL;
    if (sceKernelGetMemBlockBase(d->memblock, &base) < 0 || !base) {
        sceKernelFreeMemBlock(d->memblock); free(d); return -4;
    }
    d->pixels = base;
    memset(d->pixels, 0, bytes);
    *out = d;
    return 0;
}
int duck_vita_display_present(DuckVitaDisplay *d, const uint8_t *rgba,
                              unsigned w, unsigned h, size_t stride) {
    if (!d || !d->pixels) return -1;
    int rc = duck_vita_scale_rgba_to_abgr(d->pixels,
        DUCK_VITA_SCREEN_PITCH * DUCK_VITA_SCREEN_HEIGHT,
        DUCK_VITA_SCREEN_WIDTH, DUCK_VITA_SCREEN_HEIGHT,
        DUCK_VITA_SCREEN_PITCH, rgba, w, h, stride);
    if (rc) return rc;
    SceDisplayFrameBuf frame;
    memset(&frame, 0, sizeof(frame));
    frame.size = sizeof(frame);
    frame.base = d->pixels;
    frame.pitch = DUCK_VITA_SCREEN_PITCH;
    frame.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    frame.width = DUCK_VITA_SCREEN_WIDTH;
    frame.height = DUCK_VITA_SCREEN_HEIGHT;
    return sceDisplaySetFrameBuf(&frame, SCE_DISPLAY_SETBUF_NEXTFRAME);
}
void duck_vita_display_close(DuckVitaDisplay *d) {
    if (!d) return;
    if (d->memblock >= 0) sceKernelFreeMemBlock(d->memblock);
    free(d);
}
