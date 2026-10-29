# v028 native routine candidate

This build addresses all three requested areas: audio command work, geometry
math, and guest memory utilities. v027 remains the rollback reference. There
is no measured Vita FPS improvement or complete real-game audio validation yet.

## Audio command kernels

Four pinned DSP handlers use native clear/move/interleave/mixer prefix kernels.
They retain the original final iteration to reproduce architectural register and
accumulator state, not only PCM output. The mixer uses widened arithmetic with
both products, the original rounding contribution and saturated destination.
It preloads the following input/output block before storing the current one,
including overlapping buffers. Unaligned, wrapping or unsupported spans retain
the original implementation. No asynchronous task/completion semantics change.

480 complete synthetic ARM tasks compare all 8 MiB RDRAM, 4 KiB DMEM and captured
vector registers, all accumulator slices, flags and scalar registers. They pass
with zero differences, including mixed command lists. A separate public ARM
fixture covers 8,192 kernel cases and boundary/overlap/preload behavior.
ADPCM decoding, resampling and envelope routines still execute their existing
translated paths; they are not claimed as newly native or fully validated here.

`rebuild_audio_rsp.py --native-commands` requires the exact pinned private source
hash. Its patcher writes a private specialized copy, preserving original source.
Use with the existing native NEON multiply, partial-load and fast-memory flags.
`test_audio_commands.py` accepts the private source, patched headers, runtime,
SDK and an output directory outside this repository; it captures the complete
DSP state and runs the synthetic comparisons without shipping game source.

## Matrix conversion and vertex positions

Fixed-point-to-float matrix conversion and four-lane vertex position transforms
use NEON. Products/additions retain their original association; there are no
reciprocal approximations. Matrix conversion uses the native path only with
nearest rounding. Vertex transforms additionally require bounded normal matrix
values, so exceptional inputs and other rounding modes use the scalar path.
The existing native matrix multiplication and normalization helpers remain.

327,680 Cortex-A9 comparisons match VFP scalar results bit for bit. Tests include
signed zero and exceptional/rounding fallbacks. The full conversion regression
also passes 98,304 actual-method comparisons, with all earlier source validation,
arena ownership and serial-wrap guarantees retained.

## Guest copy, zero and byte fill

Three verified resident utility functions receive bounded native memory prefixes.
The native copy path only handles disjoint, word-aligned spans. Overlaps and
unsupported input use the original routine. Fill/zero preserve word-swapped byte
lanes, neighboring bytes and the original tail/register effects. The byte-fill
routine's stack write remains in its original position.

Private specialization verifies both the full game-object SHA256 and each
mapped generated function's SHA256. It retains entry/backedge cooperative
checkpoints. Prefixes are capped at 256 bytes per loop visit; the original final
block/tail still executes. It weakens only the three original public symbols
and links strong replacements, so direct calls and catalog references resolve
to the same replacement; unrelated code/overlay mappings remain unchanged.

12,288 host comparisons pass with ASan/UBSan across aliases, alignment, overlap,
zero/negative sizes and full context bytes. Another 768 Cortex-A9 cases compare
the new functions against the actual v027 compiled guest functions, with the
checkpoint hook stubbed for isolated execution. Hardware scheduling still needs
testing. `specialize_guest_memory.py` consumes an explicit private mapping;
it never guesses function addresses or publishes generated code.

## Build and hardware check

Recompile all five prepared renderer units. Link the new native DSP archive,
`native_memory.c`, specialized private utility object and matching weak-symbol
game object. Keep the existing framebuffer-fence vitaGL library. Private source,
maps and translated binaries stay outside this public repository.

Compare v028 with v027 in the same scene at Native / AA Off / Live Stats On.
Check FPS, frame time, LIST/DRAW/READBACK, audio queue/underruns, camera, animated
geometry, lighting, effects, controls and saves. Keep game.log. Unit/ARM/CI
checks establish bounded behavior; they cannot demonstrate a playable frame
rate or validate every audio command sequence on real hardware.
