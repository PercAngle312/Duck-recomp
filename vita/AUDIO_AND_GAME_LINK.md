# Audio, relocated startup and private game executable

`duck_vita_live_start_catalog` connects the fiber scheduler directly to checked
relocated overlay lookup. Static addresses cannot represent several unloaded
overlays at the same default address. Initialization rejects an active runtime
before resetting any mappings. The private caller supplies the full verified
catalog and allocator-observed resident generation; generated data stays private.

The AI service implements the two-slot DMA queue, frequency division, length and
status. `duck_vita_ai_vita_init` binds actual Vita audio output. Guest stereo
samples are decoded from word-swapped RDRAM and resampled to 48 kHz. An AI event
is emitted only after the native sink reports successful playback. Device
failures propagate to the guest runtime; no completion is invented.

Audio SP tasks use a separate callback from graphics. The private game launcher
binds the existing translated audio microcode through the pinned librecomp RSP
runtime and checks its identity. Successful audio execution sends SP only.
Graphics emits DP only when the renderer reports RDP FullSync. Unsupported
microcode and failed rendering/playback stop execution explicitly.

Production PCM output now uses a dedicated native Vita thread. The guest host
copies the starting DMA into an owned word-swapped snapshot and signals a
semaphore, then continues running. The worker alone resamples and calls native
audio output using alternating aligned PCM buffers, so the latest submitted
buffer is not overwritten. Both native channels are configured to unity gain.
The worker never reads live guest RDRAM or runs translated CPU/RSP code.
A nonblocking host poll consumes completion after the final native drain, then
emits one guest AI event and starts the next queued DMA. Kernel-thread messages
are not guest interrupts; the worker never invokes guest event delivery.

C11 acquire/release atomics publish jobs and completion. Shutdown detaches AI,
requests cancellation, wakes and joins the worker, then deletes its semaphore
and releases its port. Join failure retains resources instead of releasing a
port still reachable by a live worker. Initialization failures unwind resources.

The final native block is padded to 64 frames; interpolation is clamped at DMA
boundaries. Reported AI length now decreases in eight-byte-aligned steps after previously
submitted native blocks have played; it excludes the second queued DMA. It is
block-granular, not sample-accurate. Starting the
next DMA and delivering its completion still depend on host polling; a long
native renderer call can delay those steps. Translated audio RSP tasks remain
synchronous SP work. Sound, latency, underruns and native-thread scheduling need
hardware validation; this is not a cycle-accurate AI implementation. The private
application initializes its save backend and audio before guest threads.

The private build links the real game entry, all translated modules, catalog,
current runtime, Fast3D/vitaGL, translated audio RSP and SDK dependencies. ELF
relocations are retained for `vita-elf-create`. A linked ELF and packaged VPK do
not establish successful startup, rendering, sound, saves or playable gameplay.

`guest_fault.c` terminates on translated BREAK or invalid jump-table dispatch
with a useful log. These handlers never skip a fault or return fake game values.

Host sanitizer checks cover AI queue bounds, completion/failure handling,
stereo byte lanes, resampling, and live relocated lookup. Separate SP tests
check graphics without FullSync and audio completion without a DP interrupt.
The native build checks real SDK APIs and final linkage. Proprietary ROMs,
translated source/objects, private launchers and game VPKs stay out of Git.

Native audio shutdown detaches the sink and discards pending guest DMA without
emitting completion interrupts. Later service cannot use a released native port.
Reopening audio creates a fresh queue and resampler. Host SDK-boundary tests
exercise queued shutdown as well as configuration/output/drain failures.

Native controls map Cross=A, Circle=B, L trigger=Z, R trigger=R, Select=N64 L,
Start=N64 Start and D-pad directly. Triangle also sends C-up (camera look toggle)
and Square sends C-down (camera behind Daffy). The left stick remains movement.
The original manual confirms that camera look uses the N64 Control Stick for
view movement until C-up is pressed again. Right-stick directions still send
C buttons, but vertical engagement now requires <48/>208, releasing at 80/176;
horizontal engagement remains <80/>176, releasing at 96/160. This reduces
accidental look-mode toggles during diagonal camera rotation. First C-up edges
are logged with their source. Left-stick deadzone/range are unchanged.

The native audio host test executes the production worker on a real pthread
behind mocked Vita kernel/audio calls. It blocks playback deliberately while
the main thread keeps polling, mutates live RDRAM to verify snapshot ownership,
and checks two-DMA event order, cancellation, startup/device errors and join
failure. ASan/UBSan checks are not physical Vita audio verification.


Version 00.09 avoids repeatedly changing the native port configuration for
unchanged block sizes. Before a length change it drains any queued block.
The optional asynchronous sink remaining_bytes callback reports only active-DMA
bytes, aligned to eight, after preceding output blocks have finished; invalid
callback values fault the AI service. A completion IRQ still waits for successful
final drain. Host checks freeze a later device output to verify live partial
progress, FIFO-full status, no early IRQ and correct configuration ordering.

This does not implement a continuous cross-DMA mixer. Final block padding,
per-DMA drains, output gaps while translated RSP or graphics work runs, and
clamped interpolation at DMA boundaries remain. Physical sound quality,
latency and audio/video synchronization require gameplay validation.
