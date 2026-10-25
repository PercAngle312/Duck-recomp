# Duck Dodgers native Vita port experiment

This is an ARM32 bring-up branch, **not a playable Vita game**. It continues the
existing N64Recomp project and preserves the Linux Phase 19 reference.

## Concrete result

- Cross-compiled all 8,956 generated module functions and the generated resident
  code into Vita ARM static libraries. The translated audio RSP also compiles,
  using its portable scalar implementation. These are libraries, not a linked
  full-game executable.
- Built `DuckDodgers-Vita-Bringup.vpk`, a native diagnostic that checks guest
  cached/uncached memory aliases, portable 128-bit multiplication, the actual
  translated startup, and 28 executions of a translated module leaf function.
  It intentionally stops at the documented startup handoff; it does not replace
  the missing libultra runtime with fake success stubs.
- Executed the actual Vita ARM ELF's translated startup against three complete
  8 MiB RAM patterns, 28 relocated module cases and 1,072 multiply cases in
  Unicorn. Another 20,162 signed/unsigned multiply comparisons and memory-lane
  checks passed on the host.
- **No Vita hardware, Vita OS, game renderer, audio output or playable campaign
  was tested.** CPU emulation is not a substitute for that validation.

## Install the diagnostic

Install the VPK with VitaShell and open **Duck Recomp Vita Lab**. It displays
PASS/FAIL results; Triangle exits. It needs no ROM and makes no changes to game
saves or game assets. Its generic LiveArea/debug-screen resources come from the
pinned official VitaSDK samples. A hardware screenshot is useful at this stage.

The VPK contains small translated test functions. Generated proprietary game
source, compiled game archives and binaries stay outside this public repository.

## ARM32 changes

`prepare_headers.py` adapts copied headers in the build directory, without
changing pinned upstream files. It supplies an exact four-limb 64x64 -> 128-bit
multiply and maps KSEG0/KSEG1 into one bounded guest-RAM allocation, preserving
word-swapped byte/halfword lanes. Guest MMIO still requires a real runtime;
masking addresses alone does not implement PI, SP, VI or device registers.
The Linux runtime's 4 GiB virtual mapping is not carried over to Vita.

## Reproduce

Use the private Phase 19 generated source checkpoint, the pinned N64Recomp and
N64ModernRuntime checkouts, the official VitaSDK archive in toolchain-lock.json,
and VitaSDK samples at its pinned commit. The SDK archive's SHA-256 was verified.
The tested compiler is Vita GCC 15.2.0, Cortex-A9 / NEON / hard-float ABI.

```sh
python3 vita/build.py \
  --vitasdk /path/to/vitasdk \
  --game-source /private/duck-dodgers-recomp \
  --recompiler /deps/N64Recomp --runtime /deps/N64ModernRuntime \
  --samples /deps/vita-samples --build /private/vita-build --jobs 4
python3 vita/test_adapters.py --adapter-include /private/vita-build/include
python3 -m pip install unicorn==2.1.4 pyelftools==0.33
python3 vita/test_arm_execution.py /private/vita-build/duck_vita_probe
```

Normal CMake/Ninja/Python host tools are required for rebuilding. Running the
supplied VPK on Vita does not require a PC toolchain. No ROM is needed to build
the already generated source. Regeneration remains a private operation.

## Remaining port work

1. Replace/adapt the platform runtime: N64 CPU-thread scheduling, native
   libultra bindings, timers, message queues, PI/ROM DMA and overlay dispatch.
   The Linux JIT/mod infrastructure and 4 GiB address reservation cannot simply
   be enabled in this 32-bit host.
   A direct Vita compiler audit of the unmodified runtime confirms the first
   blocker: concurrentqueue's semaphore wrapper reports `Unsupported platform`
   for threads, events, renderer context and the main runtime. The RSP source
   itself passes that syntax audit. No semaphore shim has been implemented yet.
2. Implement Vita graphics task and VI presentation callbacks. RT64's current
   Vulkan backend is not a Vita backend. A researched reference is
   `Rinnegatamante/DaedalusX64-vitaGL` at the locked commit: its
   `Source/SysVita/HLEGraphics/GraphicsPluginVita.cpp` implements the graphics
   plugin `ProcessDList`, `UpdateScreen` and VI callbacks. No code from that
   renderer has been integrated yet, and the CPU remains the static recomp.
3. Connect the translated RSP to actual OSTasks and SceAudioOut, then validate
   PCM and timing. Scalar RSP compilation alone does not establish performance.
4. Add supported-ROM loading, Vita controls, private EEPROM save handling and
   full gameplay/mission/restart testing. Target the Vita's native display and
   measured memory budget; PC 4K/MSAA presets are separate Linux settings.

Linux graphics controls are preserved on `linux/graphics-settings`; that work
is not a new stable release. The user redirected the current effort to Vita.
