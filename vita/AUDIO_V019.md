# Buffered audio candidate v00.19

The prior native backend retired guest AI DMA only when its corresponding
speaker buffer finished. Rendering and guest scheduling can delay refills,
and the two-DMA stream has little reserve during long frames.

This candidate separates emulated AI completion from speaker presentation.
Guest submissions are converted to owned stereo PCM on the guest host thread.
A bounded single-producer/single-consumer ring feeds a native playback worker.
AI completion follows serialized DMA durations from the monotonic process
clock. Only the host thread emits guest interrupts. `osAiGetLength` reports
the active timed DMA, excluding presentation lookahead and the queued DMA.

Playback starts with 3072 frames (64 ms at 48 kHz), or after 80 ms for a short
isolated sound. Native output blocks are 256 frames. Underruns insert a padded
block and rebuffer. Backpressure above 6144 frames delays AI completion;
ring writes also wait for space rather than discard samples. The ring stores
16384 stereo frames (64 KiB). The resampler and optimized RSP synthesis from
v017 are retained; graphics/settings remain from v018.

This changes AI timing from physical speaker completion to emulated time.
It is a hardware-test candidate. It adds roughly 64–80 ms presentation latency
and cannot solve sustained CPU/GPU overload. Real game synthesis correctness
and audio quality remain unverified. Do not describe host tests as Vita tests.

Validation:
- One million concurrent stereo FIFO frames, including ring/cursor wrap.
- Actual backend under a host device model: timed completions before playback,
  prefill, two-DMA admission, short-sound timeout, RDRAM reuse, high-water
  backpressure, output failure propagation and clean worker join.
- Existing AI register/resampler and gap fade regressions.
- VitaSDK runtime compilation, link and full-game packaging.

Test on Vita: install v019 over DUCKVIT04, use Native resolution/AA Off for
comparison, and replay a scene with music and effects. Check speed, pitch,
crackles, dropouts and audio delay. Send `ux0:data/DuckRecomp/game.log`.
Keep v018 available for rollback. ROM and saves keep their existing paths.
