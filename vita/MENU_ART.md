# Vita menu artwork

The private v00.12 menu-art package replaces the SDK's sample bubble and
LiveArea images with the North American Duck Dodgers cover, and sets both
SFO title fields to `Duck Dodgers`. Title ID, version, game executable and
save locations remain unchanged. This is an artwork-only update.

Run with Pillow installed, outside the public repository:

```sh
python3 vita/apply_menu_art.py --vpk ORIGINAL.vpk --cover cover.png --output Duck-Dodgers-Vita-v012-Cover.vpk
```

The script produces indexed PNGs at 128x128, 840x500 and 280x158, updates
the LiveArea XML and verifies that every other VPK entry is byte-identical.
The icon crop is intended for the North American cover scan. It preserves
the supplied full cover in the background and startup images.

Source scan: [libretro Nintendo 64 thumbnails](https://github.com/libretro-thumbnails/Nintendo_-_Nintendo_64/blob/master/Named_Boxarts/Duck%20Dodgers%20Starring%20Daffy%20Duck%20(USA)%20(En%2CFr%2CEs).png).
Original game artwork belongs to its respective rights holders. Artwork
and private VPKs are supplied separately and are not committed here.

Install the new VPK as an update in VitaShell. If the old bubble image
remains cached, restart the Vita after installing. Do not uninstall the
game or delete saves just to refresh artwork. Actual LiveArea appearance
still needs a hardware check.
