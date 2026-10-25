# Translated startup through graphics submission

The private host diagnostic executes the existing translated resident and module
sources with the stack-preserving guest runtime. It currently loads 79 modules,
including the logo module, starts six guest threads, and stops deliberately at
`osSpTaskLoad_recomp` with task type 1 (graphics). This is host startup execution,
not rendered output, a linked Vita game, or verified gameplay.

`generate_overlay_catalog.py` reads the private callback checksum manifest,
resident sources, `recomp_overlays.inl` and `module_catalog.inc`. It cross-checks
module sections, ROM offsets, sizes and function offsets. Output contains game
identifiers and belongs outside this repository. Pass `--allocator-symbol` for
the verified game allocator and use the resulting isolated resident copy. The
original generated sources are never edited.

Loaded modules update `section_addresses` and exact native function dispatch.
Free retains mappings for cleanup callbacks, matching the existing Linux loader.
The observed allocator invalidates retained mappings before *any* guest reuse,
including data allocations. Active code and BSS overlaps are rejected. Module
catalogs require the allocation observer; silently retaining stale mappings is
not supported. The observer must cover the game's allocator paths before release.

The SI input bridge serializes asynchronous reads and queries, then delivers
`OS_EVENT_SI` through the existing event queue. Status records occupy four bytes,
pad records six, and byte writes preserve global word-swapped guest lanes even
for halfword-aligned arrays. Read results stay latched until the next completion.
Native polling uses VitaSDK controller APIs; the host boot diagnostic supplies
an explicit virtual neutral controller.

`osSetIntMask_recomp` preserves CPU status outside interrupt mask bits and stores
RCP mask state separately for every guest thread. This does not implement full
MI interrupts or asynchronous preemption of arbitrary translated loops.

Rebuild the private partial link with `rebuild_private_runtime.py --threads
--time --input --overlay-catalog <private_catalog.c>` plus the existing private
source/archive arguments. Its 28 strong imports include ordinary SDK/C library
imports and seven game runtime services (`dmem`, fault handlers and four SP task
exports). An unresolved-symbol count is not a gameplay milestone.

ASan/UBSan tests cover overlay lifetime and reuse, SI event delivery and input
layout. The fiber tests also check per-thread mask preservation. See
`reports/overlay-input-checkpoint.json` for exact validation limits. The host
probe's unsupported-service traps terminate immediately and are never included
in the production ARM partial link.
