# v00.23: exact triangle math cache and frame-cost breakdown

User video IMG_1479 shows approximately 14–17 FPS at 320x240 and zero queued
audio milliseconds in sampled frames. This is a baseline observation, not a
complete performance profile or proof of a single bottleneck.

The generic Fast3D triangle path recalculates texture height and reciprocals
for repeated operands. Add exact memoization for texture byte-size/stride
quotients, texture dimension reciprocals, and per-vertex perspective W
reciprocals. Every operand is part of its key. Float keys retain their bits,
including signed zero. Changed values force a fresh division; this does not
require guest write tracking or assume textures/vertices are immutable.

No approximate reciprocals, frame skipping, lower resolution, omitted
framebuffer writes or changed audio sample rate are introduced. This is a
performance candidate; actual FPS improvement is hardware-unverified.

Live Stats now adds:
- CPU GAP: wall time from the prior completed renderer task to the next task;
  includes guest execution, audio synthesis, scheduling and other host work.
- LIST: display-list translation wall time.
- DRAW: renderer submission wall time; not a GPU utilization measurement.
- READBACK: framebuffer readback wall time, including GPU synchronization.

Values are averaged over roughly half-second windows. Settings entry resets
both FPS and stage windows. The log also reports division cache hits/misses.

Validation: one million changing-input math-cache cases, repeated hits,
maximum random integer numerators, nonzero strides, signed zero, infinities
and NaNs; scalar result comparisons pass under ASan/UBSan. FPS/stage timing
and reset checks pass. The full dependency preparation recipe applies from
the pinned renderer checkout and the actual game compiles/links with VitaSDK.
LeakSanitizer is disabled due to host /proc inspection restrictions. No Vita
or emulator gameplay test was performed for this candidate.

Test: install v023 over DUCKVIT04. Use the same scene, Native resolution,
AA Off, Live Stats On. Compare against v022 and send a short video plus
`ux0:data/DuckRecomp/game.log`. Keep v022 for rollback. The goal is stable
original game speed; no full-speed or 60 FPS guarantee follows from other
console-era games having native Vita ports.
