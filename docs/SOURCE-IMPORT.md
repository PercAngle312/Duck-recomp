# Full game source import policy and build blocker

The complete Phase 11 Windows CMake checkpoint has been recovered separately and repackaged under a `game/` prefix as a ZIP containing **289 source/config/report files** (~8.5 MB compressed). An accompanying SHA-256 manifest verifies all entries.

The generated `game/build/modules/funcs_*.c`, `game/build/resident/funcs_*.c` and related files are translations of the original copyrighted title. **Do not commit or publish these generated files to this public GitHub repository without verifying redistribution rights.** No game ROM or save is included in the private import checkpoint.

## What is in this public repository

Windows RAM mapper, display settings, controller mapping, portable save paths, ROM locator, tests, portable ZIP packaging script and documentation. The standalone Windows CMake CTests are not equivalent to compiling the game.

## Private full-game integration route

1. Use the separately supplied Phase 12 source checkpoint ZIP with `game/` layout and its `IMPORT-MANIFEST.json` integrity manifest.
2. Extract to a **private** working checkout (or a restricted CI environment whose policies and rights you control).
3. Obtain pinned dependency *source*: N64Recomp, N64ModernRuntime, RT64 including recursive submodules, plus SDL2 and their permitted redistributable runtime dependencies.
4. Set `-DDUCK_BUILD_FULL_GAME=ON` and compile the actual `duckdodgers` target after applying Windows runtime integrations.
5. Bundle the compiled program, dependency DLLs and licenses through `scripts/package-release.ps1`. Do not release merely because that script creates a ZIP: validate the ROM loader and startup/gameplay first.

The upstream Phase 11 `game/CMakeLists.txt` currently requires generated C, pinned N64Recomp headers and N64ModernRuntime; its Windows graphics target also requires RT64 and SDL2. Windows full-game compilation has **not** been confirmed. Merely dropping a ROM into `roms/` does **not** load translated game data yet; that interface is still a planned release feature.
