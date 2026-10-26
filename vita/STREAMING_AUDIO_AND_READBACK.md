# Vita 00.11: streaming audio and packed framebuffer readback

The user reports no improvement in textures, performance or sound on hardware
through 00.10. Its actual hardware log shows FullSync readback averaging
9.65–9.97 ms per graphics task, versus 0.20–0.25 ms presentation. Later scene
samples also show 6–16 ms draw and 3–9 ms translation. These are task timings,
not an FPS measurement; guest execution and loading add further elapsed time.

## Changes

* Use the pinned vitaGL GPU conversion path for RGBA5551 readback instead of
  CPU-reading RGBA8 and packing each pixel. Its framebuffers.c explicitly
  reswizzles native ABGR1555 transfer output into logical RGBA5551. Preserve
  FullSync and RDRAM synchronization. Compare six packed images with the previous RGBA8 reference, then select the
  faster correct path on that device. Exclude the shared GPU wait from both
  timings. Explicitly finish pending GPU drawing before either method: the
  pinned glReadPixels waits only when its read target is the in-use scene, but
  Run switches back to the display framebuffer before the game-FBO read.
  Log GPU wait separately from transfer/packing every 256 reads. Permanently fall back on mismatch/error. These initial comparisons
  do not establish every later pixel's accuracy.
* Preserve original source pitch when cropping RGBA16 and RGBA32 uploads.
  Include decoded-view dimensions, source/loaded/tile strides, loaded byte
  count, wrap flags and texture flags in texture cache identity. A texture
  address and byte count alone cannot distinguish differently decoded views.
* Cache exact normalized guest function PCs in a 1024-entry direct map.
  Invalidate on successful module load, removal/reuse and catalog reset.
  Freed modules remain callable for destructor cleanup until allocation reuse.
* Request CPU 444, bus 222, GPU 222 and crossbar 166 MHz, retaining existing
  settings when higher. Log actual clocks and API request results. Rejected
  clock requests are reported and do not pretend to have taken effect.
* Let the native sink own both AI FIFO buffers. The worker joins queued DMAs
  into fixed 256-frame stereo blocks rather than zero-padding, draining and
  waiting for host service at every DMA boundary. No port-length reconfiguration.
  Publish progress/completion after preceding native output finishes; the host
  still emits exactly one AI interrupt per successfully played DMA in order.
  Buffers are copied before publication; workers never retain live guest RAM.
  Pad/drain only when no next guest DMA is available. Log padded frames and idle
  drains every 256 completions to expose starvation instead of hiding it.

## Validation and limits

The concurrent audio fixture checks every stereo sample across a joined pair:
3072 output frames, no boundary zeros, no port reconfiguration, one idle drain.
It also checks blocked-device host progress, ownership after guest overwrites,
FIFO/IRQ order, output/drain/signaling failures and joined shutdown. AI tests
retain the single-active-sink contract and separately exercise capacity two.
Texture fixtures check cropped rows, logical cache fields, palettes and colors.
Overlay tests check cached address reuse with a different function and reset.

Native graphics tasks and translated audio RSP synthesis still run on the guest
host thread. Expensive game execution can still starve the producer. The linear
resampler still clamps its interpolation at each guest DMA edge; this is not a
band-limited, cross-DMA interpolation replacement. A single short/isolated DMA
can require up to 255 padding frames. Acceptable native FPS, visual correctness,
audible quality and stable gameplay require physical comparison. No proprietary
ROM, translated source/object, shader compiler module or save belongs in Git.

See reports/stream-readback.json for actual compilation/execution results.
