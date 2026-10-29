"""Enable exact native LQV<0>/LRV<0> loads in private pinned RSP headers."""
from pathlib import Path
def patch(headers: Path):
 p=headers/'librecomp/rsp_vu_impl.hpp';s=p.read_text()
 if 'duck_vita_rsp_partial.h' in s:raise ValueError('Partial loads already applied')
 for name,right in [('LQV','false'),('LRV','true')]:
  anchor=f'auto RSP::{name}(r128& vt, cr32& rs, s8 imm) -> void {{'
  if s.count(anchor)!=1:raise ValueError('Unrecognized pinned instruction '+name)
  code='\n#if defined(__ARM_NEON)\n    if constexpr (e == 0) {\n        duck_rsp_load_partial<'+right+'>(&vt.u128,dmem,rs + imm * 16);return;\n    }\n#endif\n'
  s=s.replace(anchor,anchor+code)
 p.write_text('#include "duck_vita_rsp_partial.h"\n'+s)
