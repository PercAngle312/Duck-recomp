# Audio timing correction after v00.16

The user reports that v00.16 audio is still distorted. The gap fades in that
build did not establish an audio fix.

## Start accepted asynchronous DMA immediately

`osAiSetNextBuffer_recomp` previously only appended a FIFO entry. The owned PCM
snapshot did not reach the native playback worker until a later call to
`duck_vita_ai_service`. A long graphics task or guest computation between those
calls could leave already-available PCM waiting while the device ran dry.

The accepted enqueue now starts available asynchronous sink slots immediately
under the existing host-thread processing guard. A shared helper preserves the
one/two-slot sink capacity. Starting copies/owns the PCM; it does not poll the
device, dequeue a FIFO entry, deliver an interrupt or wait for playback.
Completion and failure remain on the existing service path. A device start
failure is latched for service, with no successful completion manufactured.
Synchronous sinks retain deferred service to avoid blocking this guest syscall.

The host native-worker regression now reaches a blocked native output call
without any intervening scheduler service. A two-second deadline prevents a
silent hang. Its subsequent snapshot mutations, FIFO ordering, remaining-byte
progress, device failures and joined shutdown tests still pass. The pure AI test
checks immediate ownership and absence of early IRQs for one/two-slot sinks.
The pre-change runtime fails the new immediate-submission assertion.

This corrects an avoidable enqueue delay. It does not prove how much that delay
contributed to the user's particular gameplay audio or eliminate other stalls.

## Native high multiply, conservatively enabled

The translated audio DSP contains 35 static VMUDH/VMADH calls, including ADPCM
decode arithmetic. New ARM32 NEON helpers use widened signed multiplication
and multiply-accumulate directly, preserving modulo accumulator arithmetic,
the low accumulator slice, saturation and aliased source/destination operands.

Enable this optional path using `rebuild_audio_rsp.py --mode neon --fast-memory
--native-multiply --native-high-multiply`. The new flag requires the existing
native multiply flag; previous invocations retain their previous high-multiply
implementation. Generated private DSP source is unchanged.

Validation completed before enabling it in the private v00.17 candidate:

- 147,456 ARM32 arithmetic cases across fractional and high multiply, including
  all 48 accumulator bits, saturation boundaries and source/destination aliases.
- 192 complete synthetic tasks compare all 8 MiB RDRAM and 4 KiB DMEM against
  the v00.16 DSP: load/save, clear/move, interleave, mixer, ADPCM initialization
  and continuation, and resampling initialization and continuation.
- The same 192 tasks also match the pinned scalar DSP, independently of the
  existing SSE2NEON path. This is not a recording of actual game audio tasks.

Two representative synthetic task instruction counts fell from 15,795 to
15,354 (ADPCM) and 136,657 to 135,939 (resample). These modest instruction-count
reductions are not Vita timing, an FPS result or proof that the sound improved.

## Remaining verification

The v00.17 full-game candidate retains v00.16 rendering and audio gap fades.
Actual playback needs hardware confirmation. Graphics blocking, guest timing,
real synthesis workload and audio starvation remain open; no stable-audio or
complete-gameplay claim follows from these checks. Private game code, ROM,
capture memory and shader compiler remain outside public Git.
