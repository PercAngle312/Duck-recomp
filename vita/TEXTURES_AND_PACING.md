# Vita 00.12: texture state and frame pacing

The hardware reference is 00.11. The user reports improved FPS and working
controls, with remaining texture corruption and choppy gameplay. Keep that
build available for comparison. This update retains its game core, native
controller/runtime object, performance clocks and validated shader-cache format.

## Texture correctness

* Store SetTile S/T masks, and disable wrap/mirror when the mask is zero.
* Use the same effective tile for a sampling slot's image, dimensions, shifts
  and UV origins. Retain the existing base-mip fallback consistently.
* Copy TLUT ranges across the two palette halves and preserve untouched entries.
  Mark texture bindings dirty after a palette load. Changed CI4 banks receive
  content revisions; unchanged reloads retain their revision. CI8 and raw
  paletted resources include the relevant palette revision in cache identity.
* Fix direct 4-bit LoadBlock byte sizing, which previously used an unsigned -1
  as a shift count. Direct 4-bit tile loads retain source nibble origins and
  width; source bounds include a leading half-byte when needed.
* Distinguish a real source pitch equal to a sub-tile's total byte count from
  the LoadBlock width=1 sentinel. The sentinel requires both loaded pitches
  equal to the total. This avoids shifting cropped rows in that case.
* Include masks, nibble origin and logical width in decoded texture identity.

These address source-level errors. They do not establish that every reported
hardware artifact is fixed. Full TMEM/LOD behavior and arbitrary microcodes
remain outside the implemented renderer's demonstrated coverage.

## Avoidable stalls

Staged textures use exact-key indexed lookup instead of repeatedly scanning the
growing list. The key includes source, byte count, row pitch and swizzle group;
values are array indices, so allocation growth does not leave dangling pointers.
Existing live-memory hashing and mutation checks remain active.

A palette-source mutation no longer clears unrelated GPU textures. Exact source
invalidation and per-bank revisions keep converted pixels current while letting
unaffected uploads remain cached. Paletted raw resources use palette revisions
too. This change relies on the palette-state corrections above.

Audio microcode validation compares its exact current byte sequence on every
call. A changed sequence must pass the expected CRC before native execution.
Repeated identical tasks reuse the validation result, avoiding the previous
bit-by-bit CRC on every task. A mutation test verifies rejection and revalidation.

The private audio RSP build enables the pinned SIMD implementation on ARM32
through SSE2NEON, using O2, signed wrapping and strict aliasing disabled. No
fast-math is added. The Vita allocator bridge supplies the pinned header's
supported allocation hooks. `rebuild_audio_rsp.py` copies ABI headers into its
private output, preserving the original generated source and recording hashes.
The original scalar archive remains available in the prior checkpoint.

## Validation

ARM32 code in Vita3K compared scalar and NEON results for 4,096 deterministic
vector cases, all 16 element selectors, aliasing/boundary inputs, accumulators,
flags and DMEM. Fifteen sampled real audio tasks also matched the original
archive across complete 8 MiB RDRAM and 4 KiB DMEM snapshots. These are emulator
checks of ARM code; they are not physical Vita measurements or exhaustive proof
for every possible instruction stream.

Host ASan/UBSan fixtures cover palette banks/ranges/revisions, cropped rows,
4-bit sizing, nibble origins, tile-slot selection, cache-key fields, stage-index
growth and exact live microcode identity. The pinned VitaSDK compiles the five
prepared renderer units and links the full private game with no strong imports.
CI includes the texture/state checks, microcode cache and ARM32 NEON bridge.

See reports/textures-pacing.json for exact final-build execution evidence.
The final VPK excludes comparison functions, scripted input and frame capture.
Instrumented comparisons are recorded separately from release execution.

Offscreen captures were black for both the hardware-working 00.11 baseline and
candidate in this host environment, making visual comparison inconclusive.
Software-rendered GPU stalls and recorded dummy/disk audio cannot establish
native FPS or audible quality. The final textures, sound, pacing and prolonged
playability still need physical comparison. Graphics and translated RSP tasks
remain synchronous; audio starvation can still occur during expensive guest
work. No ROM, private generated source/object, save or Sony module is committed.
