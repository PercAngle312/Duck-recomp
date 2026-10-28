"""Adapt pinned N64Recomp ABI to ARM32 without editing upstream or game data."""
from pathlib import Path
import argparse

p=argparse.ArgumentParser()
p.add_argument("--header",type=Path,required=True)
p.add_argument("--portable-header",type=Path,required=True)
a=p.parse_args()
s=a.header.read_text()
anchor="#elif defined(_MSC_VER)\n\n#include <intrin.h>"
assert s.count(anchor)==1, "Pinned header layout changed"
replacement="#elif defined(__arm__) && !defined(__aarch64__)\n#include \"portable_mul.h\"\n#define DMULT duck_dmult\n#define DMULTU duck_dmultu\n\n"+anchor
s=s.replace(anchor,replacement)
a.header.write_text(s)
assert a.portable_header.exists(), "Portable 64x64 multiply implementation absent"
print("ARM32 portable DMULT/DMULTU compatibility enabled")
