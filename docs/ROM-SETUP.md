# One-download Windows release — target user experience

The final release should be a single ZIP. Extract it anywhere, place **your own** Duck Dodgers N64 ROM in the `roms` folder, and run `DuckDodgers.exe`.

Expected contents:

```text
DuckDodgers-Windows/
  DuckDodgers.exe
  README-ROM.txt
  roms/
    <place your own Duck Dodgers .z64 here>
  licenses/
  (all necessary application DLLs and assets)
```

No Visual Studio, SDK, separate SDL/RT64 download, Python installation, developer tools, or online setup should be necessary for the **released** build. A compatible Windows operating system and GPU/driver remain necessary.

## Runtime ROM handling requirement (NOT YET IMPLEMENTED)
- Accept Duck Dodgers ROMs in `roms/` automatically.
- Recognize .z64/.v64/.n64 formats only after validating byte order and supported ROM hashes.
- Report a clear in-app missing-ROM or unsupported-ROM message.
- Keep saves/configuration next to the portable launcher, without administrator privileges.
- Ensure the game runtime can actually derive/load the required game data from the ROM supplied at launch. Existing translated source was generated from a game ROM: merely supplying a ROM with the executable does **not** automatically make the binary portable or redistributable.
- Include all required legal, redistributable DLLs and shader runtime files in the ZIP, with license notices. Run a Windows clean-machine dependency audit.
- Never ship the game ROM, extracted game content, or user saves.

## Status
The repository currently has Windows mapper, controller and display tests. The release packager deliberately rejects missing `DuckDodgers.exe`. This is not yet a playable final release, and the one-ROM setup above is the design contract rather than a completed feature.
