# Duck Dodgers — PC Native Recompilation

Linux is the reference build; Windows x64 is under active development.

**[Roadmap](ROADMAP.md)** · **[Windows CI](https://github.com/PercAngle312/Duck-recomp/actions)**

## Current verification

- Linux: native executable reached menus and 3D gameplay under RT64/Lavapipe; the complete campaign has **not** been verified.
- Windows: MSVC Windows SDK test demonstrated coherent RAM aliases and safe release ([passing run](https://github.com/PercAngle312/Duck-recomp/actions/runs/40164179463)).
- Windows CMake baseline configures and compiles a native test; its Release CTest passed on a real Windows runner.
- **No playable `DuckDodgers.exe` has been produced.**

## Source import status

The Phase 11 CMake and game source checkpoint has been recovered and tested locally, but **the full generated game source and pinned third-party source trees are not yet committed**. This repository currently contains the native Windows validation and CI foundation. See [ROADMAP.md](ROADMAP.md) for the complete import/build sequence.

**No original ROM, proprietary assets, or EEPROM saves should ever be committed to this public repository.** Players must supply their own legitimate game data locally.

## CI

`windows-sdk-baseline.yml` directly compiles/tests the Win32 memory alias API. `windows-cmake.yml` separately exercises the CMake + CTest path.

## Build status

The CMake root currently builds a stand-alone Win32 test. `-DDUCK_BUILD_FULL_GAME=ON` intentionally fails with a clear message until `game/CMakeLists.txt` is imported.
