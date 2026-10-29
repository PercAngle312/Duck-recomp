# v029 ADPCM/resampling hot-path candidate

The v028 hardware log reports roughly 17–19 FPS in the video, later audio DSP
averages of 10–16 ms per task, a repeatedly empty PCM queue and growing
underruns. Readback's packed format and source-fence path are active. This pass
targets DSP sample access and vector element selection inside the existing
ADPCM/resampling handlers. It is not a replacement with another port's HLE audio.

## Native sample windows and element selection

LDV/SDV eight-byte windows now use native word shifts/masks at every byte
alignment, including the halfword alignments used while resampling. The kernels
preserve the word-swapped DMEM / reversed vector layout and all neighboring
bytes. DMEM-wrapping windows and other vector element positions keep the original
runtime path. No approximate filtering, pitch or ADPCM math is introduced.

76 constant `vt(e)` selection sites now use compile-time NEON element selection,
including direct identity, lane broadcast and quarter/half-vector patterns.
This removes the generic shuffle table, dynamic initialization guard and
out-of-line selection call from those sites. DSP accumulator, saturation,
rounding, loop state and guest completion behavior remain unchanged.

`rebuild_audio_rsp.py --native-samples` applies `patch_rsp_samples.py` to private
header copies. Use with v028's existing native options. Generated source and
header snapshots stay outside the public checkout.

## Verification and measured scope

* 32,768 Cortex-A9 fixture cases cover all 16 element modes, all DMEM addresses
  and alignments, high address bits, wrap fallback, partial vector slices and
  neighboring-byte guards.
* 768 complete synthetic ARM tasks compare all 8 MiB RDRAM, 4 KiB DMEM, vector
  registers, accumulator slices, flags and scalar registers against the old
  headers. These include actual ADPCM and four-tap resampling handlers, init /
  continuation modes, multiple pitches, and mixed command lists. Zero
  differences. The NAudio state-address and pitch/flag command fields are now
  encoded correctly, unlike earlier generic-ABI test guesses.
* In four synthetic cases per handler, executed ARM instruction counts fall
  from 85,732 to 66,268 for ADPCM (22.7%) and 307,500 to 191,979 for resampling
  (37.6%). Counts include the same test-only state capture on both sides.
  Instruction counts are not measured CPU time or a prediction of Vita FPS.

The complete game is relinked with the new DSP archive and matching v028 objects.
No renderer or audio-queue policy change is bundled. Compare v029 with v028 in
the same scene and retain game.log: FPS, Audio RSP avg_us/max_us, queued_frames,
underruns and graphics stage times are the useful measurements. Real-game audio,
visuals and actual frame-rate gains still require hardware testing.
