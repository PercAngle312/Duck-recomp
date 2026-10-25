#pragma once
/* Portable framebuffer contract for the native Vita presentation backend.
 * This is a VI presentation surface, NOT an N64 RDP/RSP renderer.
 * Input is tightly packed RGBA8888; destination is Vita ABGR8888 in memory.
 */
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { DUCK_VITA_SCREEN_WIDTH = 960, DUCK_VITA_SCREEN_HEIGHT = 544,
       DUCK_VITA_SCREEN_PITCH = 960 };
typedef struct DuckVitaDisplay DuckVitaDisplay;
int duck_vita_display_open(DuckVitaDisplay **out);
int duck_vita_display_present(DuckVitaDisplay *display,
                              const uint8_t *rgba, unsigned width, unsigned height,
                              size_t stride);
void duck_vita_display_close(DuckVitaDisplay *display);
/* Pure CPU conversion/scaling, also usable in host regression tests.
 * Returns 0 on valid frame, negative on invalid or undersized input. */
int duck_vita_scale_rgba_to_abgr(uint32_t *dst, size_t dst_pixels,
                                 unsigned dw, unsigned dh, unsigned pitch,
                                 const uint8_t *src, unsigned sw, unsigned sh,
                                 size_t stride);
#ifdef __cplusplus
}
#endif
