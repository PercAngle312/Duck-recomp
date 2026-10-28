"""Build a private generated-code dispatch table from an explicit address map.

No automatic symbol-to-VRAM guesses. Input: CSV with columns vram,symbol.
Source and output stay in the private build directory; don't commit either.
"""
import argparse
import csv
import re
from pathlib import Path

SYMBOL = re.compile(r"[A-Za-z_][A-Za-z0-9_]*\Z")
def generate(mapping: Path, output: Path, source_root: Path | None = None) -> int:
    records = []
    with mapping.open(newline="", encoding="utf-8") as stream:
        rows = csv.DictReader(stream)
        if rows.fieldnames != ["vram", "symbol"]:
            raise ValueError("expected exact CSV header: vram,symbol")
        for line, item in enumerate(rows, 2):
            raw, symbol = item["vram"], item["symbol"]
            if not raw or not re.fullmatch(r"0[xX][0-9a-fA-F]{8}", raw):
                raise ValueError(f"line {line}: require eight-digit hexadecimal VRAM")
            addr = int(raw, 16)
            if addr & 3 or not SYMBOL.fullmatch(symbol or ""):
                raise ValueError(f"line {line}: unaligned address or invalid C symbol")
            records.append((addr, symbol))
    if not records or len(records) > 100000:
        raise ValueError("expected between 1 and 100000 entries")
    records.sort()
    if len({a for a, _ in records}) != len(records):
        raise ValueError("duplicate VRAM entry")
    if len({s for _, s in records}) != len(records):
        raise ValueError("duplicate function symbol")
    if source_root is not None:
        # Symbols are expected as C function definitions from the *private*
        # generated game tree. Confirm presence before emitting the registry.
        # This is a source-level preflight; the linker remains authoritative.
        if not source_root.is_dir():
            raise ValueError("private source directory does not exist")
        wanted = {symbol for _, symbol in records}
        found = set()
        duplicate = set()
        definition = re.compile(
            r"\b(?:RECOMP_FUNC\s+)?void\s+([A-Za-z_][A-Za-z0-9_]*)\s*"
            r"\(\s*uint8_t\s*\*\s*\w+\s*,\s*recomp_context\s*\*\s*\w+\s*\)"
            r"\s*\{"
        )
        for path in source_root.rglob("funcs_*.c"):
            # Private generated modules/resident/boot functions only.
            for match in definition.finditer(path.read_text(encoding="utf-8", errors="replace")):
                if match.group(1) in wanted:
                    if match.group(1) in found:
                        duplicate.add(match.group(1))
                    found.add(match.group(1))
        if duplicate:
            raise ValueError("ambiguous duplicate function definitions: " + ", ".join(sorted(duplicate)[:12]))
        missing = wanted - found
        if missing:
            raise ValueError("unverified function definitions: " + ", ".join(sorted(missing)[:12]))
    code = ['/* Generated from PRIVATE explicit address map. Do not publish with game source. */',
            '#include "duck_vita_dispatch.h"']
    for _, symbol in records:
        code.append(f'extern void {symbol}(uint8_t *, recomp_context *);')
    code.append('static const DuckVitaDispatchEntry entries[] = {')
    for addr, symbol in records:
        code.append(f'    {{0x{addr:08x}u, {symbol}}},')
    code.extend(['};',
                 'const DuckVitaDispatchTable duck_vita_game_dispatch = {',
                 '    entries, sizeof(entries)/sizeof(entries[0])',
                 '};', ''])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(code), encoding="utf-8")
    return len(records)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, help="Private generated C source directory for symbol validation")
    args = parser.parse_args()
    print(f"Generated {generate(args.map, args.output, args.source_root)} explicit entries")
