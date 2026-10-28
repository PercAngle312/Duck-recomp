"""Build the native first-task renderer replay, NOT the game.

Requires already checked-out dependencies at exact commits in dependencies.json.
Private RAM snapshots and task addresses are supplied separately at runtime.
"""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,tarfile,zipfile
from prepare_renderer import prepare
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--vitasdk',type=Path,required=True)
p.add_argument('--dependencies',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args();here=Path(__file__).resolve().parent;sdk=a.vitasdk.resolve();deps=a.dependencies.resolve();out=a.output.resolve()
if (here.parents[2] in out.parents) or out==here.parents[2]:raise ValueError('Build output must stay outside the public repository')
out.mkdir(parents=True,exist_ok=True);lock=json.loads((here/'dependencies.json').read_text());env={**os.environ,'VITASDK':str(sdk),'PATH':str(sdk/'bin')+os.pathsep+os.environ['PATH']}
def run(args,log,cwd=None):
 with (out/log).open('w') as f:r=subprocess.run(list(map(str,args)),cwd=cwd,env=env,stdout=f,stderr=subprocess.STDOUT)
 if r.returncode:raise RuntimeError(f'{log}: '+(out/log).read_text()[-4000:])
for name,item in lock['repositories'].items():
 directory=deps/name
 sha=subprocess.check_output(['git','-C',str(directory),'rev-parse','HEAD'],text=True).strip()
 if sha!=item['commit']:raise ValueError('Unexpected dependency commit: '+name)
 subprocess.run(['git','-C',str(directory),'diff','--quiet','HEAD','--'],check=True)
tai=out/'taihen.tar.gz';package=lock['taihen_package']
if not tai.exists():run(['curl','-fLsS','--retry','2',package['url'],'-o',tai],'taihen-download.log')
if hashlib.sha256(tai.read_bytes()).hexdigest()!=package['sha256']:raise ValueError('taiHEN package checksum mismatch')
with tarfile.open(tai) as archive:archive.extractall(out/'taihen-sdk',filter='data')
run(['make','-j4'],'mathneon-build.log',deps/'vita-math-neon-reference')
run(['make','-j4','CPPFLAGS=-I../vita-shacc-reference/include'],'vitashark-build.log',deps/'vita-shark-reference')
# Make does not track feature-variable changes in existing objects. Rebuild all
# vitaGL objects so shader serialization and draw-time sampler metadata agree.
run(['make','clean'],'vitagl-clean.log',deps/'vita-gl-reference')
run(['make','-j4','CPPFLAGS=-I../vita-shark-reference/source -I../vita-math-neon-reference/source','NO_SPLASHSCREEN=1','HAVE_GLSL_TEXTURE_SIZE=1'],'vitagl-build.log',deps/'vita-gl-reference')
bin=sdk/'bin';original_src=deps/'vita-renderer-reference/Source'
src=prepare(deps/'vita-renderer-reference',out/'renderer-source');fast=src/'SysVita/HLEGraphics/Accurate'
flags=['-O2','-fno-strict-aliasing','-ffunction-sections','-fdata-sections','-mcpu=cortex-a9','-mfpu=neon','-mfloat-abi=hard']
includes=[here,fast/'Fast3D/include',src,deps/'vita-shark-reference/source',deps/'vita-gl-reference/source',original_src,original_src/'SysVita/Include',original_src/'Config/Dev']
objects=[]
for name,source in [('adapter',here/'adapter.cpp'),('sp',here.parents[1]/'src/sp.c'),('main',here/'main.cpp'),('bridge',fast/'Fast3DMemoryBridge.cpp'),('compat',fast/'Fast3DCompat.cpp'),('interpreter',fast/'Fast3D/src/interpreter.cpp'),('gfx',fast/'Fast3D/src/gfx_opengl.cpp')]:
 obj=out/(name+'.arm.o');run([bin/'arm-vita-eabi-g++','-std=c++17',*flags,'-DDAEDALUS_VITA','-DGBI_FLOATS','-DENABLE_OPENGL','-I'+str(here.parents[1]/'include'),*['-I'+str(x) for x in includes],'-c',source,'-o',obj],name+'-compile.log');objects.append(obj)
shacc=deps/'vita-shacc-reference'
for name,compiler,standard in [('shacccg_ext','arm-vita-eabi-gcc','gnu11'),('shacccg_ext_oop','arm-vita-eabi-g++','gnu++11')]:
 suffix='.c' if name=='shacccg_ext' else '.cpp';obj=out/(name+'.arm.o')
 run([bin/compiler,'-std='+standard,*flags,'-I'+str(shacc/'include'),'-I'+str(out/'taihen-sdk/include'),'-c',shacc/'src'/(name+suffix),'-o',obj],name+'-compile.log');objects.append(obj)
libs=[deps/'vita-gl-reference/libvitaGL.a',deps/'vita-shark-reference/libvitashark.a',deps/'vita-math-neon-reference/libmathneon.a']
stubs=['taihen','SceShaccCg','SceKernelDmacMgr','SceDisplay','SceGxm','SceSysmem','SceCommonDialog','SceKernelThreadMgr','SceAppMgr','SceAppUtil','SceKernelModulemgr','SceLibKernel','SceIofilemgr','SceCtrl','ScePower']
elf=out/'duck-graphics-replay.elf'
run([bin/'arm-vita-eabi-g++','-Wl,--gc-sections','-Wl,-q',*objects,'-Wl,--start-group',*libs,'-L'+str(out/'taihen-sdk/lib'),*['-l'+s+'_stub' for s in stubs],'-lm','-lstdc++','-Wl,--end-group','-o',elf],'executable-link.log')
unresolved=subprocess.check_output([str(bin/'arm-vita-eabi-nm'),'-u',str(elf)],text=True)
if any(line.split()[0]=='U' for line in unresolved.splitlines() if line.split()):raise ValueError('Strong ELF imports remain')
run([bin/'vita-elf-create',elf,out/'replay.velf'],'velf.log')
run([bin/'vita-make-fself','-c',out/'replay.velf',out/'eboot.bin'],'self.log')
run([bin/'vita-mksfoex','-s','TITLE_ID=DUCKVIT03','-s','APP_VER=00.02','Duck Vita Graphics Replay',out/'param.sfo'],'sfo.log')
assets=out/'sce_sys'
for name in ['icon0.png','livearea/contents/bg.png','livearea/contents/startup.png','livearea/contents/template.xml']:
 target=assets/name;target.parent.mkdir(parents=True,exist_ok=True)
 run(['curl','-fLsS','--retry','2','https://raw.githubusercontent.com/vitasdk/samples/'+lock['samples_commit']+'/hello_world/sce_sys/'+name,'-o',target],'sample-'+target.name+'.log')
vpk=out/'Duck-Vita-Graphics-Replay.vpk'
run([bin/'vita-pack-vpk','-s',out/'param.sfo','-b',out/'eboot.bin','-a',str(assets)+'=sce_sys',vpk],'vpk.log')
with zipfile.ZipFile(vpk) as z:
 if z.testzip() is not None:raise ValueError('Corrupt VPK')
report={'vpk_sha256':hashlib.sha256(vpk.read_bytes()).hexdigest(),'vpk_bytes':vpk.stat().st_size,'native_elf_linked':True,'strong_unresolved_symbols':0,'weak_unresolved':unresolved.splitlines(),'executed':False,'rendered_output_verified':False,'gameplay_tested':False,'playable_game':False,'private_assets_in_vpk':False}
(out/'build-report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
