# Duck Dodgers — native PC recompilation

[Development roadmap](ROADMAP.md) · [Windows CI](https://github.com/PercAngle312/Duck-recomp/actions) · [Portable ROM setup](docs/ROM-SETUP.md)

## Goal: one download, supply your ROM, play

The intended **final Windows release** is a single ZIP with `DuckDodgers.exe`, the graphics/input runtime, redistribution-safe DLLs and necessary support data. Extract the ZIP, place your legally owned Duck Dodgers USA 16 MiB big-endian `.z64` file under `roms/`, and double-click `DuckDodgers.exe`. No developer toolchain, standalone emulator, Python install, or additional DLL download should be needed.

**This is not playable yet.** The full game runtime, ROM-to-game-data handoff, RT64 Direct3D 12 binding, complete generated game source, and real `DuckDodgers.exe` are still missing from the public repository. Do not download the current test artifacts expecting a game.

## Verified on Windows Server 2022

- Native Win32 dual RDRAM aliasing and the imported Phase 11 mapper, including coherent 8 MiB views and cleanup
- Graphics option parsing and controller button translation
- Windows executable-relative portable save folder handling
- Automatic recognition of exactly one correctly structured 16 MiB big-endian `.z64` file in `roms/` (test uses synthetic data, **not** the actual game)

**[CMake / Release 6/6 passing CI run](https://github.com/PercAngle312/Duck-recomp/actions/runs/40166192168)**

The Linux reference build previously reached the opening menus and 3D scene with Lavapipe. Full campaign validation is pending.

## Source import

`port/runtime` contains source-level Windows mapper, graphics, controller, save path and ROM locator components from / inspired by the recovered Phase 11 checkpoint. `port/tests` contains the associated Windows regression tests. `scripts/package-release.ps1` stages a future portable ZIP **only after** a real `DuckDodgers.exe` is available. `game/` remains a missing full-game source integration target; translated proprietary game code and ROM are not committed here.

## Copyright and release constraints

Never commit game ROMs, extracted game assets, EEPROM saves or proprietary game payloads. The launcher should request the user-supplied ROM locally and validate it. Release ZIPs should contain only materials that can legally be redistributed, along with required licenses. A complete dependency and clean-machine test is required before describing the Windows package as standalone.
