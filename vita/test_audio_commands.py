from pathlib import Path
import subprocess,struct,json,sys,random
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import *
import argparse
p=argparse.ArgumentParser(description='Compare complete pinned DSP tasks, including hidden vector/scalar state')
for name in ['source','headers','modern-runtime','vitasdk','output']:p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);sdk=a.vitasdk.resolve()/'bin';repo=Path(__file__).resolve().parent
if out==repo.parent or repo.parent in out.parents:raise ValueError('Private test output must be outside public checkout')
from patch_audio_commands import patch
candidate=out/'commands.private.cpp';patch(a.source,candidate)
objects=[]
for label,source in [('reference',a.source),('candidate',candidate)]:
 text=source.read_text()
 capture='{uint32_t registers[]={'+','.join('r'+str(i) for i in range(1,32))+'};duck_test_capture(&rsp.vpu,registers);return RspExitReason::Broke;}'
 text=text.replace('return RspExitReason::Broke;',capture)
 if label=='reference':text=text.replace('duck_audio_rsp(', 'reference_audio_rsp(')
 instrumented=out/(label+'.private.cpp');instrumented.write_text('#include <cstdint>\nextern "C" void duck_test_capture(const void*,const uint32_t*);\n'+text)
 obj=out/(label+'.o');objects.append(obj)
 subprocess.run([str(sdk/'arm-vita-eabi-g++'),'-std=c++20','-O2','-fwrapv','-fno-strict-aliasing','-ffunction-sections','-fdata-sections','-mcpu=cortex-a9','-mfpu=neon','-mfloat-abi=hard','-I'+str(a.headers.resolve()),'-I'+str(repo/'include'),'-I'+str(a.modern_runtime.resolve()/'thirdparty/sse2neon'),'-I'+str(a.modern_runtime.resolve()/'ultramodern/include'),'-c',str(instrumented),'-o',str(obj)],check=True)
elf=out/'commands-state-pair.elf'
subprocess.run([str(sdk/'arm-vita-eabi-g++'),'-std=c++20','-O2','-ffreestanding','-fno-builtin','-mcpu=cortex-a9','-mfpu=neon','-mfloat-abi=hard','-nostdlib','-Wl,-Ttext=0x10000','-Wl,-e,_Z14duck_audio_rspPhj',*[str(x) for x in objects],str(repo/'tests/audio_command_shim.cpp'),'-lgcc','-o',str(elf)],check=True)
symbols={}
for line in subprocess.check_output([str(sdk/'arm-vita-eabi-nm'),str(elf)],text=True).splitlines():
 p=line.split()
 if len(p)==3:symbols[p[2]]=int(p[0],16)
data=elf.read_bytes();u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);u.mem_map(0x10000,0x800000);u.mem_map(0x1000000,0x800000)
ph=struct.unpack_from('<I',data,28)[0];es,n=struct.unpack_from('<HH',data,42)
for i in range(n):
 kind,off,addr,_,size,mem,_,_=struct.unpack_from('<8I',data,ph+i*es)
 if kind==1:u.mem_write(addr,data[off:off+size])
u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
rng=random.Random(0xd028);count=0
for seed in range(96):
 for kind in range(5):
  ram=bytearray(0x800000);dm=bytearray(rng.randbytes(4096));source=0x50000;dest=0x60000;command=0x40000
  ram[source:source+4096]=rng.randbytes(4096)
  # Synthetic jump table explicitly selects the same pinned DSP handlers.
  for op,target in {2:0x119c,4:0x11c8,6:0x1208,10:0x1348,12:0x1c84,13:0x12d4}.items():struct.pack_into('<H',dm,(op*2)^2,target)
  factor=[-32768,-32767,-1,0,1,16384,32767][seed%7];struct.pack_into('<h',dm,(0x50+10)^2,factor)
  length=[0,1,15,16,17,31,32,64,128,368,512,736][seed%12]
  save=lambda offset,n:(0x06000000|(n<<12)|offset,dest)
  if kind==0:commands=[(0x02000000|((seed%8)*4),length),save(0,768)]
  elif kind==1:
   src=(seed%8)*4;dst=([0,4,8,16,32,512][seed%6] if seed%2 else 0x400)+(seed%4)
   commands=[(0x0a000000|src,(dst<<16)|length),save(0,1536)]
  elif kind==2:commands=[(0x0d000000,0),save(0,736)]
  elif kind==3:
   src=([0,16,32,0x300][seed%4])+(seed%3 if seed%4==0 else 0);dst=[0,16,0x300,0x400][seed%4];gain=[0,1,0x4000,0x7fff,0xffff,0x8000][seed%6]
   commands=[(0x0c000000|gain,(src<<16)|dst),save(0,1536)]
  else:
   commands=[]
   for j in range(12):
    op=rng.choice([2,10,12,13]);offset=rng.choice([0,16,32,0x300,0x400]);n=rng.choice([0,16,32,64,128,368])
    commands.append((0x02000000|offset,n) if op==2 else (0x0a000000|offset,(rng.choice([0,16,0x300])<<16)|n) if op==10 else (0x0c000000|rng.randrange(65536),(offset<<16)|rng.choice([0,16,0x300])) if op==12 else (0x0d000000,0))
   commands.append(save(0,1536))
  for i,(a,b) in enumerate(commands):struct.pack_into('<II',ram,command+i*8,a,b)
  struct.pack_into('<16I',dm,0xfc0,2,0,0,0,0x8001e500,0xc60,0x80026780,0x2b0,0,0,0,0,command,len(commands)*8,0,0)
  outputs=[]
  for name in ['_Z19reference_audio_rspPhj','_Z14duck_audio_rspPhj']:
   u.mem_write(0x1000000,bytes(ram));u.mem_write(symbols['dmem'],bytes(dm));u.reg_write(UC_ARM_REG_SP,0x7ff000);u.reg_write(UC_ARM_REG_LR,0x800000);u.reg_write(UC_ARM_REG_R0,0x1000000);u.reg_write(UC_ARM_REG_R1,0x1e500)
   try:u.emu_start(symbols[name]|1,0x800000,count=10000000)
   except Exception as ex:raise RuntimeError((seed,kind,name,hex(u.reg_read(UC_ARM_REG_PC)))) from ex
   assert u.reg_read(UC_ARM_REG_PC)==0x800000 and u.reg_read(UC_ARM_REG_R0)==1,(seed,kind,name)
   outputs.append((bytes(u.mem_read(0x1000000,0x800000)),bytes(u.mem_read(symbols['dmem'],4096)),bytes(u.mem_read(symbols['duck_command_state'],772))))
  if outputs[0]!=outputs[1]:
   diffs=[(i,a,b) for i,(a,b) in enumerate(zip(outputs[0][1],outputs[1][1])) if a!=b];raise AssertionError(('mismatch',seed,kind,diffs[:20]))
  count+=1
report={'ARM32_complete_tasks':count,'RDRAM_bytes_per_task':8388608,'DMEM_bytes_per_task':4096,'mismatches':0,'vector_accumulator_flags_and_scalar_state_compared':True,'real_game_audio_validated':False,'hardware_speed_measured':False};(out/'commands-comparison.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
