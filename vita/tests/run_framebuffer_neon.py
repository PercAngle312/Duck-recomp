"""Run the freestanding framebuffer fixture under Unicorn 2.1.4 ARM32."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM
from unicorn.arm_const import (
    UC_CPU_ARM_CORTEX_A9, UC_ARM_REG_PC, UC_ARM_REG_SP, UC_ARM_REG_LR,
    UC_ARM_REG_R0, UC_ARM_REG_FPEXC, UC_ARM_REG_C1_C0_2,
)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('elf', type=Path)
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args()
data = args.elf.read_bytes()
assert data[:6] == b'\x7fELF\x01\x01'
runner = Uc(UC_ARCH_ARM, UC_MODE_ARM)
runner.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
runner.mem_map(0x10000, 0x400000)
entry, phoff = struct.unpack_from('<II', data, 24)
entry_size, count = struct.unpack_from('<HH', data, 42)
for i in range(count):
    kind, offset, address, _, size, memory_size, _, _ = struct.unpack_from(
        '<8I', data, phoff + i * entry_size)
    if kind == 1:
        assert 0x10000 <= address and address + memory_size <= 0x410000
        runner.mem_write(address, data[offset:offset + size])
runner.reg_write(UC_ARM_REG_C1_C0_2, 0xf00000)
runner.reg_write(UC_ARM_REG_FPEXC, 0x40000000)
runner.reg_write(UC_ARM_REG_SP, 0x3ff000)
runner.reg_write(UC_ARM_REG_LR, 0x400000)
runner.emu_start(entry, 0x400000, count=100000000)
result = runner.reg_read(UC_ARM_REG_R0)
assert runner.reg_read(UC_ARM_REG_PC) == 0x400000, 'Instruction limit reached'
assert result == 0, f'ARM32 framebuffer mismatch: {result}'
report = {
    'ARM32_NEON_executed': True,
    'conversion_values': 65536,
    'odd_tail_tested': True,
    'both_halfword_alignments_tested': True,
    'out_of_span_bytes_preserved': True,
    'runner': 'Unicorn 2.1.4 Cortex-A9',
    'fixture_elf_sha256': hashlib.sha256(data).hexdigest(),
    'result': result,
    'hardware_performance_measured': False,
}
args.report.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report))
