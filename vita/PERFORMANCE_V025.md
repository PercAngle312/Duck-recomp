# v025 performance candidate

All three requested areas are implemented; real Vita FPS gains are unmeasured.
Install the candidate over DUCKVIT04 and keep v024 for comparison/rollback.

* Native ARM audio: five more translated RSP vector multiply operations
  (VMUDM, VMUDN, VMADM, VMADN, VMUDL) execute direct NEON arithmetic. Element
  selection remains in the pinned runtime; full 48-bit accumulator behavior,
  saturation and aliased operands are preserved. This extends the earlier
  fractional/high multiply kernels, without replacing the complete audio task
  dispatcher or guessing guest function addresses.
* Drawing: triangle capacity increases from 256 to 1024. Existing allocation
  and bounds checks use the same limit (approximately 576 KiB additional worst
  case CPU staging). Required texture/state transitions still flush in order.
  Per-program frame/noise uniform stamps avoid repeated uploads of identical
  values when revisiting a shader within the same frame.
* Readback: each submitted framebuffer scene records its existing GXM fragment
  notification. Reads wait for that source framebuffer rather than all later
  context scenes. After waiting, the pinned vitaGL GPU format-transfer helper
  completes the copy into CPU-owned memory. Missing/collided/invalid fences,
  notification errors and serial wrap retain the original full wait. The first
  six calibration images compare against the original full-wait RGBA8 path;
  a mismatch disables both optimized readback paths. No framebuffer readbacks,
  guest completion events, effects or game frames are skipped.

## Rebuilding

`rebuild_audio_rsp.py --native-mixed-multiply` requires NEON, in addition to the
existing `--native-multiply --native-high-multiply --fast-memory` configuration.
Use private generated RSP source/headers; they are not published here.

`experimental/graphics_replay/prepare_renderer.py` applies the drawing and
readback renderer patches. Recompile all five renderer translation units.
The renderer must link a matching patched vitaGL library, not the old archive:

```
python3 vita/experimental/graphics_replay/rebuild_vitagl_fence.py \
  --vitagl /path/to/pinned/vitaGL --vitashark /path/to/vitaShaRK \
  --mathneon /path/to/math-neon --vitasdk /path/to/vitasdk \
  --baseline /path/to/stable/libvitaGL.a --output /outside/source/gl-fence
```

The script requires the pinned vitaGL gxm.c (76f43e6) and matching build flags.
It compares the original compiled `.text` to the baseline object before replacing
only gxm.o. Other archive objects, shader helpers and their licenses are retained.
The LGPL vitaGL modification is published in `vitagl-readback-fence.patch`.

## Verification and limits

122,880 Cortex-A9 emulated arithmetic cases cover all five operations against a
scalar oracle, including all accumulator bits, saturation and input aliasing.
128 synthetic complete audio tasks compare all 8 MiB RDRAM and 4 KiB DMEM against
the previous object, with zero differences. These fixtures cover load/save,
clear/move, interleave and mixer commands; they do not validate full real-game
ADPCM/resampling output. No real-game audio capture is claimed.

Host sanitizer checks cover draw bounds, per-program uniform stamps and the
actual source-fence wait function with mocked GXM calls (including fallbacks).
They cannot validate GPU behavior, image quality or hardware speed. Test the same
scene at Native resolution, AA Off, Live Stats On. Compare FPS and CPU GAP/LIST/
DRAW/READBACK, DRAWS/TRIS and underruns with v024. Check framebuffer effects and
listen for new artifacts; revert if behavior regresses. Logs include fence_reads
and full_waits every 256 readbacks. GPU transfers still wait for completion; this
is selective synchronization, not a fully asynchronous renderer.
