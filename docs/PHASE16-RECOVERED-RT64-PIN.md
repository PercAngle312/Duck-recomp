# Phase 16 — Original RT64 revision recovered

The retained 489 MB private Linux handoff contains the **actual RT64 checkout's `.git` metadata** under `workspace/rt64/.git/`. Its `refs/heads/main`, `packed-refs`, and `shallow` all identify commit **`d9817c68ffd7328cd0af383af124971489366e18`**. Its Git `origin` points at `https://github.com/rt64/rt64.git`.

The same handoff records:

| Dependency | Commit |
|---|---|
| N64Recomp | `ffb39cdad1da5de07eaaa48bd1db4a89a7986771` |
| N64ModernRuntime | `cdf5abbd5026fef5c364c676e4667c45e42b6863` |
| RT64 | `d9817c68ffd7328cd0af383af124971489366e18` |

The separate **private Phase 16 ZIP** now merges the complete recovered generated source and original full-game CMake with the Phase 15 test fixes. Its PowerShell builder clones these three exact revisions with recursive submodules. This is a historical source revision recovery, **not** verification of the Windows compilation, binary dependencies, portable release, or gameplay.

Local Linux verification of this merged source with `DUCK_BUILD_GAME=OFF`: CMake configured, tests compiled, CTest **3/3 passed**. Full Windows MSVC/RT64 game compilation has not occurred. Generated title source remains outside the public repository pending redistribution rights review.
