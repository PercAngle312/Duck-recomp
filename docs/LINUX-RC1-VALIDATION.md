# Linux release candidate validation

The full game was rebuilt from private generated C and pinned N64Recomp,
N64ModernRuntime and RT64 sources. The recovered Phase 18 executable was kept
unchanged as a reference, not relabeled as a new compilation.

| Build | Executable SHA-256 |
| --- | --- |
| Phase 18 reference | `f339eaf01f05ce09ace2a897baef8c9796ad19708f1209fd354b3edf260b985f` |
| Rebuilt private recipe | `a39ae30267c6e29788e4b7860e3c3167e161a2f19459cfbfdf6ad63261066a1f` |
| Target-based Linux recipe / RC1 | `ecf96666caafbb65015c370e1bab3da1aa9aa0580ab072c3670f45632de9a319` |

RC1 is 26,498,416 bytes. Its ELF RUNPATH is `$ORIGIN/lib`, without development
SDK paths. It links the upstream renderer target and uses SDL2, X11 headers and
the DBus portal backend. Both full build routes linked successfully with GCC 13.

## Tests actually performed

- Four native execution tests passed: boot translation, generated module
  execution, upstream overlay dispatch, cached/uncached RDRAM aliasing.
- The independent Linux recipe's memory-alias CTest passed.
- Thirteen Python project checks passed.
- The original reference ran for 469.9 seconds and closed cleanly, exit 0.
- The rebuilt private-recipe executable ran for 663.8 seconds, reached the ship
  gameplay room, responded to input and closed cleanly, exit 0.
- The packaged RC1 ran for 914.3 seconds using only packaged game libraries,
  X11 and bundled Lavapipe. No game fault or audio RSP failure was observed.
  It closed through SDL's window-close path with exit 0.
- RC1 exercised logos/title, language and save selection, opening cinematics,
  ship-room movement, camera controls, pause/resume and the planet-map display.
  Its log reached 42,000 graphics tasks and 54,600 audio tasks, with 107 distinct
  registered modules and nonzero PCM output.
- RC1 restarted against the same 512-byte EEPROM data and logged EEPROM reads.
  The saved English selection was retained: Start went directly to file
  selection, skipping the first-run language menu. The save hash stayed
  `3ad1406799cb53a7e9bb0c184d41a968d2b39b88c148ed413a9969ef17754682`.
  Restart QA ran for 73.3 seconds and closed cleanly, exit 0.
- Missing and wrong ROMs were rejected. A relative ROM path supplied from
  outside the installation directory reached native SDL initialization.
- `ldd` found no missing libraries for the executable, shader compiler or
  software Vulkan driver using the packaged library directory.

QA used SDL dummy audio. It establishes generated PCM, not audible speaker
output. Xvfb and its development SDK were used by the test harness; the **game
process** used its release libraries and release ICD rather than SDK libraries.
This is not a test on another clean Linux distribution.

## Remaining limits

No full mission, later-world transition, campaign ending or mission-progress
save/reload has been verified. The file slot remains at 0%/NEW, so EEPROM
settings persistence must not be described as a completed progress-save test.
Physical GPU performance, speakers, gamepads and other Linux distributions
remain untested. There is no full Windows game executable.

The release is a **playable Linux release candidate**, with stable opening
gameplay in these tests. It is not a full-campaign certification.
