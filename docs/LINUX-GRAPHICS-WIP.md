# Graphics work preserved before the Vita experiment

The native F5 window implements saved window resolution, internal scaling,
MSAA capability checks, aspect ratio, filtering, presets, F11 fullscreen,
safe-mode startup and V-sync on restart. The real Linux process applied
1920x1080 / 4.5x / 4x MSAA, then 16:9 / linear filtering / texture-filter changes
and fullscreen. This is not a new released ZIP yet.

An unattended demo cleanup exposed callback dispatch after a module free.
The loader now retains native code until replacement instead of removing it
at allocation free. An actual upstream overlay-dispatch regression passes;
the extended run closed cleanly after 600 seconds, but graphics/audio task
counters stopped advancing in the unattended profile. Transition stability
still requires investigation. This work is separate from the tested
Phase 19 release. Do not label the prototype graphics run crash-free.

Apply both patches/linux-graphics-settings.patch and
patches/linux-overlay-lifetime.patch to the Phase 19 private project before
using the updated CMake recipe. The renderer settings code and lifetime helper
are public-safe; translated game C and assets remain private.
