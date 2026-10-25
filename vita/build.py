"""Cross-compile private game CPU archives and the explicitly non-playable Vita probe."""
import argparse,hashlib,json,os,re,subprocess,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
for name in ('vitasdk','game-source','recompiler','runtime','samples','build'):
 p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--jobs',type=int,default=4)
a=p.parse_args();lock=json.loads((ROOT/'toolchain-lock.json').read_text())
if a.jobs<1:p.error('--jobs must be positive')
for name,key in [('recompiler','N64Recomp_commit'),('runtime','N64ModernRuntime_commit'),('samples','samples_commit')]:
 actual=subprocess.check_output(['git','-C',str(getattr(a,name)),'rev-parse','HEAD'],text=True).strip()
 if actual!=lock[key]:p.error(f'{name} must match pinned commit {lock[key]}')
env=dict(os.environ,VITASDK=str(a.vitasdk.resolve()))
env['PATH']=str(a.vitasdk.resolve()/'bin')+os.pathsep+env.get('PATH','')
build=a.build.resolve()
subprocess.run(['cmake','-S',str(ROOT),'-B',str(build),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',
 f'-DDUCK_GAME_SOURCE={a.game_source.resolve()}',f'-DN64RECOMP_ROOT={a.recompiler.resolve()}',
 f'-DN64MODERN_ROOT={a.runtime.resolve()}',f'-DVITA_SAMPLES_ROOT={a.samples.resolve()}'],env=env,check=True)
subprocess.run(['cmake','--build',str(build),'--parallel',str(a.jobs)],env=env,check=True)
artifacts=['duck_vita_probe','duck_vita_probe.self','DuckDodgers-Vita-Bringup.vpk',
 'libduck_vita_modules.a','libduck_vita_resident.a','libduck_vita_audio_rsp.a']
record={name:{'bytes':(build/name).stat().st_size,'sha256':hashlib.sha256((build/name).read_bytes()).hexdigest()} for name in artifacts}
with zipfile.ZipFile(build/artifacts[2]) as z:
 assert z.testzip() is None
 assert {'eboot.bin','sce_sys/param.sfo','sce_sys/icon0.png','sce_sys/livearea/contents/template.xml'}.issubset(z.namelist())
 assert not any(n.lower().endswith(('.z64','.n64','.v64','.sav','.eep')) for n in z.namelist())
nm=a.vitasdk.resolve()/'bin/arm-vita-eabi-nm'
symbols=subprocess.check_output([str(nm),'--defined-only',str(build/'libduck_vita_modules.a')],text=True)
resident=subprocess.check_output([str(nm),'--defined-only',str(build/'libduck_vita_resident.a')],text=True)
report={'artifacts':record,'module_functions_compiled':len(re.findall(r' T duck_m',symbols)),
 'resident_functions_compiled':len(re.findall(r' T (?:func_|recomp_)',resident)),
 'compiler':subprocess.check_output([str(a.vitasdk.resolve()/'bin/arm-vita-eabi-gcc'),'--version'],text=True).splitlines()[0],
 'full_game_linked':False,'playable_game':False,'vita_hardware_tested':False,'audio_rsp_compiled':'portable scalar ARM32',
 'toolchain':lock}
(build/'compile-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'module_functions_compiled':report['module_functions_compiled'],'resident_functions_compiled':report['resident_functions_compiled'],'probe_vpk':str(build/artifacts[2]),'playable_game':False}))
