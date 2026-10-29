"""Patch pinned vitaGL gxm.o, retaining the existing library's other objects.

Reject a different baseline by comparing original gxm .text before replacement.
Output must be outside all source checkouts. Does not modify the input library.
"""
import argparse,pathlib,subprocess,shutil
p=argparse.ArgumentParser(description=__doc__)
for name in ('vitagl','vitashark','mathneon','vitasdk','baseline','output'):
 p.add_argument('--'+name,type=pathlib.Path,required=True)
a=p.parse_args();here=pathlib.Path(__file__).resolve().parent;out=a.output.resolve()
for source in (a.vitagl.resolve(),a.vitashark.resolve(),a.mathneon.resolve(),here.parents[2]):
 if out==source or source in out.parents or out in source.parents:raise ValueError('Output must be outside source checkouts')
out.mkdir(parents=True,exist_ok=True)
def run(cmd,cwd=None):subprocess.run(list(map(str,cmd)),cwd=cwd,check=True)
sdk=a.vitasdk/'bin';copy=out/'vitagl';shutil.copytree(a.vitagl/'source',copy/'source',dirs_exist_ok=True)
flags=['-g','-O3','-ffast-math','-mtune=cortex-a9','-mfpu=neon','-mfloat-abi=hard','-mfp16-format=ieee','-DSKIP_SPLASHSCREEN','-DVGL_GIT_HASH="76f43e6"','-Wno-incompatible-pointer-types']
def compile(destination):
 run([sdk/'arm-vita-eabi-gcc',*flags,*['-I'+str(x) for x in (copy/'source',a.vitashark/'source',a.mathneon/'source')],'-c',copy/'source/gxm.c','-o',destination])
reference=out/'reference';reference.mkdir(exist_ok=True)
run([sdk/'arm-vita-eabi-ar','x',a.baseline.resolve(),'gxm.o'],cwd=reference)
compile(out/'unpatched.o')
for source,target in ((reference/'gxm.o',out/'baseline.text'),(out/'unpatched.o',out/'unpatched.text')):
 run([sdk/'arm-vita-eabi-objcopy','-O','binary','--only-section=.text',source,target])
if (out/'baseline.text').read_bytes()!=(out/'unpatched.text').read_bytes():raise RuntimeError('Pinned vitaGL baseline .text mismatch; refusing archive replacement')
run(['git','apply','--check',here/'vitagl-readback-fence.patch'],cwd=copy)
run(['git','apply',here/'vitagl-readback-fence.patch'],cwd=copy)
shutil.copy2(here/'framebuffer_fence.h',copy/'source/framebuffer_fence.h')
compile(out/'gxm.o');library=out/'libvitaGL-fenced.a';shutil.copy2(a.baseline,library)
run([sdk/'arm-vita-eabi-ar','r',library,out/'gxm.o']);print(library)
