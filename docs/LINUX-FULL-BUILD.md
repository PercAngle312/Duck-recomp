# Full native Linux game build

Linux is the primary target. Windows follows Linux gameplay validation.
The Phase 18 executable is preserved as the historical reference, not used as
the output of a source build. Public CI preflight executables remain separate
from the full game.

## Inputs

Use the private complete handoff's generated game project and dependency SDK.
Keep the private project outside this public repository. Its required inputs
are `build/modules/funcs_*.c`, `build/runtime/funcs_*.c`, runtime bindings,
translated audio RSP C++, native runtime glue and section metadata.
No game ROM, translated proprietary code or save belongs in this repository.

The build script checks exact source commits before configuring:

| Dependency | Commit |
| --- | --- |
| N64Recomp | `ffb39cdad1da5de07eaaa48bd1db4a89a7986771` |
| N64ModernRuntime | `cdf5abbd5026fef5c364c676e4667c45e42b6863` |
| RT64 | `d9817c68ffd7328cd0af383af124971489366e18` |

All recursive submodules are required. Build tools are CMake >=3.20, a C++20
compiler (verified toolchain: GCC 13), Make or Ninja, Python 3 and Git.
The handoff's extracted SDK supplies SDL2 and DBus development packages plus
pkg-config. RT64's `NFD_PORTAL=ON` selects its DBus desktop portal backend.

## Build

From this repository, with the handoff paths substituted:

```sh
python3 scripts/build-full-linux.py \
  --game-source /private/workspace/duck-dodgers-recomp \
  --recompiler /private/workspace/N64Recomp \
  --runtime /private/workspace/N64ModernRuntime \
  --rt64 /private/workspace/rt64 \
  --graphics-sysroot /private/workspace/graphics-sysroot \
  --build /private/build-linux --jobs 2
```

`--configure-only` validates the configuration without compiling. An optional
`--rt64-build /private/workspace/rt64/build` chooses the renderer binary directory.
The CMake recipe uses `add_subdirectory` and the upstream `rt64` target, retaining
its transitive SDL2/DBus/re-spirv/Plume/zstd link requirements. It does not depend
on a list of hardcoded `.a` paths. The result is `build-linux/duckdodgers`.

The recipe consumes already generated game sources. It does not need a ROM to
compile them. Regeneration remains a private-project operation and requires
the user's compatible ROM and recompiler tools.

The optional `patches/linux-host-paths.patch` updates the older private native
host to honor `DUCK_USER_DATA`, then XDG/HOME fallbacks. Apply it once from the
private project using `patch -p1 < /public/repo/patches/linux-host-paths.patch`.
The updated private checkpoint already includes it. Preserve existing save
folders before switching paths. `scripts/run-linux.sh` becomes `run.sh` in a
release and sets a portable `userdata/` directory by default.

## Package and validate

`scripts/package-linux.py` requires an actual `duckdodgers` ELF, a supplied
runtime-library bundle, all corresponding licenses and an explicit validation
JSON. It creates a ROM-free archive, per-file checksums, executable identity and
an optional relocatable Lavapipe ICD. It never copies the private project.
Do not include user saves or the runtime's cached ROM in release archives.

Supported baseline: x86_64, SSE4.1, glibc 2.39, Ubuntu 24.04-compatible Linux,
X11/Wayland and Vulkan. The software renderer is an optional fallback rather
than evidence of physical GPU performance. Actual game tests must exercise
menus, movement, camera, interaction, audio, clean exit and save/restart behavior.
Native tests and successful linkage alone do not establish campaign playability.
