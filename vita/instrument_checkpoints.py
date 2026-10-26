"""Copy private N64Recomp sources with cooperative guest checkpoints.

Original files remain read-only. Run before compiling both resident and module
code. Every generated function entry and every direct backward-goto target is
covered; callbacks run later on the host scheduler, never inside the checkpoint.
Generated output is proprietary-derived and must remain outside this repository.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil

MARKER = '/* DUCK_VITA_GUEST_CHECKPOINTS_V1 */'
FUNCTION = re.compile(r'^RECOMP_FUNC\s+void\s+\w+\s*\(\s*uint8_t\s*\*\s*rdram\s*,\s*recomp_context\s*\*\s*ctx\s*\)\s*\{', re.M)
TOKENS = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)

def instrument(source):
    if MARKER in source:
        raise ValueError('Source already contains guest checkpoints')
    # Keep offsets while excluding labels/gotos in comments and string literals.
    code = TOKENS.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), source)
    functions = list(FUNCTION.finditer(code))
    if len(functions) != len(re.findall(r'^RECOMP_FUNC\b', code, re.M)):
        raise ValueError('Unsupported generated function signature')
    if not functions:
        raise ValueError('No generated functions found')
    edits = []
    backward_targets = 0
    for i, function in enumerate(functions):
        end = functions[i+1].start() if i+1 < len(functions) else len(code)
        body = code[function.end():end]
        labels = {}
        for label in re.finditer(r'^[ \t]*([A-Za-z_]\w*):[ \t]*$', body, re.M):
            if label[1] in labels:
                raise ValueError('Duplicate generated label')
            labels[label[1]] = label.end()
        targets = set()
        for jump in re.finditer(r'\bgoto\s+([A-Za-z_]\w*)\s*;', body):
            if jump[1] not in labels:
                raise ValueError('Missing generated goto target')
            if labels[jump[1]] <= jump.start():
                targets.add(labels[jump[1]])
        edits.append(function.end())
        edits.extend(function.end()+position for position in targets)
        backward_targets += len(targets)
    for position in sorted(edits, reverse=True):
        source = source[:position]+'\n    duck_vita_thread_checkpoint(rdram, ctx);'+source[position:]
    return MARKER+'\n#include "duck_vita_threads.h"\n'+source, len(functions), backward_targets

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    original, destination = args.source.resolve(), args.output.resolve()
    repo = Path(__file__).resolve().parent.parent
    if original == destination or original in destination.parents or destination in original.parents:
        raise ValueError('Output must be separate from original source')
    if repo == destination or repo in destination.parents:
        raise ValueError('Private generated output must stay outside public repository')
    sources = sorted(original.glob('funcs_*.c'))
    if not sources:
        raise ValueError('No generated source chunks')
    # Validate every chunk before writing any output.
    prepared = [(p, instrument(p.read_text())) for p in sources]
    destination.mkdir(parents=True, exist_ok=True)
    for header in original.glob('*.h'):
        shutil.copy2(header, destination/header.name)
    report = {'original_sources_modified': False, 'chunks': len(prepared), 'functions': 0, 'backward_targets': 0, 'files': []}
    for path, (text, functions, targets) in prepared:
        (destination/path.name).write_text(text)
        report['functions'] += functions
        report['backward_targets'] += targets
        report['files'].append({'name': path.name, 'functions': functions, 'backward_targets': targets, 'original_sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    (destination/'checkpoint-report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k != 'files'}))

if __name__ == '__main__':
    main()
