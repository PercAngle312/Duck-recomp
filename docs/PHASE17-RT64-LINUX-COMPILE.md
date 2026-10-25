# Phase 17 — native game-code compilation and RT64 source integration

**Verified on Linux (2026-10-25):** Recovered Phase 16 translated game code compiled successfully to `libduck_modules.a` and `libduck_runtime_resident.a` with `cmake --build build --target duck_modules duck_runtime_resident -j 12` (exit code 0). This is a significant improvement over the prior partial compile. It is **not a full executable build**.

A private CMake patch replaces RT64's hardcoded Linux `rt64.a`/other `.a` references with `set(RT64_STATIC ON)`, `add_subdirectory(RT64)` and target linking against `rt64`, so all RT64 components can be built by the same CMake graph. The Linux configuration entered RT64's own dependency checks and was blocked because the build environment lacks `gtk+-3.0` pkg-config files. SDL2 and dbus-1 development files are also unavailable. Installing/restoring those dependencies is necessary before completing RT64 configuration/linkage.

The Phase 17 archive supplied separately includes the CMake patch, build logs, both Linux `.a` artifacts, and instructions. It does not include a ROM or a playable executable. Windows MSVC compilation and gameplay have not been verified, and recovered proprietary generated title code stays outside this public repository.
