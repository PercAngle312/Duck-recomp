# Native settings menu and edge antialiasing

Hold Select for 0.6 seconds to open the native menu during the full game.
D-pad up/down selects a row, left/right edits it, Cross chooses Apply and Save
or Cancel, and Circle/Select cancels. Release the closing button before the
guest resumes so applying settings does not also trigger a gameplay action.
The existing Select+Start exit gesture remains available.

The menu exposes internal resolution, edge AA, texture filtering, aspect ratio
and final scaling. The existing bounded internal sizes are preserved:

| Profile | Widescreen | Original aspect |
| --- | --- | --- |
| Native | 424x240 | 320x240 |
| 2X | 848x480 | 640x480 |
| Display | 960x544 | 725x544 |

The menu pauses guest dispatch. A clock wrapper freezes the time source used
by guest VI/timers/Count, excluding menu time on resume. The native playback
worker can drain its already-owned audio; AI completion service remains on the
host thread. This does not generate new guest synthesis while paused.

Settings commit to `ux0:data/DuckRecomp/graphics.ini` via a temporary file and
backup. Failed writes leave pending changes unapplied. Startup also checks the
backup if the main configuration is missing. Applying occurs outside an SP
task, changes owned render-target dimensions on the next frame, invalidates
texture imports and switches the active filter. ROM and EEPROM paths are
untouched. Defaults retain native resolution with AA off; higher resolution
and AA add GPU work and are choices, not asserted performance improvements.

## Antialiasing path

`aa=off|edge` selects a separate shader that smooths luminance edges in the
rendered game image. It is a directional, edge-aware presentation filter with
neighbourhood bounds, not a new texture pack or a claim of hardware MSAA.
Source sampling clamps at the image borders and uses explicit texel size, not
`textureSize`. The source retains the game FBO's vertical orientation. AA
forces an offscreen source even when internal resolution matches the display.

Pinned vitaGL's default-surface MSAA does not by itself enable multisampling
in this backend's texture-backed game FBO. No ineffective 2X/4X MSAA toggle is
exposed. Edge AA applies to final presentation after the guest framebuffer RAM
mirror, keeping game-side framebuffer effects on the original readback path.

Presentation restores its touched program, framebuffer, array buffer, vertex
attributes, viewport, scissor, texture unit/binding, colour mask/clear colour,
blend, depth-test and cull states. Menu text uses an owned small bitmap font
drawn with scissored colour clears; no third-party font dependency is needed.
Shader compile/link failure is logged and falls back to ordinary presentation.

## Reproducing the private coordinator

`python3 vita/enable_settings_entry.py --input <private-v17-entry.cpp> --output
<new-private-entry.cpp>` adds the settings poll and paused clock to a new copy
of the authored application coordinator. Add the graphics-replay directory to
its include paths for `paused_clock.h`. It refuses an already-integrated or
instrumented coordinator and never overwrites the input. Translated game
functions, catalog objects and assets stay private.

## Evidence and limits

Host ASan/UBSan tests cover the hold gesture, exit chord, setting edits,
cancel/apply behavior, configuration round trips/replacement and clock pause
continuity. The production renderer and private coordinator compile with the
pinned ARM SDK and link into the full game. Physical menu operation, shader
compilation, orientation, live resolution changes and image quality remain
hardware-unverified until the candidate is exercised.

Audio is still reported broken in v00.17. This candidate retains that audio
path. The prior 192 synthetic tasks matched both SIMD and scalar DSP, which
does not establish correct real-game audio or scheduling. A replacement native
NAudio/synthesis path is not enabled without broader real-task and persistent
state validation. This menu release makes no audio-fix claim.
