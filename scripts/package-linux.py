"""Stage a ROM-free Linux release using an explicitly supplied dependency bundle."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('binary', 'runtime-libs', 'runtime-licenses', 'project-licenses', 'rt64', 'output'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--software-driver', type=Path)
    p.add_argument('--software-icd', type=Path)
    p.add_argument('--validation', type=Path, required=True)
    a = p.parse_args()
    if bool(a.software_driver) != bool(a.software_icd):
        p.error('Supply both --software-driver and --software-icd')
    # An executable is mandatory; never package the repository preflight stubs.
    if a.binary.name != 'duckdodgers' or a.binary.read_bytes()[:4] != b'\x7fELF':
        p.error('--binary must be the actual Linux duckdodgers ELF executable')
    root = a.output.resolve()
    if root.exists():
        p.error('Output directory already exists; preserve previous releases')
    (root / 'lib').mkdir(parents=True)
    (root / 'roms').mkdir()
    shutil.copy2(a.binary, root / 'duckdodgers')
    (root / 'duckdodgers').chmod(0o755)
    script_root = Path(__file__).resolve().parent
    shutil.copy2(script_root / 'run-linux.sh', root / 'run.sh')
    (root / 'run.sh').chmod(0o755)
    for library in sorted(a.runtime_libs.glob('*.so*')):
        if library.is_file():
            shutil.copy2(library, root / 'lib' / library.name)
    for name in ('libdxcompiler.so', 'libdxil.so'):
        shutil.copy2(a.rt64 / 'src/contrib/dxc/lib/x64' / name, root / 'lib' / name)
    shutil.copytree(a.runtime_licenses, root / 'licenses/runtime')
    shutil.copytree(a.project_licenses, root / 'licenses/project')
    # Preserve all supplied dependency notices, including nested third parties.
    for source in a.rt64.rglob('*'):
        if source.is_file() and source.name.lower().startswith(('license', 'copying', 'notice')) and '.git' not in source.parts and 'build' not in source.parts:
            target = root / 'licenses/rt64' / source.relative_to(a.rt64)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
    if a.software_driver:
        shutil.copy2(a.software_driver, root / 'lib' / a.software_driver.name)
        target = root / 'share/vulkan/icd.d/lvp_icd.json'
        target.parent.mkdir(parents=True)
        icd = json.loads(a.software_icd.read_text())
        icd['ICD']['library_path'] = '../../../lib/' + a.software_driver.name
        target.write_text(json.dumps(icd, indent=2) + '\n')
    shutil.copy2(a.validation, root / 'validation.json')
    (root / 'roms/PLACE-YOUR-ROM-HERE.txt').write_text('Supply your own USA multilingual 16 MiB big-endian Duck Dodgers .z64. No ROM is included.\n')
    (root / 'README.md').write_text('''# Duck Dodgers — Linux native build

Extract the archive, place your supported USA multilingual `.z64` in `roms/`,
then run `chmod +x run.sh duckdodgers && ./run.sh`.
You may also run `./run.sh /path/to/your/game.z64`.

Requires Linux x86_64, SSE4.1, glibc 2.39 or later (Ubuntu 24.04 baseline),
an X11/Wayland desktop and a Vulkan-capable graphics driver. Shared runtime
libraries and shader compiler are bundled. A developer toolchain is not required.
`DUCK_SOFTWARE_VULKAN=1 ./run.sh` selects bundled Lavapipe when included.

WASD: move; J: A/jump/accept; K: B/kick; Enter: Start/pause; Space: Z;
Q/E: L/R; I/P/U/O: C up/down/left/right; arrows: D-pad.
Close the window to exit cleanly and flush saves.

Saves, settings and the runtime's local ROM cache are written to `userdata/`.
Set `DUCK_USER_DATA=/your/folder` to choose another location. Preserve that folder
when upgrading. Earlier reference builds used `private/runtime-probe/`; copy its
contents into `userdata/` before first launch if you want to retain those saves.

Read `validation.json` for actual tests and remaining limits. A working opening
stage is not a full campaign certification. Physical GPU/audio/controller and
cross-distro tests require separate validation. No proprietary ROM is bundled.
''')
    # Include enough identity to match test results to this exact executable.
    digest = hashlib.sha256((root / 'duckdodgers').read_bytes()).hexdigest()
    (root / 'BUILD.json').write_text(json.dumps({'executable_sha256': digest,
        'rt64_commit': subprocess.check_output(['git', '-C', str(a.rt64), 'rev-parse', 'HEAD'], text=True).strip(),
        'minimum_glibc': '2.39', 'rom_included': False}, indent=2) + '\n')
    files = sorted(path for path in root.rglob('*') if path.is_file())
    (root / 'SHA256SUMS').write_text(''.join(hashlib.sha256(path.read_bytes()).hexdigest() + '  ' + str(path.relative_to(root)) + '\n' for path in files))
    archive = root.with_suffix('.zip')
    if archive.exists():
        p.error('Archive already exists; preserve previous releases')
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for path in sorted(root.rglob('*')):
            if path.is_file():
                z.write(path, root.name + '/' + str(path.relative_to(root)))
    with zipfile.ZipFile(archive) as z:
        if z.testzip():
            raise RuntimeError('Archive CRC verification failed')
    print(archive)


if __name__ == '__main__':
    main()
