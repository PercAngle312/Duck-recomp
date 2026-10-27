#!/usr/bin/env python3
"""Apply supplied cover art to a private VPK without rebuilding the game."""
import argparse
import copy
import hashlib
import io
import json
from pathlib import Path
import struct
import zipfile

from PIL import Image, ImageOps


def png(image):
    stream = io.BytesIO()
    image.quantize(colors=256, method=Image.Quantize.MEDIANCUT).save(
        stream, format="PNG", optimize=True)
    return stream.getvalue()


def title_sfo(data):
    data = bytearray(data)
    magic, version, keys, values, count = struct.unpack_from('<5I', data)
    if magic != 0x46535000:
        raise ValueError('Invalid SFO')
    updated = set()
    for i in range(count):
        offset = 20 + i * 16
        key, fmt, length, capacity, value = struct.unpack_from('<HHIII', data, offset)
        name = bytes(data[keys + key:]).split(b'\0', 1)[0]
        if name in (b'TITLE', b'STITLE'):
            text = b'Duck Dodgers\0'
            start = values + value
            if fmt != 0x204 or len(text) > capacity or start + capacity > len(data):
                raise ValueError('Invalid SFO title storage')
            data[start:start + capacity] = text + bytes(capacity - len(text))
            struct.pack_into('<I', data, offset + 4, len(text))
            updated.add(name)
    if updated != {b'TITLE', b'STITLE'}:
        raise ValueError('Missing title fields')
    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vpk', required=True, type=Path)
    parser.add_argument('--cover', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    if args.vpk.resolve() == args.output.resolve():
        parser.error('Use a new output path to preserve the reference build')
    image = Image.open(args.cover).convert('RGB')
    # Crop coordinates describe the original 512x374 North American box scan.
    w, h = image.size
    crop = tuple(round(v * scale) for v, scale in zip(
        (35, 100, 275, 340), (w / 512, h / 374, w / 512, h / 374)))
    replacements = {'sce_sys/icon0.png': png(ImageOps.fit(
        image.crop(crop), (128, 128), method=Image.Resampling.LANCZOS))}
    for name, size in [('bg.png', (840, 500)), ('startup.png', (280, 158))]:
        canvas = Image.new('RGB', size, (13, 41, 63))
        art = ImageOps.contain(image, size, Image.Resampling.LANCZOS)
        canvas.paste(art, ((size[0] - art.width) // 2, (size[1] - art.height) // 2))
        replacements['sce_sys/livearea/contents/' + name] = png(canvas)
    replacements['sce_sys/livearea/contents/template.xml'] = b'''<?xml version="1.0" encoding="utf-8"?>
<livearea style="a1" format-ver="01.00" content-rev="12">
  <livearea-background><image>bg.png</image></livearea-background>
  <gate><startup-image>startup.png</startup-image></gate>
</livearea>
'''
    with zipfile.ZipFile(args.vpk) as original:
        if original.testzip() is not None:
            raise ValueError('Damaged input VPK')
        replacements['sce_sys/param.sfo'] = title_sfo(original.read('sce_sys/param.sfo'))
        if not replacements.keys() <= set(original.namelist()):
            raise ValueError('Input VPK does not contain expected artwork entries')
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(args.output, 'w', zipfile.ZIP_DEFLATED) as result:
            for entry in original.infolist():
                result.writestr(copy.copy(entry), replacements.get(entry.filename, original.read(entry)))
        with zipfile.ZipFile(args.output) as result:
            assert result.testzip() is None
            unchanged = [entry.filename for entry in original.infolist()
                         if entry.filename not in replacements]
            for name in unchanged:
                assert original.read(name) == result.read(name), name
            print(json.dumps({
                'game_executable_unchanged': True,
                'eboot_sha256': hashlib.sha256(result.read('eboot.bin')).hexdigest(),
                'changed_entries': sorted(replacements),
                'vpk_sha256': hashlib.sha256(args.output.read_bytes()).hexdigest(),
            }, indent=2))


if __name__ == '__main__':
    main()
