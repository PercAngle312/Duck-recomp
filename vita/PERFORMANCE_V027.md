# v027 display-list CPU candidate

This build targets list preparation, on top of v026. The renderer still executes
every command in order, including texture loads, segment changes, microcode
changes and framebuffer effects. Real Vita FPS gains are unmeasured.

## Exact geometry conversion reuse

Matrices, vertices, viewports and lights use bounded, direct-mapped caches of
source bytes and converted results. Every lookup compares the complete source
span; no uninstrumented guest dirty flags or probabilistic hashes are trusted.
Vertex and viewport snapshots include the enclosing physical words, because
halfword-aligned input can read word-swapped bytes outside its nominal span.
Matrix keys include the current floating-point rounding mode.

Hits within a task reuse its already-converted arena allocation. Across tasks,
unchanged results are copied into the current task arena. Cache collisions and
source mutations never overwrite data referenced by an earlier command. Serial
wrap invalidates every cached task pointer. All existing bounds checks run first.
The cache consumes approximately 400 KiB of fixed storage, independent of run
length; oversized inputs retain the original conversion path.

98,304 production-method comparisons against v026 passed with ASan/UBSan. Tests
cover changing input, segmented addresses, count/size changes, direct-map
collisions, pointer lifetimes, frame reset/wrap, halfword byte lanes, invalid
bounds and all four rounding modes. In this controlled fixture, scalar field
reads fell from 5,097,672 to 1,612,628 and allocations from 98,304 to 20,998.
Those numbers describe the test workload, not real gameplay or a predicted FPS.
Graphics timing logs now include conversion_hits, conversion_misses and
task_conversion_reuses so the hardware session can show actual cache use.

## Native command-end scanning

The preliminary display-list length pass checks four commands at once using
NEON, retaining the original scalar tail and first END/no-push-branch result.
Both GBI families keep their existing opcodes. The 65,536-command cap and RDRAM
bounds remain unchanged. Scanning four commands may read subsequent words in
valid RDRAM, but never executes them or reads outside the bounded command span.
Translation itself still handles every command and mutable graphics state.

6,168 Cortex-A9 fixture cases compare native scanning against the original scalar
termination rules across opcodes, alignment, tails and push/no-push branches.
Additional checks cover the 65,536-command limit and a terminator immediately
outside the requested span. No GPU or complete game emulation is claimed.

## Rebuild and compare

`experimental/graphics_replay/prepare_renderer.py` applies both new patches.
Recompile all five renderer units because the bridge header changes. Link with
the v026 native audio archive and the matching v025 framebuffer-fence vitaGL
library. Runtime/audio behavior and settings remain unchanged.

Run `test_conversion_cache.py --source <prepared bridge cpp> --reference
<v026 prepared bridge cpp> --report <output json>`. Build tests/command_scan.cpp
with the VitaSDK, NEON, freestanding flags, libgcc and entry
`duck_test_command_scan`; execute via tests/run_command_scan_arm.py (Unicorn
2.1.4), following the existing native arithmetic fixture build pattern.

Install over DUCKVIT04. Compare exactly the same scene with v026 at Native,
AA Off, Live Stats On. Note FPS, LIST/DRAW/READBACK, DRAWS/TRIS, audio queue and
underruns; retain the game.log. Check animated characters, lighting, camera and
framebuffer effects. If the CPU gap dominates, further guest-routine work is
needed; this change cannot establish which stage limits the real Vita.
