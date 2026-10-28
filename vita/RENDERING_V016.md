# Rendering corrections after the v00.15 hardware report

The user's v00.15 video shows two black vertical strips inside the widened
scene and incorrect colours. Its log has no border-anchor matches, no SP
failures, and repeated audio starvation. Shader success and SP completion do
not establish rendering correctness.

## Captured overscan commands

The recovered private first-task RDRAM capture contains two copies of this
opaque black RGBA5551 (`0001`) FILLRECT border sequence, in native pixels:

| Edge | Inclusive bounds |
| --- | --- |
| Left | (0,17)–(23,222) |
| Right | (295,17)–(319,222) |
| Top | (0,0)–(319,17) |
| Bottom | (0,221)–(319,239) |

The previous matcher rejected all four: the side strips exceed its 20-pixel
width cap and stop inside the vertical overscan margins. The corrected matcher
accepts side strips up to one eighth of native width with top/bottom margins
up to one eighth of native height. It moves the two side strips to the physical
widescreen edges, retaining their width. It expands the horizontal overscan
fills across the widened display. Native 4:3 placement is unchanged.

The existing guards still require a main-view opaque black FILL-cycle colour
rectangle. Depth clears, textured HUD rectangles, central panels, full-screen
fades, fixed offscreen buffers and the `borders=original` override retain their
previous behavior. Matching commands produce the existing bounded anchor log.
The capture explains a concrete missed command pattern; physical confirmation
that it fixes the user's present gameplay strips is still required.

## One-cycle material colours

Fast3D stores two 28-bit colour/alpha mux sets, but previously always selected
the first set for non-two-cycle shaders. N64 one-cycle mode uses the second
set. Compare the independent reference implementation's
[combiner_1cycle](https://github.com/ata4/angrylion-rdp-plus/blob/9c8b9ed3e7d7f00dff8bc872ccdd3fba1a3673fc/src/core/n64video/rdp/combiner.c#L173),
which uses mux index 1 for RGB and alpha.

The shader key now selects the second set only for `G_CYC_1CYCLE`. Raw emulated
registers are preserved, two-cycle shaders keep both sets, and COPY/FILL modes
keep their existing synthetic first-set shaders. Regression cases cover
different RGB/alpha values, each of 56 packed source bits, identical-cycle
macros and synthetic-mode preservation. This fixes a material-selection bug;
it does not establish that every reported texture error has the same cause.

Derived shader binaries move automatically to `shaders-v008`. Users do not
need to rename or delete any folder. First material appearances can compile
fresh shaders.

## Audio gaps

The production audio worker now applies a 16-sample taper at partially padded
native blocks and a 16-sample fade-in when real samples resume. It changes no
continuous queued PCM, sample counts, resampler phase, FIFO capacity or AI
completion timing. Tests preserve the bit-exact uninterrupted two-DMA fixture
and exercise partial-block ramp endpoints and resume state across blocks.

This targets waveform-to-silence clicks. It does not remove starvation or prove
an audible improvement; translated audio synthesis and GPU/CPU scheduling
remain separate performance concerns.

## Validation and limits

The corrected overscan bounds, combiner selector and audio worker are checked
with host ASan/UBSan (leak scanning disabled locally because this execution
host denies the required process inspection). The full production runtime and
renderer are rebuilt with the pinned VitaSDK for the private game link. No
ROM, generated game code, private capture or shader compiler enters public Git.

No hardware rendering, listening or new FPS improvement is claimed for v00.16.
Framebuffer readback and incomplete texture/TMEM behavior remain open concerns.
