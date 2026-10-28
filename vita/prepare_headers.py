"""Generate ARM32 adapters from pinned upstream headers without changing them."""
import argparse
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--recompiler',type=Path,required=True)
p.add_argument('--runtime',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
def write_changed(path,data):
 if not path.exists() or path.read_bytes()!=data:path.write_bytes(data)
s=(a.recompiler/'include/recomp.h').read_text()
anchor='#elif defined(_MSC_VER)\n\n#include <intrin.h>'
assert s.count(anchor)==1,'Unexpected upstream multiplication header'
s=s.replace(anchor,'#elif defined(__arm__) && !defined(__aarch64__)\n#include "portable_mul.h"\n#define DMULT duck_dmult\n#define DMULTU duck_dmultu\n\n'+anchor)
# Mask the complete guest offset, preserving word-swapped byte/halfword lanes.
# This maps KSEG0/KSEG1 into one allocation on the 32-bit host.
needle='rdram + (';start=0;count=0
while (index:=s.find(needle,start))>=0:
 opening=index+len(needle)-1;depth=1;end=opening+1
 while depth:
  if s[end]=='(':depth+=1
  elif s[end]==')':depth-=1
  end+=1
 expression=s[opening:end]
 replacement='(((uint32_t)'+expression+') & 0x1FFFFFFFu)'
 s=s[:opening]+replacement+s[end:];start=opening+len(replacement);count+=1
assert count==7,f'Unexpected memory macro count: {count}'
write_changed(a.output/'recomp.h',s.encode())
(a.output/'librecomp').mkdir(exist_ok=True)
for name in ('rsp.hpp','rsp_vu.hpp','rsp_vu_impl.hpp'):
 write_changed(a.output/'librecomp'/name,(a.runtime/'librecomp/include/librecomp'/name).read_bytes())
print(f'Adapted ARM32 128-bit multiply and {count} memory accesses; RSP starts with portable scalar path.')
