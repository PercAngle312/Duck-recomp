# Phase 14 — recovered full-game CMake Windows patch

This patch applies to the **original recovered Phase 12** game CMake, not the public `game/CMakeLists.txt` (which is only a smoke-test scaffold). The patched source ZIP is attached in the development handoff.

Changes:
- Added `option(DUCK_BUILD_GAME "Compile translated game sources and native game" ON)` and a test-only early return for `DUCK_BUILD_GAME=OFF`. The default ON preserves the full-game targets.
- Linked `onecore` into the original `duck_windows_alias_test` target on Windows. This import library is needed for `VirtualAlloc2`, `MapViewOfFile3`, and `UnmapViewOfFile2`.
- Converted the original recovered display test to portable environment setters (`_putenv_s` on Windows, `setenv` on Linux), with always-active checks instead of `assert` that disappears under MSVC Release.

**Local checks:** Linux GCC optimized Release display test passed; Linux CMake `DUCK_BUILD_GAME=OFF` configure passed. **Not yet validated on real Windows.** No full game compile/link claimed.

To validate the original source in Windows CI without proprietary translated game code, first restore the patched `game/` test sources and runtime headers in a restricted runner, then use:

```powershell
cmake -S game -B build-win-smoke -A x64 -DDUCK_BUILD_GAME=OFF
cmake --build build-win-smoke --config Release
ctest --test-dir build-win-smoke -C Release --output-on-failure
```

The complete build still needs N64Recomp, N64ModernRuntime, RT64 with recursive dependencies, SDL2, and private generated source. Original RT64 revision has not been confirmed. **Playable `DuckDodgers.exe` has not been built.**
