# Phase 15 — recovered full-game CMake test sources

The private source checkpoint `Duck-Dodgers-Phase15-CMake-Complete-Test-Sources.zip` contains the **actual recovered Phase 14 full-game CMake** and test sources missing from the Phase 14 patch. The archive restores `game/tests/test_controller.cpp`, `game/tests/platform_paths_test.cpp`, `game/runtime/duck_controller_mapping.h`, `game/runtime/platform_paths.h`, and `game/runtime/windows_rdram_alias.h` from Phase 12.

The full-game targets remain available when `DUCK_BUILD_GAME=ON`; `-DDUCK_BUILD_GAME=OFF` now configures portable controller/display/path CTests on either Linux or Windows. Windows also defines the real RDRAM alias test and links `onecore`.

**Actually verified in the local Linux environment:** `cmake -S game -B build -DDUCK_BUILD_GAME=OFF`, `cmake --build build -j2`, and `ctest --test-dir build --output-on-failure` — **3/3 passed**. This specific complete patched source tree has **not** yet passed a Windows runner. Prior Windows CI passed the separate public-repo suite. The complete translated game source, pinned third-party checkouts, game-runtime linking, and actual `DuckDodgers.exe` are still outstanding.

The full game and its private generated code are not committed to this public repository. The source checkpoint ZIP is provided separately in the development conversation.
