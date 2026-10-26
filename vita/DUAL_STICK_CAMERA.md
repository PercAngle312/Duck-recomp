# Vita gameplay camera controls (00.10)

Left stick moves Daffy. Right stick left/right sends the original camera C-left/right controls. Triangle toggles the original stationary look mode; press it again to leave. Square sends C-down to recenter the camera. All other mappings remain unchanged.

Right-stick vertical travel is intentionally inactive: C-up toggles a mode which reassigns the movement stick to the view, and C-down is a reset, not continuous pitch. Removing those bindings prevents diagonal sweeps from accidentally entering look mode. This is horizontal dual-stick control using the game camera, not a smooth analog yaw/pitch replacement. The game may restrict camera actions in scripted scenes or confined areas.

A full modern camera needs verified game-specific yaw/pitch hooks and mode checks for gameplay, aiming, menus and cutscenes. No unverified guest-memory camera writes are included.

Validation: controller mapping fixture checks every right-stick position while moving, deliberate look/reset buttons, hysteresis, no-sample behavior and disconnect recovery. The complete private game is relinked with the pinned VitaSDK; hardware camera behavior still requires confirmation.
