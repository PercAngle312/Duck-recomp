# First game graphics task replay — not a playable game

This native Vita application replays the **actual first graphics task** captured
from translated game startup. It uses Daedalus's Fast3D memory bridge,
interpreter and vitaGL backend. The captured task has F3DEX.NoN fifo 2.08
microcode, 57 top-level triangle-pair commands and six texture rectangles.
A successful link is not evidence that those commands rendered correctly.

The ELF and VPK build with the pinned VitaSDK. The ELF has **zero unresolved
strong symbols**. It has not been executed or visually verified. It does not
boot translated game threads, implement the four missing SP task services, or
make the game playable. The production ARM runtime audit remains at 28 strong
imports. This test isolates the GPU/backend work needed to continue startup.

## Build

Check out every repository at its exact commit in `dependencies.json`, beneath
one dependency directory, retaining the directory names used in that file.
Install the SDK pinned in `../../toolchain-lock.json`. With Python 3.12+, make,
curl and git, run:

```sh
python3 vita/experimental/graphics_replay/build.py \
  --vitasdk /path/to/vitasdk \
  --dependencies /path/to/dependencies \
  --output /path/outside/public-repository/replay-build
```

The script verifies source commits, checks the taiHEN SDK package checksum,
builds the graphics dependencies, compiles the actual backend, links the ELF,
checks strong imports, and packages the SELF/VPK. It uses the SDK sample LiveArea
assets from a pinned commit. No private game assets are included in the VPK.
The C shader helper uses GNU11 explicitly because GCC 15's newer default C
semantics reject the upstream legacy taiHEN callback casts.

These upstream renderer dependencies include GPL/LGPL code. Preserve their
licenses and corresponding source when redistributing this linked application;
repository URLs and exact revisions are in `dependencies.json`.

## Runtime inputs

The private checkpoint supplies these files, outside the public repository:

* `ux0:data/DuckRecomp/first-task-rdram.private.bin`: exactly 8 MiB, word-swapped
  guest RAM captured at the graphics submission boundary.
* `ux0:data/DuckRecomp/first-task-address.txt`: the hexadecimal guest task address.

Install `Duck-Vita-Graphics-Replay.vpk` (title ID `DUCKVIT03`) and copy those
private files to the stated directory. The Vita needs its own working
`libshacccg.suprx` shader-compiler setup, as required by vitaGL. No Sony system
modules are supplied. Press Start to exit. Diagnostics are written to
`ux0:data/DuckRecomp/graphics-replay.log`.

Visual inspection must establish whether actual task output appears correctly;
a submitted-task log line alone does not establish this. Report the log and the
visible output if hardware testing is performed. It is not gameplay validation.

## Limits

Only the captured F3DEX.NoN 2.08 microcode family is accepted. An unrecognized
microcode aborts rather than being interpreted as a supported one. The adapter
uses a 320x240 native image and a 4:3 viewport on the Vita display; it does not
implement guest VI mode changes or final framebuffer presentation timing.
Every guest memory page is marked dirty before translation until guest-store
tracking is integrated. It renders each replay task synchronously and remains
separate from the guest SP/DP scheduler. Audio, saves, game input and gameplay
are outside this graphics replay.

## v0.02: initialization return-value correction

The first hardware run returned to LiveArea after logging initialization error
`-2`. That was an application error: the pinned vitaGL `vglInitWithCustomSizes`
returns `res_fallback`, initialized to `GL_FALSE` and set only when the requested
resolution is reduced. `vglInitExtended` forwards that value. Normal native-size
initialization therefore returns zero. The first replay incorrectly interpreted
zero as a failure and exited before creating the Fast3D interpreter.

Version 00.02 removes that inverted check, prevents repeated initialization,
and logs the resolution fallback value, viewport, shader compiler messages,
translation, rendering and framebuffer swap stages. The user-reported exit
verifies that the original app ran up to this boundary; it does not verify
rendering. The updated build still requires a new hardware graphics check.
