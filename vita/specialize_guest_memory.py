"""Specialize explicitly mapped private guest utilities; preserve original tails.

Mapping entries: {symbol, source, sha256, object_sha256, label, kernel}. Hashes cover the entire
original generated function. Outputs and mapping stay outside the public repo.
"""
import argparse,pathlib,json,hashlib,re,subprocess
p=argparse.ArgumentParser(description=__doc__)
for name in ['source-root','mapping','original-object','vitasdk','headers','output']:p.add_argument('--'+name,type=pathlib.Path,required=True)
a=p.parse_args();out=a.output.resolve();here=pathlib.Path(__file__).resolve().parent
if here.parent==out or here.parent in out.parents:raise ValueError('Private output must be outside public checkout')
out.mkdir(parents=True,exist_ok=True);entries=json.loads(a.mapping.read_text());bodies=[];symbols=[]
object_hash=hashlib.sha256(a.original_object.read_bytes()).hexdigest()
if not entries or any(e.get('object_sha256')!=object_hash for e in entries):raise ValueError('Private game object hash mismatch')
for e in entries:
 if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*',e['symbol']) or e['kernel'] not in ['zero','copy','fill']:raise ValueError('Invalid utility mapping')
 s=(a.source_root/e['source']).read_text();start=s.index('RECOMP_FUNC void '+e['symbol']+'(');end=s.index('\n;}',start)+3;body=s[start:end]
 if hashlib.sha256(body.encode()).hexdigest()!=e['sha256']:raise ValueError('Private function hash mismatch')
 anchor='\n'+e['label']+':\n'
 if body.count(anchor)!=1:raise ValueError('Private loop anchor mismatch')
 body=body.replace(anchor,anchor+'    duck_vita_'+e['kernel']+'_bulk(rdram,ctx);\n');bodies.append(body);symbols.append(e['symbol'])
from instrument_checkpoints import instrument
source=out/'native-utilities.private.c';text='#include "recomp.h"\n#include "duck_vita_native_memory.h"\n'+'\n'.join(bodies)+'\n';text,_,_=instrument(text);source.write_text(text)
sdk=a.vitasdk.resolve()/'bin';original=out/'game-only.arm.o';cmd=[sdk/'arm-vita-eabi-objcopy',*['--weaken-symbol='+s for s in symbols],a.original_object.resolve(),original];subprocess.run(list(map(str,cmd)),check=True)
cmd=[sdk/'arm-vita-eabi-gcc','-std=c11','-O2','-fno-strict-aliasing','-ffunction-sections','-fdata-sections','-mcpu=cortex-a9','-mfpu=neon','-mfloat-abi=hard','-I'+str(a.headers.resolve()),'-I'+str(here/'include'),'-c',source,'-o',out/'native-utilities.arm.o'];subprocess.run(list(map(str,cmd)),check=True)
print(json.dumps({'functions':len(symbols),'mapping_verified':True,'private_object':str(original)}))
