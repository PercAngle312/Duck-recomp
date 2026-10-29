"""Enable exact mixed multiply kernels in private RSP header copies."""
from pathlib import Path
def patch(headers: Path):
    p=headers/'librecomp/rsp_vu_impl.hpp';s=p.read_text()
    if 'duck_vita_rsp_mixed.h' in s:raise ValueError('Mixed multiply already applied')
    for name,acc,signed in [('VMUDM','false','true'),('VMUDN','false','false'),('VMADM','true','true'),('VMADN','true','false'),('VMUDL',None,None)]:
        anchor=f'auto RSP::{name}(r128& vd, cr128& vs, cr128& vt) -> void {{'
        if s.count(anchor)!=1:raise ValueError('Unrecognized RSP instruction '+name)
        call='duck_rsp_multiply_low' if acc is None else f'duck_rsp_multiply_mixed<{acc},{signed}>'
        body='\n#if defined(__ARM_NEON)\n    const r128 shuffled=vt(e);\n    '+call+'(&vd.u128,&vs.u128,&shuffled.u128,&ACCL.u128,&ACCM.u128,&ACCH.u128);\n    return;\n#endif\n'
        s=s.replace(anchor,anchor+body)
    p.write_text('#include "duck_vita_rsp_mixed.h"\n'+s)
