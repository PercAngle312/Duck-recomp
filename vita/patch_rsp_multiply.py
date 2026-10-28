"""Enable native ARM32 fractional and optional high multiply in private headers."""
from pathlib import Path


def patch(headers: Path, high: bool = False):
    path = headers / 'librecomp/rsp_vu_impl.hpp'
    text = path.read_text()
    if 'duck_vita_rsp_multiply.h' in text:
        raise ValueError('RSP multiply patch already applied')
    for name, accumulate in [('VMULF', 'false'), ('VMACF', 'true')]:
        anchor = f'auto RSP::{name}(r128& vd, cr128& vs, cr128& vt) -> void {{'
        if text.count(anchor) != 1:
            raise ValueError('Unrecognized pinned RSP fractional multiply: ' + name)
        body = ('\n#if defined(__ARM_NEON)\n'
                '    const r128 shuffled = vt(e);\n'
                f'    duck_rsp_multiply<{accumulate}, U != 0>(&vd.u128, &vs.u128, &shuffled.u128,\n'
                '        &ACCL.u128, &ACCM.u128, &ACCH.u128);\n'
                '    return;\n#endif\n')
        text = text.replace(anchor, anchor + body)
    if high:
        for name, accumulate in [('VMUDH', 'false'), ('VMADH', 'true')]:
            anchor = f'auto RSP::{name}(r128& vd, cr128& vs, cr128& vt) -> void {{'
            if text.count(anchor) != 1:
                raise ValueError('Unrecognized pinned RSP high multiply: ' + name)
            body = ('\n#if defined(__ARM_NEON)\n'
                    '    const r128 shuffled = vt(e);\n'
                    f'    duck_rsp_multiply_high<{accumulate}>(&vd.u128, &vs.u128, &shuffled.u128,\n'
                    '        &ACCL.u128, &ACCM.u128, &ACCH.u128);\n'
                    '    return;\n#endif\n')
            text = text.replace(anchor, anchor + body)
    path.write_text('#include "duck_vita_rsp_multiply.h"\n' + text)
