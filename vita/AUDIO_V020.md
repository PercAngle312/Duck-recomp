# v00.20: resampler and native playback priority

Reviewed the supplied `vita-audio-fix.patch` against v019.

Accepted:
- A 32-bit resampler fast path for bounded phase/rate/input ranges, retaining
  the scalar 64-bit fallback. Unsigned interpolation magnitude avoids signed
  overflow and preserves truncation toward zero.
- Playback worker requests priority `0x10000080`, retries `0x10000100` if
  rejected, and logs both rejection and selected priority. Thread creation
  failure at both priorities remains a real initialization error.

Skipped the proposed `power_vita.c`: `experimental/graphics_replay/adapter.cpp`
inside `duck_fast3d_init()` already raises CPU/bus/GPU/XBAR to at least
444/222/222/166 MHz before renderer initialization, without lowering a user's
higher clocks. It logs actual clocks and request return codes. The full-game
link already includes ScePower. Existing user logs confirm those frequencies.

Validation:
- 6000 paired DMA cases, 53,510,430 stereo output frames identical to v019.
  Random chunking, changing rates, joined/nonjoined boundaries, legal maximum
  DMA size and forced 64-bit fallback were exercised. ASan/UBSan enabled;
  LeakSanitizer disabled because the host cannot inspect /proc task state.
- ARM Cortex-A9 -O2 assembly inspected: the 32-bit fast loop uses multiply
  arithmetic; software 64-bit division remains in rate-change/fallback paths.
  This is not a hardware speed measurement.
- Buffered audio tests cover preferred-priority success, rejected-priority
  default fallback, failure of both attempts, existing timed AI, prefill,
  worker shutdown, PCM ownership and output-error propagation.
- Existing stereo FIFO concurrency, AI/resampler and gap tests pass.
- Actual full game built and linked with the pinned VitaSDK. No hardware or
  emulator gameplay validation is claimed.

Retains v019's 64ms prefill, 256-frame 48kHz output and bounded presentation
FIFO. Rebuffering and synthesis/render contention remain possible causes of
stutter. This release does not establish that the audio problem is fixed.

Install v020 over DUCKVIT04. Compare the same scene using Native resolution
and AA Off. Send `ux0:data/DuckRecomp/game.log`; the log now shows selected
playback priority as well as queued frames, underruns and padded frames.
Keep v019 for comparison/rollback. Existing ROM/save paths are unchanged.
