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

This first audio sink is synchronous and blocks the guest host during output.
It pads the final native block to 64 frames and clamps interpolation at DMA
boundaries. Timing, underruns and performance need hardware testing; it is not
a cycle-accurate AI implementation. The private application must initialize its
save backend and native audio before starting translated game threads.

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
Start=N64 Start, D-pad directly and right stick to C buttons. A 12-unit left-stick
center deadzone suppresses small drift; each half is rescaled to reach exactly
-80/+80, with inverted Y. Sampling-mode and polling errors disconnect the current
snapshot. Host checks cover every mapped button, axes, C directions and failures;
physical feel and live gameplay input still need Vita testing.
