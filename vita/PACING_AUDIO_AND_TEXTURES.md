# v00.09 — CPU, textures, audio pacing and deliberate camera controls

User hardware feedback for v00.08: rendering looks better but texture errors and
slow FPS remain; sound is distorted/laggy. Movement appeared to become camera
control. The original USA manual (page 8) documents C-up camera look: while in
this mode the Control Stick moves the view, and C-up again exits. This explains
the reported behavior as a possibility; it does not prove which input occurred.
Manual: https://www.gamesdatabase.org/Media/SYSTEM/Nintendo_N64//Manual/formated/Daffy_Duck_Starring_as_Duck_Dodgers_-_2000_-_Infogrames.pdf

## Changes

* Rebuild the private checkpointed core from the same 194 translated source
  units using -O2 -frounding-math -fno-strict-aliasing, with no added fast-math.
  All 9,416 game exports are preserved. Source and checkpoint scheduling stay
  unchanged. The previous O1 object and v00.08 package remain available.
  Export parity is not proof of identical runtime behavior or a hardware FPS gain.
* Decode CI8 pixels after applying the final dimensions, preserving the source
  pitch when cropping. Previously a full-width decoded buffer could be uploaded
  with a narrower width, changing where every row began.
* Honor intensity/alpha TLUT entries for CI4/CI8, keeping RGBA5551 for RGBA TLUTs.
  Palette interpretation is part of texture cache identity; changing it marks
  both texture slots dirty. Cache equality compares fields instead of padding
  bytes with memcmp, preventing padding-dependent misses. Synthetic helper tests
  cover palette mode and keys with intentionally different padding bytes.
* Avoid redundant native audio port configuration. Drain queued output before
  changing native block length. Report actual block-level active-DMA progress to
  osAiGetLength; the previous result stayed full until completion. FIFO status,
  successful-playback completion and main-thread-only IRQ delivery are preserved.
* Add Triangle=C-up look toggle and Square=C-down camera reset; leave left-stick
  movement intact. Stronger vertical right-stick thresholds reduce inadvertent
  look toggles while retaining all C directions and horizontal hysteresis.
  Log the first eight C-up rising edges, with Triangle/right-stick origin.

The texture and audio changes correct real source issues, but not every visual
artifact or sound problem is proven solved. Hardware comparison is still needed.
The shader cache is compatible and requires no folder rename. ROM and saves are
not installed, erased or replaced by the update.

## Validation

The optimized 194-unit core compile and ARM partial link check game exports.
The pinned SDK compiles all 29 runtime sources and five prepared renderer units;
the full private game ELF must link with no unresolved strong symbols. Shared
color/key tests, native controller tests, AI tests and a concurrent audio-worker
SDK-boundary fixture exercise the new paths. The worker test blocks playback
partway through a DMA and verifies decreasing length without an early IRQ, and
rejects reconfiguring a still-queued device buffer. These are host/SDK checks,
not audible native sound verification. Actual execution: reports/pacing-audio.json.

## Still unfinished

Native PCM output remains per-DMA with 64-frame final-block alignment, zero
padding, clamped edge interpolation and final drain. It can still starve while
native graphics/translated RSP work runs; a worker does not make synthesis
asynchronous. osAiGetLength progress is block-granular. FullSync readback and
fixed-VI per-task presentation remain. Arbitrary texture/microcode support,
visual effects, prolonged stability, saves and acceptable FPS are not declared
complete. Do not remove necessary guest work or fake completion to raise counters.
