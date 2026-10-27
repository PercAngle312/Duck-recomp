# Vita 00.13: pre-swapped texture rows and framebuffer transfers

The user's physical 00.12 log and video show corrupted character detail and
uneven animation. Later graphics batches average up to about 20 ms drawing
and 19 ms readback. Audio work increases with the scene; starvation padding
is reported. These are the reference symptoms, not a validation of this update.

The bridge previously guessed a DXT=0 LoadBlock's row pitch by looking ahead
at most seven following commands. That cannot reliably decode packed mip
chains, delayed tile setup, or a later reuse at another pitch. Staging now
preserves the original byte sequence. The loaded texture records DXT=0, and
format decoders undo the pre-swapped odd-row halves using the actual selected
view's pitch. The conversion runs on texture import/cache misses, keeps the
source immutable, uses 8-byte groups for 4/8/16-bit formats and 16 for RGBA32,
and participates in cache identity. LoadTile/ordinary nonzero-DXT blocks stay
linear. Interpreter-generated S2DEX background snapshots are explicitly kept
linear too; only real guest LoadBlock commands opt into this layout. A bounded log records the first sixteen converted texture views.

Reference: [N64 texture loading manual](https://ultra64.ca/files/documentation/online-manuals/functions_reference_manual_2.0i/gdp/gDPLoadTexture.html)
and the pinned libultra `gDPLoadTextureBlockS` macros: S denotes pre-swapped
odd lines, with DXT=0 also used to load multiple differently sized mip tiles.

The interpreter's separate staged-source registry was still a linear scan
on every texture load. It now uses ordered lookup for exact registrations
and containing-span resolution. The bridge's existing indexed staging remains.

Framebuffer readback requests pinned vitaGL's unconverted ABGR1555 variant
and performs channel conversion using ARM32 NEON instead of the library's
scalar loop. The existing six-image RGBA8 comparison/timing and fallback
remain in place. RDRAM writes use contiguous vector/pair stores, preserving
the recompiler's halfword twiddle, unaligned halfword starts and odd tails.
GPU synchronization, full readback, mutation checking and completion semantics
are preserved. This reduces CPU work; it does not eliminate GPU stalls.

Validation includes ASan/UBSan decoder/layout fixtures, all 65,536 ABGR1555
values, both halfword alignments and framebuffer boundaries. A freestanding
VitaSDK ARM32 fixture executes the production NEON path in Unicorn 2.1.4,
including all values and tails; this is not a native speed measurement.
All renderer translation units use the same patched header. See the matching
`reports/row-layout.json` for final build and execution evidence.

The original 9,416-export game core, native runtime, controller mappings,
audio DSP/worker and save paths are unchanged from 00.12. The cover art is
retained. Hardware visual accuracy, audio quality and FPS need confirmation.
The two-source TMEM approximation, base-mip fallback, synchronous graphics
and audio synthesis, and full framebuffer readback remain limitations.
