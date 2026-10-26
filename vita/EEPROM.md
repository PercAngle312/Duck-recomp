# Persistent EEPROM service

`duck_vita_eeprom_init` binds a 512-byte (4-Kbit) or 2048-byte (16-Kbit)
image to an explicit storage backend. The application must bind this before
guest startup; an unbound device probes absent. Failed initialization detaches
it instead of leaving a stale save image active.

`osEepromLongRead/Write_recomp` use eight-byte block addresses and preserve
word-swapped guest byte lanes, including unaligned buffers. Invalid sizes and
out-of-range transfers fail. Writes commit a full candidate image first; only
a successful storage commit changes the active image and returns success.
A failed backend commit leaves the in-memory save unchanged.

`duck_vita_eeprom_file` uses native Vita I/O handles on Vita and POSIX I/O on
host tests. It syncs a temporary image, rotates the old image to `.bak`, renames
the temporary image to the main path, and syncs filesystem metadata. Loading
recovers `.bak` when main is absent after an interrupted rename. A malformed
existing main fails initialization; it is never silently reset or overwritten.
The caller creates the parent directory and owns the backend/path lifetime.

This is synchronous EEPROM HLE. It does not reproduce SI/PIF bit traffic or
libultra's per-block programming delay. These timing and bus-arbitration limits
must be considered during gameplay validation. Native filesystem behavior and
actual gameplay saving remain untested on physical Vita hardware.

Host ASan/UBSan checks cover image boundaries, persistent reload, rejected
writes, backup recovery, malformed files and guest byte layout. Native VitaSDK
compilation checks the actual I/O API path. Private game-address aliases belong
outside the public repository and must use the verified original mappings.
