# Validate texture sources on demand

Previously every display-list build walked all historically staged textures to
mark their content dirty. The live adapter first marked every RAM page dirty,
because translated guest writes are not comprehensively instrumented. This
meant the walk grew with the historical cache, even when the current task used
only a few textures.

The bridge now checks each requested source on its first use in a task. The
existing content hash, row-layout identity, source copying and selective GPU
cache invalidation remain in place. Repeated uses in the same task reuse that
validation. There is no per-task walk of inactive texture stages and no blanket
dirty-page fill in the live adapter. The dirty-page API remains available for
compatibility; it is not used to skip content checks.

This deliberately retains conservative hashing. It does not assume RAM stayed
unchanged, trust incomplete dirty-page tracking, or suppress framebuffer copies.
At build-counter wrap the bridge clears validation tags once, preventing a
dormant old tag from incorrectly matching a new task's serial.

The regression fixture compiles the actual StageTextureImage and BuildDisplayList
methods from the previous and patched bridge. Controlled RAM, hashing and GPU
callbacks isolate validation behavior. It checks staged byte content, changes
between tasks, repeated uses, two swizzle layouts, cache growth and serial wrap.
It does not test the NEON hash implementation or display-list decoding.

With 4,096 staged textures and 128 subsequent sparse-use tasks, both methods
perform 5,124 hashes and 1,026 texture updates. The previous method visits
532,482 historical entries; the new method visits none during normal builds.
Both compile and run with AddressSanitizer and UndefinedBehaviorSanitizer.

All five production renderer units compile with the pinned VitaSDK, and the
complete private game links without unresolved strong imports. Exact execution
results are in `reports/texture-validation.json`.

This reduces CPU cache-maintenance work. It is not a measured Vita FPS gain:
GPU draw/readback stalls and translated audio synthesis remain separate costs.
The user v00.13 release is preserved; no new installable release is issued by
this change.
