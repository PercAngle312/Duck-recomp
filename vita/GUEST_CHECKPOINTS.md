# Cooperative scheduling inside translated game code

The production game rebuild now instruments private generated CPU code before
compilation. Every generated function entry and every direct backward-goto
target calls `duck_vita_thread_checkpoint`. Original source is copied, not
edited; generated output remains private. Both resident and overlay modules
must be rebuilt. Linking the runtime alone does not activate this feature.

After 1,024 checkpoint visits, an eligible running guest fiber returns to the
host scheduler. Host services then advance PI DMA, controller snapshots, AI
output, timers and VI before selecting the next guest thread. Service callbacks
never run on the suspended guest stack. Native locals and the guest context
survive the yield. Higher-priority ready threads can run, while the suspended
thread keeps its place among equal-priority threads. Explicit osYieldThread
continues to rotate equal-priority threads as before.

Checkpoints skip IRQ-disabled, EXL/ERL and fully CPU-masked regions. This is
cooperative scheduling at generated-code boundaries, not cycle-accurate N64
exception delivery. Per-device interrupt mask/deferred-event fidelity is still
incomplete. Native renderer calls and translated audio RSP tasks remain synchronous and
are not interrupted by CPU-code checkpoints. PCM playback uses its own native
audio worker and is polled from the host service loop. Disabled-interrupt busy loops and
uninstrumented native code can still hold the host. Performance and gameplay
need hardware validation; entry/loop calls add overhead.

`duck_vita_threads_checkpoint_yields()` exposes actual yields, reset on runtime
initialization. The private v00.03 launcher includes `loop_yields` in its periodic
status log. A positive counter establishes scheduler activity, not a rendered
frame or playable game.

Private rebuild order:

1. Generate the checked overlay catalog and allocation observer.
2. Run `instrument_checkpoints.py` on copied resident and module sources.
3. Compile all copies and callbacks with the pinned ABI/toolchain.
4. Link the game-only object against the current authoritative runtime once.

The instrumenter validates source signatures and local goto targets, ignores
comments/strings, and rejects duplicate instrumentation. Focused execution
checks cover a busy loop with no OS calls, priority wakeup from a host event,
critical-region guards, equal-priority ordering, saved locals/FPU state and
propagation of host-service errors.
