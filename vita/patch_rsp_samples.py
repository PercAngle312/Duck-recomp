"""Native sample-window transfers and constant vector selectors in private copies."""
from pathlib import Path
def patch(headers:Path):
 p=headers/'librecomp/rsp_vu_impl.hpp';s=p.read_text()
 if 'duck_vita_rsp_samples.h' in s:raise ValueError('Sample specialization already applied')
 for name in ['LDV','SDV']:
  kind='r128' if name=='LDV' else 'cr128'
  anchor=f'auto RSP::{name}({kind}& vt, cr32& rs, s8 imm) -> void {{\n    auto address = rs + imm * 8;'
  if s.count(anchor)!=1:raise ValueError('Pinned sample operation mismatch')
  vector='(uint8_t*)&vt.u128 + (e == 0 ? 8 : 0)' if name=='LDV' else '(const uint8_t*)&vt.u128 + (e == 0 ? 8 : 0)'
  call=f'duck_rsp_sample_load8({vector},dmem,address)' if name=='LDV' else f'duck_rsp_sample_store8(dmem,address,{vector})'
  s=s.replace(anchor,anchor+f'\n    if constexpr (e == 0 || e == 8) {{if ({call})return;}}')
 count=s.count('vt(e)')
 if count<32:raise ValueError('Pinned element selection sites mismatch')
 s=s.replace('vt(e)','duck_rsp_element<e>(vt)')
 p.write_text('#include "duck_vita_rsp_samples.h"\n#include "duck_vita_rsp_elements.h"\n'+s)
 return count
