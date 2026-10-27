"""Patch private copies of pinned RSP headers; generated source stays intact."""
from pathlib import Path


def patch(headers: Path):
    path = headers / 'librecomp/rsp_vu_impl.hpp'
    text = path.read_text()
    original = text
    for name, scale, alignment, width in [('LQV', 16, 15, 16), ('SQV', 16, 15, 16),
                                         ('LDV', 8, 7, 8), ('SDV', 8, 7, 8)]:
        kind = 'r128' if name.startswith('L') else 'cr128'
        anchor = (f'auto RSP::{name}({kind}& vt, cr32& rs, s8 imm) -> void {{\n'
                  f'    auto address = rs + imm * {scale};')
        if text.count(anchor) != 1:
            raise ValueError('Unrecognized pinned RSP memory operation: ' + name)
        element = 'e == 0' if width == 16 else '(e == 0 || e == 8)'
        vec = '&vt.u128' if width == 16 else '(uint8_t*)&vt.u128 + (e == 0 ? 8 : 0)'
        memory = 'dmem + (address & 0xfff)'
        args = (vec + ', ' + memory) if name.startswith('L') else (memory + ', ' + vec)
        fast = (f'\n    if constexpr ({element}) {{\n'
                f'        if ((address & {alignment}) == 0) {{\n'
                f'            duck_rsp_reverse_words{width}({args}); return;\n'
                '        }\n    }')
        text = text.replace(anchor, anchor + fast)
    if text == original:
        raise ValueError('No RSP memory edits')
    path.write_text('#include "duck_vita_rsp_memory.h"\n' + text)
    path = headers / 'librecomp/rsp.hpp'
    text = path.read_text()
    for operation, length, helper, args in [
        ('read', 'rd_len', 'duck_rsp_dma_read', 'dmem, rdram, dmem_addr, dram_addr, rd_len'),
        ('write', 'wr_len', 'duck_rsp_dma_write', 'rdram, dmem, dram_addr, dmem_addr, wr_len'),
    ]:
        anchor = f'    assert(dmem_addr + {length} <= 0x1000);'
        if text.count(anchor) != 1:
            raise ValueError('Unrecognized pinned RSP DMA ' + operation)
        text = text.replace(anchor, anchor + f'\n    if ({helper}({args})) return;')
    path.write_text('#include "duck_vita_rsp_memory.h"\n' + text)
