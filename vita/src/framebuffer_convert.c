#include "duck_vita_framebuffer.h"
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
int duck_vita_scale_rgba_to_abgr(uint32_t *dst, size_t dst_pixels,
                                 unsigned dw, unsigned dh, unsigned pitch,
                                 const uint8_t *src, unsigned sw, unsigned sh,
                                 size_t stride) {
    if (!dst || !src || !dw || !dh || !sw || !sh || pitch < dw ||
        sw > SIZE_MAX / 4 || stride < (size_t)sw * 4 ||
        dh > SIZE_MAX / pitch || dst_pixels < (size_t)dh * pitch ||
        sh > SIZE_MAX / stride || dw > UINT32_MAX / 4)
        return -1;
    /* At most 960x544 in the native backend; use 64-bit products so
       requested source sizes cannot overflow interpolation coordinates. */
    for (unsigned y = 0; y < dh; ++y) {
        size_t sy = (size_t)(((uint64_t)y * sh) / dh);
        const uint8_t *row = src + sy * stride;
        for (unsigned x = 0; x < dw; ++x) {
            size_t sx = (size_t)(((uint64_t)x * sw) / dw);
            const uint8_t *p = row + sx * 4;
            /* ABGR8888 numeric layout: A in high byte, R in low byte. */
            dst[(size_t)y * pitch + x] = ((uint32_t)p[3] << 24) |
                ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | p[0];
        }
    }
    return 0;
}
