# Color and performance update — v00.08

The user has reported working movement and audible sound on Vita. Uploaded
videos show real game cutscenes and a level/HUD. The user reports incorrect
textures/materials (Daffy appears red) and unacceptable frame rate. Doubling of
text in the recording is attributed by the user to the screen protector.
Stable gameplay, saves and correct rendering are still not established.

## Color changes

The pinned interpreter omitted G_MW_LIGHTCOL in both MOVEWORD handlers. This
command is used by gSPLightColor to update material lighting without uploading
an entire light. It now decodes logical 0xRRGGBBxx words into diffuse/copy-color
fields, using F3DEX2's 24-byte slot stride or F3D's 32-byte stride. Directions
and unrelated slots are preserved; ambient slots are included. The game sent
these commands very frequently in the initial emulator run. This is a plausible
cause of the material colors, but correct Daffy colors need hardware verification.

Light loads mark direction coefficients dirty. G_MWO_POINT_RGBA vertex-color
updates are handled instead of requiring every MODIFYVTX to be an ST update.
Other unsupported modes retain explicit checks.

Pinned vitaGL GPU readback uses ABGR1555 storage for both GL packed-5551
variants. Readback now uses byte RGBA and explicit N64 RGBA5551 packing,
tested with black/red/green/blue/white/transparent pixels. This also avoids
the emulator's unsupported packed-format transfer. It fixes a framebuffer
conversion bug, not necessarily every model texture issue. Storage is reused.

## Performance changes

The scene renders at native 320x240 into an RGBA game FBO, then explicitly
blits into the same 725x544 4:3 viewport on the 960x544 Vita display. Native
rendering shades about 80.5% fewer pixels. This is not a promised FPS gain:
CPU work, texture hashing, synchronization and audio RSP costs remain.

At native size, FullSync readback reads the existing game FBO directly,
skipping a redundant equal-sized intermediate blit. Readback remains enabled.
Offscreen color textures use RGBA8. Presentation remains per task with fixed
VI dimensions; full VI-driven presentation is unfinished.

All five renderer objects use -O2 and -fno-strict-aliasing, without added
fast-math. Generated game code and runtime scheduling are unchanged. The
v00.07 artifact remains available for comparison. DUCK_NATIVE_RESOLUTION=0
at adapter compile time retains the old higher internal resolution; the
distributed v00.08 defaults to native resolution.

Every five seconds, game.log reports average task costs in microseconds for
translation, draw, readback and presentation, plus light/vertex update counts.
These are not a gameplay FPS counter. Emulator timings are not native Vita
measurements and must not be used to claim hardware performance improvement.

## Validation

Shared color-helper ASan/UBSan tests cover red-to-black update, ambient and
last slots, both strides, copy fields, invalid offsets, preserved directions
and RGBA5551 packing. The pinned SDK compiles all prepared renderer objects,
and the private game ELF links with no unresolved strong symbols. Actual
execution results are in reports/color-performance.json. Hardware color and
FPS improvement still need comparison with the existing build.

The versioned shader cache remains compatible; do not rename or delete it.
ROM, compiler and save files are not replaced. Unknown microcodes, texture
edge cases and long-session stability remain open.
