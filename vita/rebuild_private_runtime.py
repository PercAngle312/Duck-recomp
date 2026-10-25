"""Compile restored private resident/callback sources with VitaSDK hard-float.

Never regenerates or edits source files. Supply an isolated resident generation
with only the three missing exports restored; retain all existing bindings.
The manifest lists private callback source basenames and their SHA-256 hashes.
Optional module/RSP archives expand the audit to the complete translated code.
Outputs remain private, relocatable ARM objects, not a runnable game.
"""
import argparse,concurrent.futures,hashlib,json,re,subprocess
from pathlib import Path
root=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
for name in ('vitasdk','recomp-include','resident-source','private-include','callbacks','output'):
    p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--modules',type=Path);p.add_argument('--audio-rsp',type=Path)
p.add_argument('--jobs',type=int,default=4)
p.add_argument('--threads',action='store_true',help='Include stack-preserving Vita fibers and guest thread callbacks')
p.add_argument('--time',action='store_true',help='Include monotonic timers, initialization and VI retrace service')
a=p.parse_args()
if a.jobs<1 or a.jobs>8:p.error('--jobs must be 1..8')
if bool(a.modules)!=bool(a.audio_rsp):p.error('Pass both --modules and --audio-rsp')
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
sdk=a.vitasdk.resolve()/'bin';cc=sdk/'arm-vita-eabi-gcc';ld=sdk/'arm-vita-eabi-ld';nm=sdk/'arm-vita-eabi-nm'
def run(cmd):
    r=subprocess.run(list(map(str,cmd)),capture_output=True,text=True)
    if r.returncode:raise RuntimeError(r.stderr)
    return r.stdout
def unresolved(path):
    strong=[];weak=[]
    for line in run([nm,'-u',path]).splitlines():
        parts=line.split()
        if not parts:continue
        (strong if parts[0]=='U' else weak).append(parts[-1])
    return dict(strong=sorted(set(strong)),weak=sorted(set(weak)))
manifest=json.loads(a.callbacks.read_text());sources=sorted(a.resident_source.glob('funcs_*.c'))
if not sources:raise ValueError('No regenerated resident sources')
for name,digest in manifest.items():
    if Path(name).name!=name or not name.endswith('.c'):raise ValueError('Unsafe callback basename')
    source=a.callbacks.parent/name
    if hashlib.sha256(source.read_bytes()).hexdigest()!=digest:raise ValueError('Callback checksum mismatch: '+name)
    replaced={'queue_ops.c'}
    if a.time:replaced.update(('count_compare.c','vi_config.c','vi.c'))
    if name not in replaced:sources.append(source)
sources += [root/'src/os_core.c',root/'src/os_io.c',root/'src/os_queue.c']
if a.threads:sources += [root/'src/os_threads.c',root/'src/fiber_vita.c']
if a.time:sources += [root/'src/os_time.c',root/'src/os_vi.c',root/'src/os_vi_config.c',root/'src/clock_vita.c']
flags=['-std=c11','-O1','-mcpu=cortex-a9','-mfpu=neon','-mfloat-abi=hard',
       '-ffunction-sections','-fdata-sections','-ffreestanding','-fno-builtin','-fno-tree-loop-distribute-patterns']
for directory in (a.recomp_include,root/'include',a.private_include,a.resident_source):
    flags+=['-I'+str(directory.resolve())]
def compile(item):
    i,source=item;target=out/(str(i)+'-'+source.stem+'.o')
    run([cc,*flags,'-c',source,'-o',target]);return target
with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:
    objects=list(pool.map(compile,enumerate(sources)))
raw=out/'resident.arm.o';run([ld,'-r',*objects,'-o',raw])
libgcc=run([cc,'-print-libgcc-file-name']).strip()
libm=run([cc,'-print-file-name=libm.a']).strip()
math=out/'resident-with-sdk-math.arm.o';run([ld,'-r',raw,libm,libgcc,'-o',math])
report=dict(compiler=run([cc,'--version']).splitlines()[0],compiled_sources=len(sources),
 float_abi='hard',raw_resident=unresolved(raw),resident_with_sdk_math=unresolved(math),
 full_game_linked=False,playable_game=False,vita_os_tested=False)
report['fiber_runtime_compiled']=a.threads
report['timer_vi_runtime_compiled']=a.time
if a.modules:
    combined=out/'full-code-with-sdk-math.arm.o'
    run([ld,'-r',raw,'--whole-archive',a.modules,a.audio_rsp,'--no-whole-archive',libm,libgcc,'-o',combined])
    report['full_translated_code']=unresolved(combined)
    defined=run([nm,'--defined-only',combined])
    report['module_functions']=sum(bool(re.search(r'\bT duck_m\d',line)) for line in defined.splitlines())
    report['full_code_sha256']=hashlib.sha256(combined.read_bytes()).hexdigest()
(out/'native-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:len(v['strong']) for k,v in report.items() if isinstance(v,dict) and 'strong' in v}))
