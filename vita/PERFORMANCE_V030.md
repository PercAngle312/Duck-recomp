# v030 native audio accumulator candidate

This build continues v029 with shorter ARM NEON fractional and mixed multiply
paths. It does not replace the whole NAudio algorithm or establish a Vita FPS
gain. Install over DUCKVIT04 and keep the same ROM/settings for comparison.

The 48-bit accumulator now uses three 16-bit vector limbs, with unsigned carry
comparisons between limbs. Mixed signed/unsigned products correct the high
product limb directly. Signed destination saturation compares the high limb
with the middle limb's sign extension. All accumulator bits still wrap exactly;
only destination values saturate. Inputs are read before writes, including
aliased destination/source registers. Vector flags and rounding rules retain
their previous behavior. Fractional/mixed helpers are explicitly inlined to
avoid GCC's inconsistent call/spill decisions in the large translated function.

## Validation

The existing scalar arithmetic oracles exercise 147,456 fractional/high cases
and 122,880 mixed/low cases on an emulated Cortex-A9, covering edge values,
carry boundaries, signed overflow, saturation and input aliases.

The complete-task comparison now runs reference and candidate in separate ARM
ELF images. This prevents weak template helper coalescing across versions.
960 synthetic tasks compare all 8 MiB of RDRAM, 4 KiB of DMEM and captured vector,
accumulator, flag and scalar state. Cases include clear/move/interleave/mixer,
ADPCM, resampling, mixed lists, and envelope initialization/continuation with
random saved state and consecutive envelope updates. A second 960-task run
compares RDRAM/DMEM using uninstrumented production DSP code.

Four seeds per command category count executed ARM instructions using the
production code, without hidden-state capture. Reference and candidate use the
same v028 native command prefixes, so the comparison isolates this change:

| Synthetic workload (four tasks) | v029 instructions | v030 instructions | Reduction |
| --- | ---: | ---: | ---: |
| ADPCM | 53,877 | 49,576 | 8.0% |
| Resampling | 190,386 | 170,322 | 10.5% |
| Envelope | 114,963 | 107,463 | 6.5% |
| ADPCM/resample/envelope/interleave list | 412,011 | 372,426 | 9.6% |

These are synthetic instruction counts, not hardware timings or FPS. They
include equal test DMA/libc shim work. Tiny clear/move workloads execute 12 more
instructions across four tasks; they do not exercise these multiply kernels.
Real scene command mix, memory/cache behavior, GPU work and guest CPU cost can
change the practical result.

## Reproduction and hardware comparison

Use the pinned VitaSDK and the v029 private checkpoint. Preserve the v029
patched headers and helper files in a separate reference include directory;
candidate headers use these public helpers. Compile the unchanged specialized
private DSP source, replace its archive and the coordinator version string, and
relink the exact checkpoint objects/libraries. The private v030 checkpoint also
contains the candidate header copies for later rebuilds.

Run `test_audio_commands.py` with `--reference-headers <v029 include>` and
`--reference-native-commands`. Its default 96 seeds produce 960 complete tasks.
Add `--measure-production --instruction-seeds 4` for production instruction
counts and RDRAM/DMEM comparisons; this mode intentionally omits hidden-state
capture. Run both existing ARM multiply fixtures. CI runs both arithmetic
fixtures; generated DSP code and game data remain private.

Test the same scene with Native resolution, AA Off and Live Stats On. Retain
game.log and compare Audio RSP timing, FPS and audio queue/underruns with v029.
Listen for envelope/volume artifacts and check a longer session. Hardware audio
correctness and FPS improvement are pending.
