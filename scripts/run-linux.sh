#!/bin/sh
# This file is installed as run.sh beside the game executable.
set -eu
app_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
expected=2bbfe0cfc6aff7b623ae53673b8f91e9008cc5749fe07920712e0d0f9abe3fc5
if [ "${1:-}" = --safe-mode ]; then
    export DUCK_GRAPHICS_SAFE=1
    shift
fi
if [ "${1:-}" = --settings ]; then
    export DUCK_SHOW_GRAPHICS=1
    shift
fi
if [ "$#" -gt 1 ]; then
    echo 'Usage: ./run.sh [--safe-mode] [--settings] [path/to/Duck-Dodgers-US.z64]' >&2
    exit 2
fi
if [ "$#" -eq 1 ]; then
    case "$1" in /*) rom=$1 ;; *) rom=$PWD/$1 ;; esac
else
    count=0
    rom=
    for candidate in "$app_dir"/roms/*.z64 "$app_dir"/roms/*.Z64; do
        [ -f "$candidate" ] || continue
        count=$((count+1))
        rom=$candidate
    done
    if [ "$count" -ne 1 ]; then
        echo 'Place exactly one supported Duck Dodgers USA .z64 ROM in roms/, or pass its path to ./run.sh.' >&2
        exit 3
    fi
fi
if [ ! -f "$rom" ]; then
    echo "ROM file does not exist: $rom" >&2
    exit 3
fi
actual=$(sha256sum -- "$rom")
actual=${actual%% *}
if [ "$actual" != "$expected" ]; then
    echo "Unsupported ROM. Required SHA-256: $expected" >&2
    exit 3
fi
if [ "$(uname -m)" != x86_64 ]; then
    echo 'This build requires an x86_64 Linux computer.' >&2
    exit 4
fi
export LD_LIBRARY_PATH="$app_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# Explicit override supports read-only installations and existing save folders.
export DUCK_USER_DATA="${DUCK_USER_DATA:-$app_dir/userdata}"
mkdir -p -- "$DUCK_USER_DATA"
case "$DUCK_USER_DATA" in /*) ;; *) DUCK_USER_DATA=$PWD/$DUCK_USER_DATA; export DUCK_USER_DATA ;; esac
if [ "${DUCK_SOFTWARE_VULKAN:-0}" = 1 ]; then
    export VK_ICD_FILENAMES="$app_dir/share/vulkan/icd.d/lvp_icd.json"
fi
cd "$app_dir"
exec ./duckdodgers "$rom"
