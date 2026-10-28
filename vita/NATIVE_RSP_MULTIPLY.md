# Native ARM32 RSP fractional multiply

The private audio candidate replaces the SSE-to-NEON fractional multiply paths
with direct ARM NEON operations. This retains translated task execution,
command dispatch, DMA, and guest completion ordering. It does not replace the
audio ABI with generic HLE commands.

`duck_vita_rsp_multiply.h` handles VMULF/VMULU and VMACF/VMACU, preserving all
three 16-bit slices of each 48-bit accumulator lane. Only the destination
saturates. Input vectors are loaded before output writes to preserve register
aliasing; the existing RSP implementation performs element selection first.
VMULF rounding avoids signed 32-bit overflow on `-32768 * -32768`.

Enable only for a private NEON candidate with:

```
python3 vita/rebuild_audio_rsp.py [existing private build arguments] --fast-memory --native-multiply
```

The build patches private copies of the pinned runtime headers. Generated game
source and original input headers are untouched. The feature remains opt-in;
the user v00.13 release is preserved.

Validation:

- 98,304 ARM32 arithmetic cases executed in Unicorn Cortex-A9. The independent
  scalar oracle compares the complete accumulator and destination, including
  signed/unsigned saturation, overflow, edge operands, random state, and
  destination/source aliases.
- 128 complete synthetic translated audio tasks matched the isolated v00.13
  implementation across all 8 MiB RDRAM and 4 KiB DMEM.
- The full private game ELF linked with no unresolved strong imports.
- The pinned VitaSDK workflow now compiles and executes the public ARM fixture.

The private candidate additionally uses the existing runtime shadow comparison
to check sampled real game audio tasks against the v00.13 DSP. See
`reports/native-rsp-multiply.json` for that run's actual results. Emulator
timings are not Vita performance measurements. This does not resolve full
framebuffer readback stalls or establish stable gameplay/audio on hardware.
