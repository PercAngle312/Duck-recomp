# Vita port implementation comparison

Audited 2026-10-27 against the pinned source revisions below. The reference
user build remains v00.13. This investigation does not change production audio,
framebuffer synchronization, or guest completion events.

## Useful implementations

| Project and inspected revision | Relevant implementation | Application to Duck Dodgers |
| --- | --- | --- |
| [DaedalusX64-vitaGL](https://github.com/Rinnegatamante/DaedalusX64-vitaGL/tree/fb7238e2d4b3600e5e5c84986871be1396e337af) | `Source/HLEAudio/NAudio.cpp`, `Source/HLEAudio/ABI_MixerInterleave.cpp`, `Source/SysVita/HLEAudio/AudioPluginVita.cpp` | Closest candidate: native NAudio command kernels and a separate synthesis worker. Compare kernels with this game's actual translated DSP before replacement. |
| [sm64-vita](https://github.com/Rinnegatamante/sm64-vita/tree/1dcb595f9306d0f546eb2bf173a02de641b3d5f8) | `src/pc/pc_main.c`, `src/pc/gfx/gfx_vitagl.c` | Native synthesis chooses a high/low sample count from queued PCM; GPU rendering avoids an unconditional full-color RDRAM mirror in this display path. These are architectural references, not drop-in game code. |
| [Shipwright](https://github.com/Rinnegatamante/Shipwright/tree/cefd9bb1d2841954fb6cc9f70b261c36e844aae0) | `soh/soh/OTRGlobals.cpp`, `libultraship/src/graphic/Fast3D/gfx_opengl.cpp` | Separate audio generation worker, queued-sample targets, and explicit shutdown synchronization. The inspected graphics path has depth-query readbacks, rather than our per-task full-color mirror. |

Daedalus's README also specifies renderer-specific vitaGL build flags, including
`READBACKS_SPEEDHACK` and `HAVE_UNFLIPPED_FBOS`. These are not independent safe
performance switches: adopting them requires matching orientation, ownership,
and synchronization assumptions throughout our renderer.

## Audio experiment actually performed

The game's captured audio microcode command layout and scratch buffer locations
match the NAudio family: main scratch at `0x4f0`, dry channels at `0x9d0` and
`0xb40`, with `0x170`-byte blocks. This identifies a useful candidate; it does
not establish that every upstream command has the same behavior.

Nine pinned Daedalus HLE audio translation units compiled with the VitaSDK for
Cortex-A9 hard-float ARM32. A freestanding comparison harness executed the ARM
code in Unicorn against the existing translated DSP. It compared all 8 MiB of
RDRAM after each synthetic task. The translated reference and optimized DSP
were also checked against each other, including DMEM.

| Command sequence | Unmodified upstream HLE | Candidate with exact mixer |
| --- | --- | --- |
| Load/save | 32/32 matching | 32/32 matching |
| Clear/move | 32/32 matching | 32/32 matching |
| Interleave | 32/32 matching | 32/32 matching |
| Mixer | 0/32 matching | 32/32 matching |

The mismatch is meaningful: this microcode scales the existing output by
`32767/32768`, applies the RSP accumulator rounding term, then accumulates the
input contribution and saturates. The generic upstream mixer does not reproduce
that operation exactly. The isolated candidate uses a 64-bit accumulator and
the observed preload/store ordering. Its 128 matching cases are synthetic
tests of four command sequences, not complete audio compatibility or a measured
speed improvement.

Other integration hazards found:

- `AudioHLECommand` stores `cmd1` before `cmd0`; raw copying of task words into
  that structure swaps the command fields. Explicit field assignment is needed.
- The harness rejects unsupported opcodes instead of accepting upstream UNKNOWN
  handlers as successful implementations.
- DMA alignment/count masking differs between some generic HLE helpers and the
  translated DSP. The tested sequences use compatible aligned addresses.
- The synthetic fixture resets HLE state for each task. Production must preserve
  ADPCM, resampling, envelope, loop, and inter-task state correctly.

The private experiment remains outside the production build. No generated game
source, captured RAM, ROM data, or private executable is included in this report.
The two machine-readable result summaries are in
`reports/other-ports-audio-comparison.json`.

## Implementation order justified by these sources

1. Extend shadow comparisons to ADPCM, resampling, envelope mixing, loop state,
   overlapping buffers, and real consecutive game tasks. Keep translated DSP
   authoritative until output and state comparisons pass. Retain upstream
   license notices and meet GPL obligations if distributing its implementation.
2. Move synthesis work off the guest/graphics execution path only with owned
   task inputs and preserved guest completion ordering. Our asynchronous PCM
   playback worker does not by itself make DSP synthesis asynchronous. Copying
   only another port's worker entry point would not establish safe scheduling.
3. Trace guest framebuffer consumers and image lifetimes before introducing
   selective/deferred color readback. Native decomp ports do not prove that this
   statically translated game can run safely without its RDRAM mirror.

The strongest reuse opportunity is native NAudio processing. Readback avoidance
is the strongest graphics architectural lead, but requires game-specific
dependency evidence. Neither is enabled by this research change, and no new
hardware performance or gameplay claim is made.
