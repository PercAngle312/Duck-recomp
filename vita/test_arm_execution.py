"""Execute the actual Vita ARM ELF's CPU-only probes with Unicorn, not Vita OS emulation."""
from pathlib import Path
import argparse,json,random,struct
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3,UC_ARM_REG_SP,UC_ARM_REG_LR,UC_ARM_REG_PC,UC_ARM_REG_C1_C0_2,UC_ARM_REG_FPEXC

p=argparse.ArgumentParser(description=__doc__);p.add_argument('elf',type=Path);p.add_argument('--output',type=Path);a=p.parse_args()
u=Uc(UC_ARCH_ARM,UC_MODE_ARM)
with a.elf.open('rb') as file:
 elf=ELFFile(file);assert elf['e_machine']=='EM_ARM' and elf.elfclass==32
 segments=[s for s in elf.iter_segments() if s['p_type']=='PT_LOAD' and s['p_memsz']]
 pages=set()
 for s in segments:pages.update(range(s['p_vaddr']&~4095,(s['p_vaddr']+s['p_memsz']+4095)&~4095,4096))
 for page in sorted(pages):u.mem_map(page,4096)
 for s in segments:u.mem_write(s['p_vaddr'],s.data())
 syms={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
RAM=0x10000000;CTX=0x20000000;STACK=0x30000000;STOP=0x40000000;OUT=CTX+0x1000
u.mem_map(RAM,8*1024*1024);u.mem_map(CTX,0x4000);u.mem_map(STACK,0x100000);u.mem_map(STOP,4096)
u.reg_write(UC_ARM_REG_C1_C0_2,0xf<<20);u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
def call(name,registers,stack_args=()):
 sp=STACK+0xf0000
 u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,STOP)
 for reg,value in zip((UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3),registers):u.reg_write(reg,value&0xffffffff)
 for i,value in enumerate(stack_args):u.mem_write(sp+i*4,struct.pack('<I',value))
 u.emu_start(syms[name],STOP,timeout=10_000_000,count=2_000_000)
 assert u.reg_read(UC_ARM_REG_PC)==STOP,f'{name} did not return'
def put32(addr,value):u.mem_write(addr,struct.pack('<I',value&0xffffffff))
def get64(addr):return struct.unpack('<Q',u.mem_read(addr,8))[0]
def reg64(index):return get64(CTX+index*8)
for fill in (0xa5,0xff,0x37):
 seed=bytes([fill])*(8*1024*1024);u.mem_write(RAM,seed);u.mem_write(CTX,bytes(1024))
 u.mem_write(CTX+4*8,struct.pack('<Q',0x12345678))
 for name in ('handoffs','failed','target'):put32(syms[name],0)
 call('recomp_entrypoint',[RAM,CTX])
 expected=bytearray(seed);expected[0x26a30:0x37f90]=bytes(0x37f90-0x26a30)
 assert u.mem_read(RAM,len(expected))==expected
 assert reg64(29)&0xffffffff==0x800281b0 and reg64(8)&0xffffffff==0x80037f90 and reg64(9)==0 and reg64(4)==0x12345678
 assert struct.unpack('<I',u.mem_read(syms['handoffs'],4))[0]==1
 assert struct.unpack('<I',u.mem_read(syms['failed'],4))[0]==0
module_cases=0
for base in (0x80050000,0x80208000,0x80401230,0x80057df0):
 addresses=struct.unpack('<I',u.mem_read(syms['section_addresses'],4))[0];put32(addresses+4,base)
 for value,threshold in ((10,3),(3,3),(2,3),(0,1),(-1,3),(0x7fffffff,1),(0x7fffffff,0x7fffffff)):
  u.mem_write(CTX,bytes(1024));put32(RAM+(base&0x7fffff)+0x220,threshold);put32(RAM+(base&0x7fffff)+0x224,value)
  before=bytes(u.mem_read(RAM,8*1024*1024));expected=bytearray(before)
  result=value-threshold if value>=threshold else value
  expected[(base&0x7fffff)+0x224:(base&0x7fffff)+0x228]=struct.pack('<I',result&0xffffffff)
  call('duck_m000_0001E0',[RAM,CTX])
  assert u.mem_read(RAM,len(expected))==expected and reg64(4)&0xffffffff==(base+0x224)&0xffffffff
  module_cases+=1
rng=random.Random(0xd0d6e);mask=(1<<64)-1
edges=(0,1,1<<32,(1<<63)-1,1<<63,mask)
cases=[(x,y) for x in edges for y in edges]+[(rng.getrandbits(64),rng.getrandbits(64)) for _ in range(500)]
for x,y in cases:
 for signed,name in ((False,'duck_vita_umul'),(True,'duck_vita_smul')):
  call(name,[x&0xffffffff,x>>32,y&0xffffffff,y>>32],[OUT,OUT+8])
  sx=x if not signed or x<1<<63 else x-(1<<64);sy=y if not signed or y<1<<63 else y-(1<<64);prod=sx*sy
  assert (get64(OUT),get64(OUT+8))==(prod&mask,(prod>>64)&mask)
result={'arm_elf_executed':True,'translated_startup_ram_patterns':3,'translated_module_cases':module_cases,'arm_multiply_cases':len(cases)*2,'vita_hardware_tested':False,'vita_os_or_graphics_tested':False,'full_game_playable':False}
if a.output:a.output.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
