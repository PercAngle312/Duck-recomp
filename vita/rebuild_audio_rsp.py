"""Compile private translated audio RSP code with the pinned VitaSDK.

Source stays private. Scalar and ARM32 NEON use the same generated function and
RSP implementation; NEON enables the upstream SIMD path through SSE2NEON. This
is not a gameplay test. Validate optimized output against a known-good build.
"""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess
p=argparse.ArgumentParser(description=__doc__)
for name in ['source','headers','modern-runtime','vitasdk','output']:
 p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--mode',choices=['scalar','neon'],default='neon')
p.add_argument('--fast-memory',action='store_true',help='Enable validated aligned RSP memory paths in private header copies')
p.add_argument('--native-multiply',action='store_true',help='Enable ARM32 fractional multiply paths; requires NEON mode and output validation')
p.add_argument('--native-high-multiply',action='store_true',help='Enable VMUDH/VMADH widened NEON paths; requires --native-multiply and complete-task validation')
p.add_argument('--native-mixed-multiply',action='store_true',help='Enable validated native ARM mixed multiply kernels')
p.add_argument('--native-partial-loads',action='store_true',help='Enable exact NEON LQV<0>/LRV<0> loads')
p.add_argument('--native-commands',action='store_true',help='Specialize validated clear/move/interleave/mixer command prefixes')
p.add_argument('--native-samples',action='store_true',help='Native unaligned sample windows and constant vector selectors')
a=p.parse_args();here=Path(__file__).resolve().parent;out=a.output.resolve()
if a.native_samples and a.mode!='neon':p.error('--native-samples requires NEON')
if a.native_commands and a.mode!='neon':p.error('--native-commands requires NEON')
if a.native_partial_loads and a.mode!='neon':p.error('--native-partial-loads requires NEON')
if a.native_mixed_multiply and a.mode!='neon':p.error('--native-mixed-multiply requires NEON')
if a.native_multiply and a.mode!='neon':p.error('--native-multiply requires --mode neon')
if a.native_high_multiply and not a.native_multiply:p.error('--native-high-multiply requires --native-multiply')
if out==here.parent or here.parent in out.parents:p.error('Private outputs must remain outside the public repository')
for path in [a.source,a.headers/'recomp.h',a.headers/'librecomp/rsp_vu.hpp',a.headers/'librecomp/rsp_vu_impl.hpp',a.modern_runtime/'thirdparty/sse2neon/sse2neon.h']:
 if not path.is_file():p.error('Missing private/source dependency: '+str(path))
if a.headers.resolve()==out or a.headers.resolve() in out.parents:p.error('Output must not be inside input headers')
out.mkdir(parents=True,exist_ok=True);headers=out/'include';shutil.copytree(a.headers,headers,dirs_exist_ok=True)
if a.mode=='neon':
 header=headers/'librecomp/rsp_vu.hpp';text=header.read_text();anchor='#elif defined(__aarch64__) || defined(_M_ARM64)'
 if text.count(anchor)!=1:p.error('Unrecognized pinned RSP vector header')
 text=text.replace(anchor,anchor+' || defined(__ARM_NEON)').replace('#include <sse2neon.h>','#include "duck_vita_sse2neon.h"');header.write_text(text)
if a.fast_memory:
 from patch_rsp_memory import patch
 patch(headers)
if a.native_multiply:
 from patch_rsp_multiply import patch
 patch(headers, high=a.native_high_multiply)
if a.native_mixed_multiply:
 from patch_rsp_mixed import patch
 patch(headers)
if a.native_partial_loads:
 from patch_rsp_partial import patch
 patch(headers)
sample_sites=0
if a.native_samples:
 from patch_rsp_samples import patch
 sample_sites=patch(headers)
compiled_source=a.source.resolve()
if a.native_commands:
 from patch_audio_commands import patch
 compiled_source=out/'audio_rsp.commands.private.cpp';patch(a.source,compiled_source)
sdk=a.vitasdk.resolve()/'bin';obj=out/'audio_rsp.arm.o';archive=out/'libduck_vita_audio_rsp.a'
cmd=[sdk/'arm-vita-eabi-g++','-std=c++20','-O2','-fwrapv','-fno-strict-aliasing','-ffunction-sections','-fdata-sections','-mcpu=cortex-a9','-mfpu=neon','-mfloat-abi=hard','-I'+str(headers),'-I'+str(here/'include'),'-I'+str(a.modern_runtime.resolve()/'ultramodern/include'),'-I'+str(a.modern_runtime.resolve()/'thirdparty/sse2neon'),'-c',compiled_source,'-o',obj]
with (out/'compile.log').open('w') as f:r=subprocess.run(list(map(str,cmd)),stdout=f,stderr=subprocess.STDOUT)
if r.returncode:raise SystemExit((out/'compile.log').read_text()[-3000:])
# Use a new archive, retaining any previous archive under a unique checkpoint name.
if archive.exists():archive.rename(out/('previous-'+hashlib.sha256(archive.read_bytes()).hexdigest()+'.a'))
subprocess.run([sdk/'arm-vita-eabi-ar','rcs',archive,obj],check=True)
report={'mode':a.mode,'fast_memory':a.fast_memory,'source_sha256':hashlib.sha256(a.source.read_bytes()).hexdigest(),'original_vector_header_sha256':hashlib.sha256((a.headers/'librecomp/rsp_vu.hpp').read_bytes()).hexdigest(),'sse2neon_sha256':hashlib.sha256((a.modern_runtime/'thirdparty/sse2neon/sse2neon.h').read_bytes()).hexdigest(),'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'float_abi':'hard','command':list(map(str,cmd)),'generated_source_modified':a.native_commands,'hardware_tested':False}
if a.fast_memory:
 report['fast_memory_helper_sha256']=hashlib.sha256((here/'include/duck_vita_rsp_memory.h').read_bytes()).hexdigest()
 report['patched_headers_sha256']={n:hashlib.sha256((headers/'librecomp'/n).read_bytes()).hexdigest() for n in ['rsp.hpp','rsp_vu.hpp','rsp_vu_impl.hpp']}
report['native_samples']=a.native_samples
report['native_element_sites']=sample_sites
report['native_commands']=a.native_commands
if a.native_commands:
 report['native_commands_helper_sha256']=hashlib.sha256((here/'include/duck_vita_audio_commands.h').read_bytes()).hexdigest()
report['native_multiply']=a.native_multiply
if a.native_multiply or a.native_mixed_multiply:
 report['native_accumulator_helper_sha256']=hashlib.sha256((here/'include/duck_vita_rsp_accumulator.h').read_bytes()).hexdigest()
report['native_high_multiply']=a.native_high_multiply
report['native_partial_loads']=a.native_partial_loads
if a.native_partial_loads:
 report['native_partial_helper_sha256']=hashlib.sha256((here/'include/duck_vita_rsp_partial.h').read_bytes()).hexdigest()
report['native_mixed_multiply']=a.native_mixed_multiply
if a.native_mixed_multiply:
 report['native_mixed_helper_sha256']=hashlib.sha256((here/'include/duck_vita_rsp_mixed.h').read_bytes()).hexdigest()
 report['mixed_header_sha256']=hashlib.sha256((headers/'librecomp/rsp_vu_impl.hpp').read_bytes()).hexdigest()
if a.native_multiply:
 report['native_multiply_helper_sha256']=hashlib.sha256((here/'include/duck_vita_rsp_multiply.h').read_bytes()).hexdigest()
 report['multiply_header_sha256']=hashlib.sha256((headers/'librecomp/rsp_vu_impl.hpp').read_bytes()).hexdigest()
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
