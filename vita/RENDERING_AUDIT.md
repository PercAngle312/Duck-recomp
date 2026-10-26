# Vita rendering integration audit — v00.07

Scope: production OSTask validation, microcode selection, memory bridge,
converted display-list/vertex/texture lifetimes, Fast3D batching, generated
shaders, vitaGL compilation/linking, texture binding caches, depth state,
framebuffer setup/readback and final presentation. Reviewed the pinned sources
and the live adapter; this is not a proof of correctness for every interpreter
opcode, texture format, generated shader combination or game scene.

The original dependency checkout and translated game code are preserved.
`experimental/graphics_replay/renderer.patch` is applied only to build copies.
Rebuild every renderer translation unit against the prepared include tree.

| Finding | Fix and scope |
| --- | --- |
| Fixed 10 MiB Vita vertex arena advanced without a per-task limit; next arena was allocated before swap | Reusable CPU batch, bounded at 256 triangles and 64 floats/vertex; draw checks exact shader stride, copies populated bytes into a fresh mapped GPU snapshot; no per-task pointer walk |
| Empty/truncated shader file caused unsigned allocation underflow and unchecked binary parsing | Private versioned cache envelope, maximum 2 MiB payload, exact length/layout/checksum checks; reject and compile; retain compiler-derived vertex stride |
| Shader cache shared another application's directory and omitted filtering/sRGB | `DuckRecomp/shaders-v007`; separate full shader IDs, filtering and sRGB keys; original caches untouched |
| Shader map narrowed the second 64-bit key to 32 bits | Keep both full 64-bit keys; current upstream options use low bits, so no claim of an observed game collision |
| Shader/program linkage not checked; shader handles retained after linking | Check compile and link results, flush IDs/compiler/source on failure, release attached shader handles after link; cached program also requires link success |
| Cached depth comparison ignored decal-only transitions | Reapply comparison when decal mode changes, alongside depth enable/write-mask state |
| Framebuffer texture setup bound texture zero and left software binding cache stale | Restore actual previous binding around framebuffer texture allocation/resizing; invalidate deleted texture IDs |
| MSAA resolve restored a vector index as a GL framebuffer handle | Restore the stored GL FBO handle; current production MSAA remains disabled, so this branch was compile-checked only |
| Matrix/vertex/viewport/light conversions read without checking complete guest spans | Resolve and validate span/alignment/count before allocation and read; reject unaligned list starts |
| Adapter cleared before interpreter bound the target, inheriting masks/scissor | Use the interpreter's existing target selection and clear; remove redundant external clear/viewport mutation |
| Guest pointer mask could accept unsupported virtual segments | Accept physical RDRAM and KSEG0/KSEG1 only; exercise alias rejection in host test |

## Validation boundaries

The pinned SDK compiles all five production renderer objects and the private
full game links with no unresolved strong symbols. Host tests exercise the real
cache helper with valid roundtrips, all truncated lengths, header/layout/length
and payload corruption, trailing data, and failed-write preservation. Guest
span tests check physical/KSEG aliases, alignment, end-of-RAM and overflow.

Actual execution results are recorded separately in `reports/rendering-audit.json`.
Emulator video is offscreen software OpenGL and audio is dummy. A successful
framebuffer swap is not a visually inspected frame. SP completion counts include
audio tasks and must not be reported as rendered frame counts. The emulator
reports some generated GLSL warnings and lacks real EEPROM sync semantics.

## Remaining areas requiring gameplay evidence

* Fixed 320×240 guest VI dimensions and per-task host presentation. Multiple
  graphics tasks per guest frame can expose intermediate images; proper VI
  framebuffer-driven scheduling still needs game traces and visual comparison.
* Current whitelist supports the observed F3DEX.NoN fifo 2.08 signature. Other
  microcodes fail explicitly; their presence later in the game is unverified.
* FullSync performs color-image readback. Other framebuffer formats/modes,
  render-to-texture effects and their native performance are not all validated.
* Upstream depth-query backend returns no data; a future game call to that path
  needs a real implementation. It is not replaced with a fake depth value.
* Texture/palette swizzles, clamp/filter edges, fog, alpha/blend and Z fighting
  require scene comparison. Shader code has a legacy depth scale; changing it
  without a rendering reference would be speculative and was deferred.
* The entire graphics/RSP task path is synchronous. Worker PCM output does not
  make graphics asynchronous or guarantee frame pacing and underrun-free audio.

No claim of playable Vita gameplay or successful physical v00.07 execution.
