# Duck Dodgers PC recompilation — roadmap

Updated 2026-10-25. **Linux is the gameplay reference; Windows is pre-release.**

## Verified milestones
- [x] Recovered complete Linux source and pinned N64Recomp, N64ModernRuntime and RT64 dependencies in the user-provided handoff.
- [x] Linux native executable compiled and linked against RT64.
- [x] Software Vulkan (Lavapipe) ran the rebuilt Linux game through language selection, save selection, opening sequence and into 3D gameplay.
- [x] Linux generated 512-byte EEPROM saves, survived restart, and passed native regression tests.
- [x] Windows Server 2022 MSVC compiled a native RAM-alias test and verified coherent dual mapped views: [successful baseline run](https://github.com/PercAngle312/Duck-recomp/actions/runs/40164179463).
- [x] Windows CMake x64 Release build passes RDRAM alias, graphics settings, and controller-mapping CTests ([CI run](https://github.com/PercAngle312/Duck-recomp/actions/runs/40165678745)).
- [x] Imported production Phase 11 Windows RDRAM mapper and verified it with native Windows CTest ([passing run](https://github.com/PercAngle312/Duck-recomp/actions/runs/40166061940)).
- [x] Imported portable Windows save-path handling and one-folder ROM discovery code; automated Windows checks in progress.
- [x] Added release-packaging script that requires real DuckDodgers.exe, includes runtime DLLs/licenses, and excludes ROMs.
- [ ] **Full-game Windows executable is not compiled.**
- [ ] Full game playthrough and true progress-save reload unverified.

## First Windows playable build
1. **Import source safely:** Phase 11 mapper, portable paths, graphics and controller parts are imported under `port/`; full generated game source and source-only dependency checkouts remain to be integrated in `game/`. Exclude ROMs, extracted game content, saves, and bundled vendor binaries from the public repository.
2. **Repair CMake preflight:** Keep small independent Windows tests buildable without the large game dependency tree. The imported game CMake currently requires `N64RECOMP_ROOT` even for basic tests; fix this gate. Set RT64 and runtime paths with verified CMake cache variables.
3. **Real Windows runtime:** Compile and run the `VirtualAlloc2`/placeholder RDRAM mapper on Windows. Add retry/cleanup tests; check 4 GiB reservation and dirty alias coherence.
4. **MSVC full build:** Compile translated modules and N64ModernRuntime, RT64 with Direct3D12, SDL2 input, EEPROM loader and PC backend; produce `DuckDodgers.exe` without proprietary ROM embedded in the distributed build.
5. **Runtime assets:** Require the user to supply their legally obtained ROM on first launch. Validate hash, generate or load only locally derived game data, and ensure CI artifacts exclude copyrighted game content.
6. **Graphics UI:** Resolution presets, windowed/borderless, render-scale 1x–4x, MSAA, widescreen (experimental), vsync/display target. **Display refresh is not equivalent to game FPS.** HFR interpolation requires game testing.
7. **Input/audio:** SDL controller hot-plug, deadzones, remapping, audio queue limits, Windows audio device handling.
8. **Windows smoke checks:** Menu → 3D gameplay, analog movement, clean exit, 512-byte EEPROM roundtrip, restart, audio and shader checks.
9. **Progression QA:** Checkpoint persistence, cutscenes, level transitions, all objectives, crashes, widescreen and FPS compatibility.
10. **Release:** Windows ZIP with DuckDodgers.exe, redistributable runtime DLLs, shader files, license notices, troubleshooting and checksums. User drops a legally owned supported .z64 into `roms/` and starts the game, without additional downloads. The binary still needs an actual compatible game-data loading strategy.

## CI stages
- `windows-sdk-baseline.yml`: direct MSVC Windows API smoke test, **passed**.
- `windows-cmake.yml`: CMake x64 baseline and Release CTest **passed** after fixing test memory access optimization.
- **Not yet added:** full-game Windows build workflow requiring complete vetted source and reproducible dependency checkouts.

## Repository rules
- Do not commit Duck Dodgers ROMs (.z64/.n64/.v64), extracted images/audio, EEPROM saves, or binary game payloads.
- Preserve dependency license notices, pinned upstream source revisions, and the original Linux test baseline.
- Do not label a CI test executable as a playable Windows game.

## Phase 13 Windows game CMake preflight — 2026-10-25
- [x] Added `game/CMakeLists.txt` with isolated MSVC runtime integration builds and clear full-game dependency checks.
- [x] Ran the game-specific Windows 2022 CMake Release configure, compilation and CTest: [workflow run](https://github.com/PercAngle312/Duck-recomp/actions/runs/40199158516).
- [x] CI verifies that `DUCK_BUILD_GAME=ON` refuses missing private generated sources and dependency checkouts with a useful error.
- [ ] Full-game sources are not imported to public CI; this is **not** a full game build or playable executable.

## Full Windows build launcher — 2026-10-25
- [x] Added `scripts/build-full-windows.ps1`, which validates generated game source, N64Recomp headers, N64ModernRuntime and RT64 before invoking the original full-game CMake with `DUCK_BUILD_GAME=ON`, `DUCK_ENABLE_BOOT_PROBE=ON`, and the `duckdodgers` target. This avoids inadvertently building only tests.
- [ ] Running the full game compiler/linker still requires a private source checkout plus pinned, complete dependency trees. The public runner does not contain these; no playable EXE exists yet.
- [ ] Investigate MSVC compilation/linkage and RT64 D3D12 after the actual game inputs are supplied.
