"""Copy pinned concurrentqueue headers and add the Vita semaphore branch.

Upstream files remain untouched. Put output/concurrentqueue ahead of the
upstream concurrentqueue directory in the Vita runtime's include path.
"""
import argparse
import json
import subprocess
from pathlib import Path

def prepare(runtime, output):
    lock = json.loads((Path(__file__).parent / 'toolchain-lock.json').read_text())
    actual = subprocess.check_output(['git', '-C', str(runtime), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != lock['N64ModernRuntime_commit']:
        raise ValueError('Runtime does not match pinned commit')
    destination = output / 'concurrentqueue'
    destination.mkdir(parents=True, exist_ok=True)
    for source in (runtime / 'thirdparty/concurrentqueue').glob('*.h'):
        data = source.read_text()
        if source.name == 'lightweightsemaphore.h':
            include = '#include <cstddef> // For std::size_t'
            branch = '#if defined(_WIN32)\nclass Semaphore'
            if data.count(include) != 1 or data.count(branch) != 1:
                raise ValueError('Unexpected upstream semaphore header')
            data = '// Altered copy: Duck Recomp adds a Vita backend.\n' + data
            data = data.replace(include, '#if defined(__vita__)\n#include "vita_semaphore.hpp"\n#endif\n' + include)
            data = data.replace(branch, '#if defined(__vita__)\nusing Semaphore = duck_vita::Semaphore;\n#elif defined(_WIN32)\nclass Semaphore')
        target = destination / source.name
        if not target.exists() or target.read_text() != data:
            target.write_text(data)
    return destination

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(prepare(args.runtime.resolve(), args.output.resolve()))
