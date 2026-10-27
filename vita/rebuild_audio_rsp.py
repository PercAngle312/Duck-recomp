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
a=p.parse_args();here=Path(__file__).resolve().parent;out=a.output.resolve()
if out==here.parent or here.parent in out.parents:p.error('Private outputs must remain outside the public repository')
for path in [a.source,a.headers/'recomp.h',a.headers/'librecomp/rsp_vu.hpp',a.headers/'librecomp/rsp_vu_impl.hpp',a.modern_runtime/'thirdparty/sse2neon/sse2neon.h']:
 if not path.is_file():p.error('Missing private/source dependency: '+str(path))
if a.headers.resolve()==out or a.headers.resolve() in out.parents:p.error('Output must not be inside input headers')
out.mkdir(parents=True,exist_ok=True);headers=out/'include';shutil.copytree(a.headers,headers,dirs_exist_ok=True)
if a.mode=='neon':
 header=headers/'librecomp/rsp_vu.hpp';text=header.read_text();anchor='#elif defined(__aarch64__) || defined(_M_ARM64)'
 if text.count(anchor)!=1:p.error('Unrecognized pinned RSP vector header')
 text=text.replace(anchor,anchor+' || defined(__ARM_NEON)').replace('#include <sse2neon.h>','#include "duck_vita_sse2neon.h"');header.write_text(text)
sdk=a.vitasdk.resolve()/'bin';obj=out/'audio_rsp.arm.o';archive=out/'libduck_vita_audio_rsp.a'
cmd=[sdk/'arm-vita-eabi-g++','-std=c++20','-O2','-fwrapv','-fno-strict-aliasing','-ffunction-sections','-fdata-sections','-mcpu=cortex-a9','-mfpu=neon','-mfloat-abi=hard','-I'+str(headers),'-I'+str(here/'include'),'-I'+str(a.modern_runtime.resolve()/'ultramodern/include'),'-I'+str(a.modern_runtime.resolve()/'thirdparty/sse2neon'),'-c',a.source.resolve(),'-o',obj]
with (out/'compile.log').open('w') as f:r=subprocess.run(list(map(str,cmd)),stdout=f,stderr=subprocess.STDOUT)
if r.returncode:raise SystemExit((out/'compile.log').read_text()[-3000:])
# Use a new archive, retaining any previous archive under a unique checkpoint name.
if archive.exists():archive.rename(out/('previous-'+hashlib.sha256(archive.read_bytes()).hexdigest()+'.a'))
subprocess.run([sdk/'arm-vita-eabi-ar','rcs',archive,obj],check=True)
report={'mode':a.mode,'source_sha256':hashlib.sha256(a.source.read_bytes()).hexdigest(),'original_vector_header_sha256':hashlib.sha256((a.headers/'librecomp/rsp_vu.hpp').read_bytes()).hexdigest(),'sse2neon_sha256':hashlib.sha256((a.modern_runtime/'thirdparty/sse2neon/sse2neon.h').read_bytes()).hexdigest(),'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'float_abi':'hard','command':list(map(str,cmd)),'generated_source_modified':False,'hardware_tested':False}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
