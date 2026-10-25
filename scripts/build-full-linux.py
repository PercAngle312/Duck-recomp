"""Build the actual Linux game from private generated inputs and pinned sources."""
import argparse
import os
from pathlib import Path
import subprocess

PINS = {
    'recompiler': 'ffb39cdad1da5de07eaaa48bd1db4a89a7986771',
    'runtime': 'cdf5abbd5026fef5c364c676e4667c45e42b6863',
    'rt64': 'd9817c68ffd7328cd0af383af124971489366e18',
}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('game-source', 'recompiler', 'runtime', 'rt64', 'build'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--graphics-sysroot', type=Path)
    p.add_argument('--rt64-build', type=Path)
    p.add_argument('--jobs', type=int, default=2)
    p.add_argument('--configure-only', action='store_true')
    a = p.parse_args()
    if a.jobs < 1:
        p.error('--jobs must be positive')
    for name, pin in PINS.items():
        source = getattr(a, name).resolve()
        actual = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
        if actual != pin:
            p.error(f'{name} must be pinned to {pin}; found {actual}')
    env = os.environ.copy()
    flags = []
    if a.graphics_sysroot:
        sr = a.graphics_sysroot.resolve()
        env['PKG_CONFIG_SYSROOT_DIR'] = str(sr)
        env['PKG_CONFIG_LIBDIR'] = ':'.join(str(sr / path) for path in (
            'usr/lib/x86_64-linux-gnu/pkgconfig', 'usr/share/pkgconfig'))
        env['LD_LIBRARY_PATH'] = str(sr / 'usr/lib/x86_64-linux-gnu') + ':' + env.get('LD_LIBRARY_PATH', '')
        flags += [f'-DCMAKE_PREFIX_PATH={sr}/usr',
                  f'-DCMAKE_C_FLAGS=-I{sr}/usr/include/x86_64-linux-gnu',
                  f'-DCMAKE_CXX_FLAGS=-I{sr}/usr/include/x86_64-linux-gnu']
        pkgconf = sr / 'usr/bin/pkgconf'
        if pkgconf.exists():
            flags.append(f'-DPKG_CONFIG_EXECUTABLE={pkgconf}')
    if a.rt64_build:
        flags.append(f'-DDUCK_RT64_BINARY_DIR={a.rt64_build.resolve()}')
    recipe = Path(__file__).resolve().parents[1] / 'cmake/FullLinuxGame'
    build = a.build.resolve()
    configure = ['cmake', '-S', str(recipe), '-B', str(build), '-DCMAKE_BUILD_TYPE=Release',
                 f'-DDUCK_GAME_SOURCE={a.game_source.resolve()}',
                 f'-DN64RECOMP_ROOT={a.recompiler.resolve()}',
                 f'-DN64MODERN_ROOT={a.runtime.resolve()}',
                 f'-DDUCK_RT64_ROOT={a.rt64.resolve()}', *flags]
    subprocess.run(configure, env=env, check=True)
    if a.configure_only:
        return
    subprocess.run(['cmake', '--build', str(build), '--target', 'duckdodgers', 'duck_memory_alias',
                    '--parallel', str(a.jobs)], env=env, check=True)
    subprocess.run(['ctest', '--test-dir', str(build), '--output-on-failure'], env=env, check=True)
    print(f'Linked full Linux executable: {build / "duckdodgers"}. Gameplay requires separate validation.')


if __name__ == '__main__':
    main()
