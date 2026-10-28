"""Execute rebuilt ARM I/O and private binding wrappers; no Vita OS emulation."""
import argparse,json,struct
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import (UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3,
 UC_ARM_REG_SP,UC_ARM_REG_LR,UC_ARM_REG_PC,UC_ARM_REG_C1_C0_2,UC_ARM_REG_FPEXC)
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--elf',required=True);p.add_argument('--bindings',required=True);p.add_argument('--output',required=True)
a=p.parse_args();bindings=json.load(open(a.bindings));u=Uc(UC_ARCH_ARM,UC_MODE_ARM)
with open(a.elf,'rb') as file:
 elf=ELFFile(file);assert elf.elfclass==32 and elf['e_machine']=='EM_ARM'
 segments=[s for s in elf.iter_segments() if s['p_type']=='PT_LOAD' and s['p_memsz']]
 pages=set()
 for s in segments:pages.update(range(s['p_vaddr']&~4095,(s['p_vaddr']+s['p_memsz']+4095)&~4095,4096))
 for page in sorted(pages):u.mem_map(page,4096)
 for s in segments:u.mem_write(s['p_vaddr'],s.data())
 symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
 assert not [s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name and s['st_shndx']=='SHN_UNDEF']
RAM=0x10000000;CTX=0x20000000;ROM=0x21000000;STACK=0x30000000;STOP=0x40000000
for address,size in [(RAM,0x800000),(CTX,4096),(ROM,4096),(STACK,0x10000),(STOP,4096)]:u.mem_map(address,size)
u.reg_write(UC_ARM_REG_C1_C0_2,0xf<<20);u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
calls=0
def call(name,args):
 global calls
 u.reg_write(UC_ARM_REG_SP,STACK+0xff00);u.reg_write(UC_ARM_REG_LR,STOP)
 for r,v in zip((UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3),args):u.reg_write(r,v&0xffffffff)
 u.emu_start(symbols[name],STOP,timeout=1_000_000,count=1000000)
 assert u.reg_read(UC_ARM_REG_PC)==STOP,name+' did not return'
 calls+=1;return u.reg_read(UC_ARM_REG_R0)
def wr(p,v):u.mem_write(RAM+p,struct.pack('<I',v&0xffffffff))
def rd(p):return struct.unpack('<I',u.mem_read(RAM+p,4))[0]
def reg(i,v):u.mem_write(CTX+i*8,struct.pack('<Q',v&0xffffffffffffffff))
def result():return struct.unpack('<Q',u.mem_read(CTX+16,8))[0]
def invoke(callback,**values):
 for i,v in values.items():reg(int(i[1:]),v)
 call(bindings[callback],[RAM,CTX])
rom=bytes(i^0x5a for i in range(256));u.mem_write(ROM,rom)
assert call('duck_vita_os_io_reset',[RAM,ROM,len(rom)])==0
for off in (0,4,128,252):
 invoke('osPiReadIo_recomp',r4=0x10000000+off,r5=0xffffffffa0000100)
 assert result()==0 and rd(0x100)==int.from_bytes(rom[off:off+4],'big')
before=bytes(u.mem_read(RAM,0x800000))
invoke('osPiReadIo_recomp',r4=0x100000ff,r5=0x80000100)
assert result()==0xffffffffffffffff and bytes(u.mem_read(RAM,len(before)))==before
# Return queue and manager command queue live in actual guest RAM.
for p,v in [(0x3010,4),(0x3014,0x80003100)]:wr(p,v)
invoke('osCreatePiManager_recomp',r4=100,r5=0x80002000,r6=0x80002100,r7=2)
def submit(mb,priority,off,dest,count):
 wr(0x5010,0x80000000|dest);wr(0x5014,count);wr(0x5018,0x80003000)
 invoke('osPiStartDma_recomp',r4=0x80000000|mb,r5=priority,r6=0,r7=off,r29=0xffffffff80005000)
submit(0x4000,0,8,0x6000,16);assert result()==0
submit(0x4040,1,24,0x6020,8);assert result()==0 and rd(0x6000)==0
assert call('duck_vita_os_pi_service',[])==1 and rd(0x3100)==0x80004040
assert rd(0x6000)==0
for i in range(8):assert bytes(u.mem_read(RAM+((0x6020+i)^3),1))==rom[24+i:25+i]
assert call('duck_vita_os_pi_service',[])==1 and rd(0x3104)==0x80004000
for i in range(16):assert bytes(u.mem_read(RAM+((0x6000+i)^3),1))==rom[8+i:9+i]
assert call('duck_vita_os_pi_service',[])==0
invoke('osSetEventMesg_recomp',r4=8,r5=0x80003000,r6=0x12345678)
assert rd(0x3008)==2
assert call('duck_vita_os_event_emit',[8])==0 and rd(0x3108)==0x12345678
invoke('osSetEventMesg_recomp',r4=8,r5=0)
assert call('duck_vita_os_event_emit',[8])==0xffffffff
report=dict(arm_calls_returned=calls,private_binding_wrappers_executed=4,pio_valid_cases=4,
 invalid_pio_entire_ram_unchanged=True,priority_dma_jobs_completed=2,
 word_swapped_dma_verified=True,completion_messages_are_job_pointers=True,
 event_registration_and_delivery_passed=True,vita_os_tested=False,gameplay_tested=False)
with open(a.output,'w') as file:json.dump(report,file,indent=2);file.write('\n')
print(json.dumps(report,indent=2))
