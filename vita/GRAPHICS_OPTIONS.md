# Vita widescreen and image options

The renderer can now load `ux0:data/DuckRecomp/graphics.ini` at startup. A missing
file uses the packaged `app0:graphics.ini` preset if present; when neither file
exists the v00.13 display defaults are preserved. Restart the application after
changing options. The `graphics-presets` directory contains ready-made files;
copy the chosen file to the above location and name it `graphics.ini`.

| Setting | Values | Effect |
| --- | --- | --- |
| `aspect` | `original`, `wide` | Centered 4:3 or the full 960x544 Vita display |
| `quality` | `native`, `enhanced`, `display` | Internal rendering resolution |
| `filter` | `n64`, `bilinear`, `nearest` | N64 three-point, hardware bilinear where requested by the game, or point texture sampling |
| `presentation` | `sharp`, `smooth` | Point or bilinear scaling of the final game image |
| `borders` | `edge`, `original` | Anchor matching thin black screen-edge fill strips to the wide display edges, or retain their original 4:3 placement |

| Quality | Original aspect | Wide aspect |
| --- | --- | --- |
| native | 320x240 | 424x240 |
| enhanced | 640x480 | 848x480 |
| display | 725x544 | 960x544 |

`widescreen.ini` uses the wide view, native resolution, bilinear textures and
smooth presentation. `widescreen-enhanced.ini` increases internal resolution.
`original.ini` restores the prior appearance. No setting automatically changes
Vita clocks beyond the existing renderer clock policy.

The wide view uses the interpreter's existing clip-space aspect correction
before clipping: it expands the horizontal 3D view instead of stretching the
original 4:3 picture. Fixed-aspect offscreen buffers keep their existing behavior.
2D rectangles remain centered at their original proportions, so menus and HUD
can retain side margins. Game-side object culling and special camera sequences
still need physical gameplay inspection; the renderer cannot restore objects
the guest never submits.

FullSync still produces a 320x240 guest RAM mirror. In widescreen it crops the
original center view before downsampling, avoiding a squeezed wide image in
guest framebuffer textures. Optional smooth scaling affects presentation copies
to the default framebuffer, leaving guest readback filtering unchanged. Shader
cache keys already include the texture filtering mode.

Higher resolution sharpens geometry and reduces visible pixel stair-stepping;
it is not MSAA, a replacement texture pack, or new source texture detail.
Bilinear filtering smooths original texels and may look softer. Widescreen draws
more pixels, and enhanced resolution is substantially more expensive. Neither
is a fix for existing readback stalls or a verified performance improvement.

Host tests validate bounded presets, malformed configuration, aspect math and
the guest readback crop. All five renderer units are cross-compiled with the
pinned VitaSDK and the complete private game is linked. See
`reports/graphics-options.json` for the actual emulator validation scope.
Visual quality and HUD/culling behavior require Vita gameplay confirmation.

The `edge` border setting is the default for new candidates. It applies only
to main-view, opaque-black fill-cycle rectangles spanning the native screen
height and at most 1/16 of its width. Such strips otherwise stay at the old
4:3 edges with scene pixels visible outside them. It preserves strip width and
moves them to the outer display edges. Textured rectangles, partial-height HUD
panels, central fills, full-screen fades, fixed offscreen buffers and original
4:3 rendering keep their existing behavior. `borders=original` disables this
correction if a scene needs the original behavior. The first matching commands
are logged as `Widescreen border anchor` for hardware confirmation. This is a
targeted interpretation of the reported interior black strips; their exact
guest commands and the physical result have not yet been verified.
