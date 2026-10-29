# v00.24: request-driven native input and texture batch coalescing

CPU host-service pass: Vita controller sampling previously ran on every guest
scheduler service, including repeated checkpoints with no pending SI request.
The native reader also configures sampling mode, so this made two native API
calls per service. Now poll for the first snapshot and for queued guest SI
read/query requests. SI completion still latches the snapshot and emits its
normal event. Audio, timers and PI retain their per-service cadence. This
reduces host work on the same thread as game/audio synthesis; it is not a
replacement audio synthesizer or a claim that game CPU work has been rewritten.

Rendering: remove the unconditional pre-import Flush on a texture load. The
production TextureCacheLookup now flushes before a different cached binding,
upload or eviction. A hit of the identical complete cached texture node can
retain pending triangles. Sampler/shader/depth/blend changes keep their existing
flushes. Framebuffer texture binding clears the cached-node tracking, forcing
a subsequent ordinary texture switch to flush. Geometry order and the existing
256-triangle capacity are unchanged; no draw sorting or skipped geometry.

Live Stats adds DRAWS and TRIS for the most recent game frame. Log totals add
kept_batches, draws and triangles. These show whether the batching change helps
the actual workload. GPU readback synchronization is preserved: pinned vitaGL
only waits automatically for certain active-FBO reads, so removing the explicit
finish without a replacement fence would risk stale framebuffer data.

Validation:
- Production live service exercised for 20,000 idle services: two native reads,
  with audio/timer servicing retained every pass. Subsequent SI read/query
  sampling and native-read failure paths pass. Unchanged v023 fails the native
  read-count assertion.
- Existing native button/stick mapping, SI snapshot/event/ABI tests pass.
- Executable production TextureCacheLookup body under a draw-order spy: same
  node retained; binding changes, framebuffer transitions, uploads and eviction
  flush before rebinding/reuse. ASan/UBSan pass. This is not a GPU test.
- Existing audio FIFO/resampler/worker regressions pass.
- Pinned renderer preparation and actual full VitaSDK build/link pass.

No physical Vita or emulator gameplay testing was performed for this build.
FPS gains, physical controller behavior and texture correctness remain to be
compared with v023. Install over DUCKVIT04, use the same scene at Native/AA Off,
enable Live Stats, and send a short video plus ux0:data/DuckRecomp/game.log.
Keep v023 for rollback. ROM/save paths and previous audio behavior are retained.
