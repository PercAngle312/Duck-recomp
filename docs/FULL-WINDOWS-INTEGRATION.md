# Full Windows compilation: verified source-tree handoff

The public repository contains a **test-only** `game/CMakeLists.txt`. It does **not** define `duckdodgers`. To build the game, use the recovered Phase 12 **full game** `game/CMakeLists.txt` together with the rest of that checkpoint in a separate/private working tree. The full CMake file defines:
- `duck_modules` and `duck_runtime_resident` from generated C translation units.
- `duck_boot_probe` and `duckdodgers` (with `DUCK_ENABLE_BOOT_PROBE=ON`).
- `RT64_STATIC=ON`, `RT64_SDL_WINDOW_VULKAN=OFF`, RT64 `add_subdirectory`, `find_package(SDL2 CONFIG REQUIRED)` and Windows system libraries.

Source layout for `scripts/build-full-windows.ps1`:

```
private-checkout/game/CMakeLists.txt    <- recovered full CMake, not public stub
private-checkout/game/runtime/
private-checkout/game/build/modules/funcs_0.c
private-checkout/game/build/resident/funcs_0.c
private-checkout/game/build/runtime/funcs_0.c
private-checkout/game/build/runtime/bindings.cpp
private-checkout/game/build/runtime/audio_rsp.cpp
private-checkout/N64Recomp/include/recomp.h
private-checkout/N64ModernRuntime/CMakeLists.txt
private-checkout/rt64/CMakeLists.txt
```

Example from repo root on Windows, after extracting the user's own local handoff and restoring the pinned source dependencies:

```powershell
./scripts/build-full-windows.ps1 -GameSource "C:/private/duck/game" -N64Recomp "C:/private/duck/N64Recomp" -N64ModernRuntime "C:/private/duck/N64ModernRuntime" -RT64 "C:/private/duck/rt64"
```

**Important:** This is a native compilation attempt, not proof of compatibility. The preserved full CMake also invokes RT64's Windows Direct3D12 build and requires SDL2's CMake package. Additional compiler errors, missing submodules, dependencies and symbol mismatches may appear. No full MSVC compilation has been tested. The standalone CI checks cannot establish playability.

## Fail-safe changes
The launcher validates the input files **and** checks that the supplied game CMake actually declares an executable named `duckdodgers`. CI injects synthetic dependency files plus a test-only CMake file to ensure the launcher refuses to report success.

## Final release
The planned artifact includes executable + runtime DLLs + notices + `roms/` folder; user supplies the legally owned compatible ROM. The runtime must still be tested loading that ROM locally, and dependency closure must be audited on a clean Windows machine.
