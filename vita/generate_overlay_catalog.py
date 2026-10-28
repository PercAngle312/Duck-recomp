"""Generate private static-function catalog and optional allocation observer.

Inputs are existing generated game metadata/source, never a public asset pack.
Original sources are read-only. Output contains proprietary-derived identifiers
and MUST remain outside the public repository.
"""
import argparse,hashlib,json,re,shutil
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for name in ('resident-source','callbacks','module-overlay','module-catalog','output'):
 p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--allocator-symbol')
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
resident={};seen=set();manifest=json.loads(a.callbacks.read_text())
for name,digest in manifest.items():
 if Path(name).name!=name or not name.endswith('.c'):raise ValueError('Unsafe callback basename')
 if hashlib.sha256((a.callbacks.parent/name).read_bytes()).hexdigest()!=digest:raise ValueError('Callback checksum mismatch')
sources=list(a.resident_source.glob('funcs_*.c'))+[a.callbacks.parent/n for n in manifest]
for source in sources:
 for name in re.findall(r'\bvoid\s+(func_[0-9A-Fa-f]+|recomp_entrypoint)\([^{};]*\)\s*\{',source.read_text()):
  address=0x80000400 if name=='recomp_entrypoint' else int(name[5:],16)
  if address in resident:raise ValueError('Duplicate resident export: '+name)
  resident[address]=name
if not resident:raise ValueError('No resident functions')
text=a.module_overlay.read_text()
arrays={}
for section,index,body in re.findall(r'static FuncEntry section_(\d+)_module_(\d+)_funcs\[\]\s*=\s*\{(.*?)\};',text,re.S):
 functions=[(int(off,16),name) for name,off in re.findall(r'\.func\s*=\s*(\w+),\s*\.offset\s*=\s*(0x[0-9A-Fa-f]+)',body)]
 if len(functions)!=body.count('.func ='):raise ValueError('Unparsed function row')
 if len({off for off,name in functions})!=len(functions):raise ValueError('Duplicate module entry')
 if int(section)!=int(index)+1:raise ValueError('Unexpected section index')
 arrays[int(index)]=(int(section),sorted(functions))
catalog=[tuple(int(x,0) for x in row) for row in re.findall(r'\{(0x[0-9A-Fa-f]+)u,\s*(0x[0-9A-Fa-f]+)u,\s*(\d+)u,\s*(\d+)u\}',a.module_catalog.read_text())]
if len(catalog)!=len(arrays) or set(arrays)!=set(range(len(catalog))):raise ValueError('Catalog/module count mismatch')
if len({r[0] for r in catalog})!=len(catalog):raise ValueError('Duplicate module ID')
sections={}
for rom,base,size,section,index in re.findall(r'\.rom_addr\s*=\s*(0x[0-9A-Fa-f]+),\s*\.ram_addr\s*=\s*(0x[0-9A-Fa-f]+),\s*\.size\s*=\s*(0x[0-9A-Fa-f]+),\s*\.funcs\s*=\s*section_(\d+)_module_(\d+)_funcs',text):
 index=int(index)
 if index in sections:raise ValueError('Duplicate section descriptor')
 sections[index]=(int(rom,16),int(base,16),int(size,16),int(section))
if len(sections)!=len(catalog):raise ValueError('Section descriptor count mismatch')
for index,(id,rom,size,bss) in enumerate(catalog):
 if sections[index][0]!=rom or sections[index][2]!=size or sections[index][3]!=index+1:raise ValueError('Module catalog/section mismatch')
lines=['/* PRIVATE GENERATED GAME CATALOG. Do not publish. */','#include "duck_vita_overlays.h"']
all_names=set(resident.values())
for section,functions in arrays.values():all_names.update(name for off,name in functions)
for name in sorted(all_names):lines.append(f'extern void {name}(uint8_t*,recomp_context*);')
lines.append('static const DuckCodeFunction resident[]={')
for address,name in sorted(resident.items()):lines.append(f'{{0x{address:08x}u,{name}}},')
lines.append('};')
for index,(section,functions) in sorted(arrays.items()):
 if not functions:continue
 lines.append(f'static const DuckCodeFunction module_{index}[]={{')
 for off,name in functions:
  if off>=catalog[index][2]:raise ValueError('Function exceeds initialized module')
  lines.append(f'{{0x{off:x}u,{name}}},')
 lines.append('};')
lines.append('static const DuckCodeModule modules[]={')
for index,(id,rom,size,bss) in enumerate(catalog):
 section,functions=arrays[index]
 ptr=f'module_{index}' if functions else 'NULL'
 lines.append(f'{{0x{id:08x}u,{size}u,{bss}u,{section}u,0x{sections[index][1]:08x}u,{ptr},{len(functions)}u}},')
lines+=['};',f'const DuckCodeCatalog duck_vita_private_catalog={{resident,{len(resident)}u,modules,{len(catalog)}u,0x80000400u,0x80037f90u,{int(bool(a.allocator_symbol))}}};']
(a.output/'private_catalog.c').write_text('\n'.join(lines)+'\n')
observed=0;returns=0
if a.allocator_symbol:
 if not re.fullmatch(r'func_[0-9A-Fa-f]+',a.allocator_symbol):raise ValueError('Invalid allocator symbol')
 destination=a.output/'resident';destination.mkdir(exist_ok=True)
 if destination.resolve()==a.resident_source.resolve():raise ValueError('Observer output must not overwrite source')
 for source in a.resident_source.glob('funcs*'):
  if source.suffix!='.c':shutil.copy2(source,destination/source.name);continue
  data=source.read_text();pattern=r'(RECOMP_FUNC void '+a.allocator_symbol+r'\([^{}]*\)\s*\{)(.*?)(?=RECOMP_FUNC void |\Z)'
  def instrument(match):
   global observed,returns
   observed+=1;body=match.group(2);returns=body.count('return;')
   if not returns:raise ValueError('Allocator has no native return')
   body=body.replace('return;','duck_vita_after_allocation(rdram,(uint32_t)ctx->r2,duck_allocation_request); return;')
   return match.group(1)+'\n    const uint32_t duck_allocation_request=(uint32_t)ctx->r4;'+body
  before=observed;data=re.sub(pattern,instrument,data,flags=re.S)
  if observed>before and '#include "duck_vita_overlays.h"' not in data:data='#include "duck_vita_overlays.h"\n'+data
  (destination/source.name).write_text(data)
 if observed!=1:raise ValueError('Allocator must have exactly one definition')
print(json.dumps(dict(resident_functions=len(resident),modules=len(catalog),module_functions=sum(len(x[1]) for x in arrays.values()),allocator_definitions_observed=observed,allocator_return_observers=returns)))
