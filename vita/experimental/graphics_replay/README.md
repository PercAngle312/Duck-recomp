# Vita Fast3D renderer integration

The production game and historical replay share the same adapter, pinned
Daedalus Fast3D memory bridge/interpreter and vitaGL backend. The private full
VPK (DUCKVIT04) executes translated guest threads and real SP graphics/audio
submission. The standalone replay builder below still produces DUCKVIT03 and
requires captured task data; it is not the production game package.

Hardware v00.05 reached the first display list, then failed on unsupported
`textureSize`. v00.06 enabled vitaGL's size-uniform compatibility and completed
its first task and framebuffer swap in Vita3K. Neither establishes physical
Vita gameplay. Rendering, controls, audible sound and saves need gameplay checks.

## Build

Check out every repository at its exact commit in `dependencies.json`, beneath
one dependency directory, retaining the directory names used in that file.
Install the SDK pinned in `../../toolchain-lock.json`. With Python 3.12+, make,
curl and git, run:

```sh
python3 vita/experimental/graphics_replay/build.py \
  --vitasdk /path/to/vitasdk \
  --dependencies /path/to/dependencies \
  --output /path/outside/public-repository/replay-build
```

The script verifies source commits, checks the taiHEN SDK package checksum,
builds the graphics dependencies, compiles the actual backend, links the ELF,
checks strong imports, and packages the SELF/VPK. It uses the SDK sample LiveArea
assets from a pinned commit. No private game assets are included in the VPK.
The C shader helper uses GNU11 explicitly because GCC 15's newer default C
semantics reject the upstream legacy taiHEN callback casts.

These upstream renderer dependencies include GPL/LGPL code. Preserve their
licenses and corresponding source when redistributing this linked application;
repository URLs and exact revisions are in `dependencies.json`.

## Runtime inputs

The private checkpoint supplies these files, outside the public repository:

* `ux0:data/DuckRecomp/first-task-rdram.private.bin`: exactly 8 MiB, word-swapped
  guest RAM captured at the graphics submission boundary.
* `ux0:data/DuckRecomp/first-task-address.txt`: the hexadecimal guest task address.

Install `Duck-Vita-Graphics-Replay.vpk` (title ID `DUCKVIT03`) and copy those
private files to the stated directory. The Vita needs its own working
`libshacccg.suprx` shader-compiler setup, as required by vitaGL. No Sony system
modules are supplied. Press Start to exit. Diagnostics are written to
`ux0:data/DuckRecomp/graphics-replay.log`.

Visual inspection must establish whether actual task output appears correctly;
a submitted-task log line alone does not establish this. Report the log and the
visible output if hardware testing is performed. It is not gameplay validation.

## Limits

Only the captured F3DEX.NoN 2.08 microcode family is accepted. An unrecognized
microcode aborts rather than being interpreted as a supported one. The adapter
uses a 320x240 native image and a 4:3 viewport on the Vita display; it does not
implement guest VI mode changes or final framebuffer presentation timing.
Every guest memory page is marked dirty before translation until guest-store
tracking is integrated. It renders each replay task synchronously and remains
separate from the guest SP/DP scheduler. Audio, saves, game input and gameplay
are outside this graphics replay.

## v0.02: initialization return-value correction

The first hardware run returned to LiveArea after logging initialization error
`-2`. That was an application error: the pinned vitaGL `vglInitWithCustomSizes`
returns `res_fallback`, initialized to `GL_FALSE` and set only when the requested
resolution is reduced. `vglInitExtended` forwards that value. Normal native-size
initialization therefore returns zero. The first replay incorrectly interpreted
zero as a failure and exited before creating the Fast3D interpreter.

Version 00.02 removes that inverted check, prevents repeated initialization,
and logs the resolution fallback value, viewport, shader compiler messages,
translation, rendering and framebuffer swap stages. The user-reported exit
verifies that the original app ran up to this boundary; it does not verify
rendering. The updated build still requires a new hardware graphics check.

## Renderer integration entry point

The adapter now exports `duck_fast3d_submit_task(uint32_t task_guest_address)`
through `fast3d_task.h`. The legacy `duck_fast3d_replay` calls this same
path. The diagnostic purple frame has been removed; this code path now only
presents frames driven by submitted guest graphics tasks.

The real SP runtime passes each graphics OSTask to this adapter using the
same live 8 MiB RDRAM. It emits SP completion only on successful submission,
and DP completion only when a FullSync command was observed. Audio tasks use
the translated audio RSP path and never enter Fast3D. This establishes execution
wiring, not visible gameplay validation.

## Private full-game linkage

The production runtime is built in the pinned VitaSDK workflow; its linked
runtime-only ELF is **not** the game. For an actual private full-game attempt,
use `vita/link_private_game.py` outside the public checkout, providing the
translated ARM32 game objects, a real scheduler-starting entry object, the
Fast3D renderer object directory, additional private callback objects through
repeatable `--extra-object` options, and the pinned renderer libraries via
repeatable `--library` options. Missing objects and unresolved strong symbols
are fatal. Never commit the private game link output or its derived objects.

## Shared renderer compiler preflight

The live game and replay share `adapter.cpp`. Initialization now invokes the
pinned vitaGL compiler initializer before constructing Fast3D or starting guest
execution. Failure returns `-8`, logs both supported `libshacccg.suprx` paths,
and flushes the error. The module is not distributed with this project.

This checks module initialization, not successful shader compilation or rendered
output. The pinned vitaGL's `GL_SHADER_COMPILER` query always returns true, even
when the module is absent, so it cannot serve as a dependency check. The internal
`start_shader_compiler` symbol is tied to the locked vitaGL revision; updating
that dependency requires checking its ABI and behavior again.

The pinned SDK workflow also compiles this shared renderer against its exact
locked public dependencies. That CI artifact is an object, not a playable build.

## Required texture-size translation

Build vitaGL with `HAVE_GLSL_TEXTURE_SIZE=1` as well as `NO_SPLASHSCREEN=1`.
Fast3D emits `textureSize(uTexN, 0)` and `textureSize(uTexMaskN, 0)` for
filtering and clamping. The Vita compiler cannot compile that GLSL intrinsic
directly. This pinned vitaGL option rewrites these calls into sampler-size
uniforms and fills them with the currently bound texture dimensions on draws.
Its level-zero limitation matches Fast3D's explicit level-zero queries.

Run `make clean` before changing this feature: make does not invalidate object
files when command-line feature variables change. Shader serialization and
uniform/draw handling must all use the same feature setting. The build script
performs this clean rebuild. Keep the generated shader cache from older builds
separate or remove only `ux0:/data/DaedalusX64/ShaderCache_F3D` before testing
this update; it contains derived shader binaries, not game saves.

Physical v00.05 execution reached guest audio DMA completions, translated its
first display list with FullSync, and failed compiling `textureSize` at the first
Fast3D task. Enabling the translation addresses that reported compiler error;
rendering and gameplay still require execution validation.

## Renderer audit update (v00.07)

`prepare_renderer.py` applies `renderer.patch` to a build copy of the exact
pinned dependency. It preserves the upstream checkout and rejects an output
inside a source repository. Both production/private builds and replay builds
must use the prepared include tree for **all five renderer objects**; mixing
unpatched headers and patched objects can change class layout.

Changes reviewed against the actual source:

* The reusable CPU triangle buffer holds at most 256 triangles, with room for
  64 floats per vertex. Each GL draw copies exactly its populated bytes into a
  separate mapped vitaGL scratch allocation. This removes the fixed 10 MiB
  arena, unchecked per-task pointer advance, and allocation across buffer swaps.
* Program binaries use an app-owned `DuckRecomp/shaders-v007` directory, keys
  containing both full 64-bit shader IDs, filtering and sRGB mode, and a bounded
  envelope with version, vertex layout, binary format, length and CRC. Truncated,
  stale-layout and corrupt files recompile. Writes use a temporary file then
  rename; cache failure is nonfatal. The cache is derived data, not a save file.
* Compiled and cached programs must link successfully before use. Failed
  compilation/linking prints the shader ID and compiler information and stops;
  no placeholder shader masks the problem. Attached shader handles are released
  after linking. A rejected cached program recompiles from source.
* Depth comparison is refreshed when decal mode changes. Temporary framebuffer
  texture allocations restore the binding they displaced. Deleted texture IDs
  invalidate their binding-cache entries. MSAA restore uses the GL FBO handle,
  rather than the renderer's vector index.
* Matrix, vertex, viewport and light conversions check guest spans before reads;
  display-list starts must be 8-byte aligned. Unknown microcode still fails.
* The adapter lets the interpreter bind and clear its own framebuffer instead
  of clearing an inherited target with stale scissor/depth-mask state.

The native CI job now applies the same patch and compiles the adapter, bridge,
compatibility layer, interpreter and GL backend. The host cache regression
covers truncated files at every length, stale layout, malformed headers, length
limits, checksum changes, trailing bytes and failed-write preservation.

Remaining limitations: fixed native VI dimensions, synchronous graphics/RSP
submission, per-task presentation, empty depth-query implementation in upstream,
and no demonstrated arbitrary microcode support. Texture formats, blending,
depth, framebuffer readback and effects still need in-game visual comparison.
A code audit cannot guarantee every path is correct or that the next hardware
run will reach gameplay.

## Native resolution and material colors (v00.08)

The production adapter now defaults to 320x240 scene rendering with an explicit
blit into the existing 4:3 display viewport. Both MOVEWORD light-color updates
and MODIFYVTX RGBA updates are implemented using material_updates.h. FullSync
readback retains guest pixels, reads a matching game FBO directly at native
size, and packs byte RGBA into N64 RGBA5551 explicitly. See
../../COLOR_AND_PERFORMANCE.md for the hardware report, rationale and limits.
All five renderer objects now use -O2 without added fast-math, and the adapter
logs average phase timings every five seconds. No shader-cache rename is needed.

For v00.09, see ../../PACING_AUDIO_AND_TEXTURES.md. CI8 uploads now retain the
correct row pitch after cropping; CI TLUT intensity/alpha mode and cache palette
identity are handled. Texture keys compare fields rather than struct padding.
The current private core is rebuilt with O2 and dynamic-rounding semantics;
this is independent of the generic public renderer build.

For v00.11, see ../../STREAMING_AUDIO_AND_READBACK.md. Readback compares and times
six packed/reference images, uses the faster correct device path, and explicitly
finishes pending GPU scenes before CPU-visible FullSync output. RGBA crops retain
source pitch; texture cache identity includes decode-view dimensions and strides.
The adapter requests and logs standard performance clocks without lowering higher
user settings. Add ScePower_stub when linking the adapter.

For v00.12, see ../../TEXTURES_AND_PACING.md. Palette bank revisions and tile-mask,
UV-slot, nibble and source-pitch fixes address texture state. A staging index and
scoped source/palette invalidation avoid repeated scans and global GPU cache
flushes. The prepared patch now also updates Fast3DMemoryBridge.h; compile all
five units against the same prepared headers.

For v00.25, see ../../PERFORMANCE_V025.md. Prepared renderer builds require the
matching source-fence vitaGL library, generated by rebuild_vitagl_fence.py.
The original library cannot satisfy the new readback wait symbol.
