# v00.22: optional live stats panel

Hold Select to open Settings. Set LIVE STATS to ON using left/right, then
choose APPLY AND SAVE. The panel defaults off and `stats=on|off` persists in
`ux0:data/DuckRecomp/graphics.ini`. Cancel preserves the active value.

The top-right panel displays:
- FPS: actual game framebuffer swaps per second over at least 500ms.
- FRAME: average milliseconds per presented game frame over the same window.
- RENDER: internal width and height.
- AUDIO BUFFER: queued native stereo PCM frames converted to milliseconds.
- UNDERRUNS: cumulative native output blocks which required padding.
- SILENCE: cumulative padded samples converted to milliseconds.

Audio totals include time spent paused in settings. They do not represent a
count of distinct audible interruptions. FPS excludes settings-menu swaps;
opening settings resets its sample window. The initial window shows dashes.

Rendering uses the existing bitmap font in one batched triangle draw, with a
dark backdrop. It draws after the guest framebuffer RAM mirror and before
presentation, restores touched GL state, and initializes its shader only when
enabled. The disabled overlay does not allocate geometry or submit draws.

Validation: existing settings/config tests extended for toggle, cancel,
8-row wrapping, save/reload and defaults; ASan/UBSan pass. Existing audio
regressions pass. Full VitaSDK compilation/link performed. Physical overlay
appearance, performance overhead and accuracy in gameplay remain unverified.
The v021 audio recovery behavior and existing ROM/save paths are retained.
