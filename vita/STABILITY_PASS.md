# Stability consolidation after the physical 00.13 test

The current user build remains 00.13. This pass does not publish a new VPK.
The user's longer log confirms the packed readback path passes its reference
comparison, but drawing/readback still stall and translated audio synthesis
reaches roughly 24–25 ms per task during gameplay. Starvation remains a central
problem; successful linkage is not proof that it is resolved.

## Changes under validation

* Add opt-in `--fast-memory` to the private audio rebuild. Header copies receive
  aligned 16-/8-byte RSP vector transfer paths and word-layout-preserving DMA
  copies. All element/misalignment/wrapping cases outside those paths retain
  the original operations. Generated audio source and input headers are intact.
  The default build remains unchanged until explicitly enabled and verified.
* Interpolate the end of an audio DMA against the first frame of an already
  queued, owned snapshot. Previously that interval repeated the last sample.
  An absent next DMA keeps the old clamp behavior and never blocks completion.
  The host still retires playback completions in order; no extra FIFO depth,
  invented completion, delayed IRQ or resampling-rate workaround is added.

* Preserve residual resampling time when the next DMA changes frequency.
  The previous phase was in old input-frame units and was reused at the new
  rate. A focused test checks the first sample after a rate switch and verifies
  that subsequent chunks at the same rate are not scaled twice.

## Checks

The host RSP comparison covers 524,672 cases across every DMEM address, all
16 vector elements, aligned/unaligned access, negative immediates and address
wrap. Reference and optimized translation units rename their RSP classes to
prevent weak-method coalescing from making the comparison test itself.

A pinned VitaSDK ARM32 fixture executes both implementations in Unicorn 2.1.4:
3,072 vector cases plus all 4,097 DMA byte counts in both directions, checking
logical bytes, untouched guard areas and rejected spans. Separate private
execution compares the real 00.13 archive (all defined symbols isolated) with
the optimized archive on 128 synthetic complete translated audio jobs, including
load/save, clear/move, interleave and mixer operations. Whole 8 MiB RDRAM and
4 KiB DMEM outputs match. Synthetic jobs are not hardware gameplay tests.

Existing contracts cover controller centering, DMA/interrupts, message queues,
guest scheduling, shaders, material/texture layouts, persistent EEPROM reload,
short I/O, six save-transaction failures, backup recovery, uncertain-sync detach,
audio device failures, FIFO refills, owned snapshots and joined shutdown.
The audio fixture additionally verifies the new boundary sample and mutates
both original guest buffers after submission, ensuring interpolation consumes
owned data. See `reports/stability-pass.json` for final results and limits.

## Remaining work before another user build

Native frame-time and audible quality remain unmeasured for these changes.
The GPU still waits for full framebuffer readback; this pass does not skip it
or alter guest memory/completion semantics. Audio synthesis is still synchronous.
The renderer's two-source TMEM approximation and base-mip fallback remain.
Internal game execution/differential checks and final CI are recorded separately.
No claim that all bugs are fixed or that a new build is playable is made here.
