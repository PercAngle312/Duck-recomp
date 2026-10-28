# Audio starvation padding and explicit tile origins

Hardware v00.14 feedback reports better FPS but continued choppiness, distorted
audio and texture errors. These changes address two specific mechanisms; they
do not establish that all reported symptoms are fixed.

## Native audio output

The native worker now uses fixed 64-frame blocks, the pinned VitaSDK minimum
granularity, instead of 256 frames. At 48 kHz this reduces one output period
from 5.33 ms to 1.33 ms. A producer-starved partial block now inserts at most
63 silent frames instead of 255. No padding is inserted at a boundary when
another owned DMA is available.

The two-entry guest FIFO, immutable DMA snapshots, resampler, DMA/IRQ ordering,
failure handling and joined shutdown are retained. There is no output-port
reconfiguration and no false completion to feed the producer. The tradeoff is
four times as many native output calls. Long gaps caused by slow synthesis or
GPU blocking remain possible; hardware must establish the audible benefit and
CPU cost.

The concurrent worker regression passes with ASan/UBSan, including real-sized
552-frame single DMAs at actual rates 24,004 and 32,006 Hz. It checks captured
PCM, final silence bounds, ownership under RAM mutation, ordered completion,
refills, device errors and shutdown. Those two single-DMA tails contain 48 and
4 padded frames with the new block size.

## Texture row origin

The previous renderer inferred LoadBlock's width=1 sentinel from
`row bytes == full-image stride == total bytes`. A genuine one-row LoadTile
can have exactly those values too. Selecting its padded TMEM stride can
produce the wrong dimensions or zero height (for example a one-pixel RGBA16
row is 2 source bytes but the TMEM row is 8 bytes).

Loaded textures now retain an explicit tile/block origin. All ten import-side
stride decisions, source-pitch fallbacks and triangle UV dimensions use that
origin. Cache identity includes it, so matching addresses and byte counts do
not alias different load semantics. The existing DXT=0 unswizzle remains
specific to block loads. No filtering or resolution changes are made.

Tests compile the actual prepared renderer stride function and cover 132
tile/block combinations plus one-row RGBA16 and CI4 pixel fixtures. All five
renderer units and the 29-module runtime compile with the pinned VitaSDK; the
complete private game ELF links with no unresolved strong imports.

Full GPU readback stalls, complete TMEM/mip behavior, audible correctness and
the remaining hardware texture artifacts are still unresolved. A current
v00.14 gameplay clip and game.log are needed to identify those remaining
artifacts rather than infer them from older builds.

The pinned VitaSDK CI run 40672813922 passes these source changes. The new
emulator attempt stops before guest startup at host SDL input-device
initialization (UDEV); it does not validate this candidate in-game.
