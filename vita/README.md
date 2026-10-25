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
   libultra bindings, timers, blocking message queues and overlay dispatch.
   Cartridge reads and PI command/completion queues now have a working batch,
   but the guest CPU loop must still schedule that service and wake waiting threads.
   The Linux JIT/mod infrastructure and 4 GiB address reservation cannot simply
   be enabled in this 32-bit host.
   The semaphore blocker now has a VitaSDK adapter in `include/vita_semaphore.hpp`.
   `prepare_runtime.py` adds it to copied concurrentqueue headers without changing
   upstream. Its queue compiles and a standalone probe links for ARM against the real SDK; a host API model
   passed count/poll/timeout/long-timeout/wakeup checks and 20,000 unique queue
   deliveries from four producers. This does not establish Vita OS behavior.
   The runtime still needs a Vita window interface, scheduling and memory work;
   the adapter alone does not make the runtime compile or link.
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

## Resume after the semaphore checkpoint

Run the reproducible test and runtime compiler audit (no ROM required):

```sh
python3 vita/test_runtime.py \
  --vitasdk /path/to/vitasdk --runtime /deps/N64ModernRuntime \
  --recompiler /deps/N64Recomp --adapter-include /private/vita-build/include \
  --output /private/vita-runtime-validation
```

This uses a host C++ compiler, then the pinned Vita compiler. The host fixture
models only the semaphore API and is explicitly excluded from native compilation.
The native queue object imports the five real Vita semaphore APIs; a standalone
ARM ELF then links against the SDK. Neither binary has been run on Vita OS. The report
records outstanding runtime syntax failures rather than treating them as a
complete game build. All tests use assertions enabled by the supplied commands.

Next: supply a real Vita renderer/display interface where the desktop runtime
requires `WindowHandle`, then implement scheduling and bounded guest memory.
Keep RT64 desktop assumptions out of this backend. The current diagnostic VPK
remains the earlier CPU bring-up binary; this checkpoint adds source and tests,
not a new game/VPK release. Linux Phase 19 and graphics branches remain separate.

## Cartridge I/O and complete-code link checkpoint (2026-10-25)

`src/os_io.c` supplies actual guest ABI callbacks for ROM PIO, PI-manager
command queue setup, queued cartridge-read DMA and hardware-event registration.
PI service runs at an explicit host CPU-loop poll, handles high-priority jobs
first, copies big-endian ROM bytes into word-swapped RDRAM, and sends the actual
OSIoMesg pointer to the completion queue. Invalid jobs do not report success.
ROM writes are rejected; no ROM or user saves are modified. An event is sent
only when the host explicitly raises it. This is not timed hardware emulation.

`src/os_core.c` restores reproducible source for early Status/translation/queue
initialization callbacks that previously survived only in the private object.
`src/os_queue.c` preserves the earlier guest ring-queue implementation, adds
canonical-address/null-context checks, and remains **nonblocking only**. A full
or empty OS_MESG_BLOCK call still returns failure rather than suspending a
thread. These components are included in the `duck_vita_os_io` CMake archive.

The separate PI register model now uses WR_LEN at +0x0C for cartridge-to-RDRAM
DMA. RD_LEN at +0x08 is the unsupported RAM-to-cartridge direction. Its own bus
memory representation is big endian and must not be confused with this
callback layer's word-swapped translated RDRAM.

| Audit scope | Unresolved result |
| --- | --- |
| Supplied resident soft-float object | 30 before, 26 after four callbacks; no new imports |
| Resident rebuilt from source with VitaSDK hard-float | 27, including the SDK assertion callback |
| Same resident with real SDK libm/libgcc | 21 (19 game services/diagnostics plus assertion and errno support) |
| All 8,956 module functions, resident and audio RSP, with SDK math | 29 strong, 4 weak references |

The complete-code audit adds real loader/RSP, libc and exception-table
requirements, so its count is not comparable to the narrower 26-symbol result.
It is a relocatable object, **not a final Vita executable**. The three missing
resident functions are regenerated in an isolated private output. Existing
native binding replacements are retained. No working Linux source is edited.

Host ASan/UBSan I/O and core tests pass. A hard-float ELF with no unresolved
symbols executes the four existing private binding wrappers and their callbacks
in Unicorn: PIO boundary cases, priority DMA, word-swapped bytes, job-pointer
completion messages and event delivery pass across 16 ARM calls. This tests
CPU code, not Vita OS, scheduling, rendering or gameplay. The earlier actual
translated startup (three RAM patterns), 28 module cases and 1,072 multiply
cases also still pass in ARM emulation.

Reproduce using the private I/O checkpoint's source inputs and reports:

```sh
python3 vita/test_os_io.py --vitasdk /deps/vitasdk \
  --recomp-include /checkpoint/headers \
  --base-object /checkpoint/inputs/base-30.arm.o \
  --binding-source /checkpoint/inputs/callbacks/runtime_bindings_arm.c \
  --output /private/io-test
python3 vita/test_arm_os_io.py --elf /private/io-test/io-cpu-test.elf \
  --bindings /private/io-test/private-bindings.json --output /private/arm-results.json
python3 vita/rebuild_private_runtime.py --vitasdk /deps/vitasdk \
  --recomp-include /checkpoint/headers \
  --resident-source /checkpoint/inputs/resident \
  --private-include /checkpoint/inputs/includes \
  --callbacks /checkpoint/inputs/callbacks/sources.json \
  --modules /checkpoint/inputs/libduck_vita_modules.a \
  --audio-rsp /checkpoint/inputs/libduck_vita_audio_rsp.a \
  --output /private/native-link --jobs 4
```

Use Unicorn 2.1.4 and pyelftools 0.33 for CPU tests. The SDK archive remains
pinned in toolchain-lock.json and was re-downloaded and SHA-256 verified.
The behavior reference was n64decomp/libreultra at
`1aca5c13ca041cef86f8dc194b727361dad9c09b` (pidma.c, pirawdma.c, devmgr.c and
seteventmesg.c); this port implements those contracts without copying game code.

Next integration: stack-preserving guest thread suspension/resumption, blocking
queues, timer/event delivery on the game CPU loop, overlay loading and real SP
and graphics tasks. The current VPK is still the earlier CPU diagnostic. No new
playable VPK or hardware result is claimed by this checkpoint.

## Stack-preserving guest threads (2026-10-25)

`src/os_threads.c` now runs guest threads on separate native stacks. The Vita
backend uses SceFiber and loads its system module; the host test backend uses
ucontext. Guest create/start/destroy/priority/yield callbacks and pause_self are
implemented. The private yield binding forwards to the actual yield callback
instead of the old abort-only binding. Missing entrypoints and invalid lifecycle
operations set a runtime fault; they do not report success.

Queue hooks now suspend a blocked send/receive inside its current native call
stack, wake the highest-priority eligible waiter, recheck the queue condition
and preempt at kernel boundaries. Explicit equal-priority yields use FIFO order.
The PI helper's callback context is now stack-local, so a preempted queue call
cannot have that context overwritten by a later DMA/event operation.

Initialize with `duck_vita_threads_init(rdram, resolver, service_poll)`, seed an
entry with `duck_vita_threads_boot`, then call `duck_vita_threads_run` with a
bounded dispatch budget. Supply the real PI poll as service_poll to process
queued DMA before selecting runnable threads. Waiting for events is distinct
from idle and failure. This loop still requires the actual game resolver,
initialization, timer/VI events, overlay loader and graphics/SP tasks.
Busy guest loops are not preempted: this is cooperative kernel-boundary scheduling.
The original generic scheduler/context helpers remain separate and unchanged.

Host UBSan tests pass blocking recv/send with preserved local arrays, priority
wake/preemption, self-destruction, dispatch budgets, external queue wakeups and
floating-point rounding modes across equal-priority yields. Existing I/O ARM
checks and ABI/dispatch regressions pass. The native test ELF/SELF/VPK builds
against the real Vita fiber libraries, with 30 assertion call sites retained.
Physical Vita/firmware execution has **not** been tested. The later Vita3K
result below validates the actual VPK with HLE fibers. Thread stack size is initially
128 KiB each, limited to 16 concurrent guest contexts; gameplay usage is unmeasured.

The private whole-code partial link resolves six prior game symbols. It now
reports 31 strong imports (15 remaining game-side services/state plus 16 SDK or
linker imports) and 4 weak references. Native fibers introduce real SDK imports,
so total import counts across these scopes are not progress percentages.
Use `rebuild_private_runtime.py --threads` and the private callback manifest
containing the yield binding to reproduce this audit.

Build the separately labeled hardware test:

```sh
cmake --build /private/vita-build --target DuckDodgers-Vita-Thread-Test.vpk
```

Install it in VitaShell and open **Duck Vita Thread Test**. It reads no ROM/save
and uses a different title ID from the earlier CPU diagnostic. It shows PASS or
an assertion failure and Triangle exits. A PASS is a runtime prerequisite,
not a gameplay result. No new playable VPK is claimed.

## Actual thread-test VPK execution in Vita3K (2026-10-25)

The unchanged `DuckDodgers-Vita-Thread-Test.vpk` boots and displays
**Native fiber / guest thread tests: PASS (0)** in Vita3K v0.2.1
4232-92bda548. Its SHA-256 and precise conditions are recorded in
`reports/vita3k-thread-test.json`. This runs the packaged ARM SELF through the
emulator, rather than substituting the host ucontext test for Vita APIs.

No Sony firmware was installed. Automatic module selection first failed because
it requested absent libfiber.suprx. Manual modules with an empty LLE list use
Vita3K's implemented HLE fibers; the same VPK then passes. Host gamepad, rumble,
sensor and camera discovery was disabled in a private emulator copy because this
sandbox cannot enumerate those devices. Only that host SDL initialization flag
was changed; guest CPU execution, Vita fiber HLE and the VPK/assertions were
unchanged. Audio/video initialization still ran, using virtual X11, llvmpipe
OpenGL and dummy host audio. It remained on PASS until emulator shutdown.

The original host UBSan thread test and 16 ARM I/O calls also pass on retest.
This does not validate real Vita firmware fibers, a physical gamepad, Triangle
exit, audio hardware, game graphics or gameplay. The unchanged VPK is still a
runtime diagnostic; no playable game build is established by this result.

## Monotonic timers, VI clock and translated boot (2026-10-25)

The native time service initializes guest FPU/clock services, implements
64-bit o32 timer arguments, one-shot/periodic deadlines, cancellation and
nonblocking expiry messages. Clock updates advance CP0 Count and Compare across
wraps; writing Compare equal to Count waits for a full wrap rather than raising
a match on the next tick. Logical time starts at initialization while Count
continues from the bound clock epoch. Timer processing is bounded per poll and
updates event state before queue delivery, which can switch fibers.

The VI manager starts a configured HLE retrace clock, promotes pending frame
buffers and delivers osViSetEvent messages at the requested retrace interval.
Re-registering or disabling that event updates its countdown. This does not
decode OSViMode timings or implement complete MI/CPU interrupt dispatch/RDP.
The native clock converts Vita process-time microseconds to N64 Count ticks.

Host UBSan tests cover deadlines, 64-bit ABI, periodic/cancel/drop behavior,
Compare wrap, VI interval/reset/disable, bounded catch-up, clock reversal and a
timer message waking an actual blocked guest fiber. A timer test ELF links
against the real VitaSDK fiber/system imports. Add `--time` alongside `--threads`
to the private runtime rebuild; it replaces the historical Count and VI state
sources with current implementations. All 8,956 modules/resident/audio RSP still
partially link: 29 strong imports and 4 weak references in this scope.

An additional private host execution probe runs actual translated resident
startup with the supported private ROM, these kernel services and stackful
threads. It reaches the four game thread entrypoints after startup and then
halts explicitly at the unimplemented duck_before_free overlay hook (exit 86).
Unsupported services in that diagnostic terminate; they are not success stubs
and are not included in the production partial link. This is host translated
boot progress, not Vita gameplay. See reports/time-checkpoint.json.
