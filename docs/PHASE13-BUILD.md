# Phase 13 Windows source-build preflight

The recovered Phase 12 `game/CMakeLists.txt` currently requires N64Recomp headers and generated translated code before configuring even the portable Windows tests. The Phase 13 local patch adds an opt-out to the recovered CMake project:

```cmake
option(DUCK_BUILD_GAME "Build translated native game code" ON)
if(NOT DUCK_BUILD_GAME)
  message(STATUS "DUCK_BUILD_GAME=OFF: configuring portable regression tests only")
  return()
endif()
```

Configure the private imported source in standalone regression-test mode:

```powershell
cmake -S game -B build-portable -A x64 -DDUCK_BUILD_GAME=OFF
cmake --build build-portable --config Release
ctest --test-dir build-portable -C Release --output-on-failure
```

The full game remains a separate build requiring generated C files and pinned N64Recomp / N64ModernRuntime / RT64 trees. **The isolated preflight mode does not build DuckDodgers.exe.**

The separate Phase 13 checkpoint ZIP contains the complete recovered game CMake file with this option and its source-level regression tests, but NOT the proprietary translated game code. It is not a playable release.

The public root CMake continues to exercise its independently compiled Windows CTests, which were previously 6/6 green.
