"""Execute the ARM32 display-list scanner fixture (Unicorn 2.1.4)."""
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

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('elf', type=Path)
p.add_argument('--report', type=Path, required=True)
a = p.parse_args()
data = a.elf.read_bytes()
assert data[:6] == b'\x7fELF\x01\x01'
u = Uc(UC_ARCH_ARM, UC_MODE_ARM)
u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
u.mem_map(0x10000, 0x800000)
entry, phoff = struct.unpack_from('<II', data, 24)
entry_size, count = struct.unpack_from('<HH', data, 42)
for i in range(count):
    kind, offset, address, _, size, memory_size, _, _ = struct.unpack_from('<8I', data, phoff + i * entry_size)
    if kind == 1:
        assert 0x10000 <= address and address + memory_size <= 0x810000
        u.mem_write(address, data[offset:offset + size])
u.reg_write(UC_ARM_REG_C1_C0_2, 0xf00000)
u.reg_write(UC_ARM_REG_FPEXC, 0x40000000)
u.reg_write(UC_ARM_REG_SP, 0x7ff000)
u.reg_write(UC_ARM_REG_LR, 0x800000)
u.emu_start(entry, 0x800000, count=2000000000)
assert u.reg_read(UC_ARM_REG_PC) == 0x800000, 'Instruction limit reached'
result = u.reg_read(UC_ARM_REG_R0)
assert result == 6168, f'ARM32 command scanner failed: returned {result}'
report = {'ARM32_executed': True, 'cases': result,
          'operations': ['bounded display-list END/no-push scanner'],
          'comparison': 'original scalar termination scan; both GBI families, first terminator, no-push vs push, tails and alignment',
          'runner': 'Unicorn 2.1.4 Cortex-A9',
          'fixture_sha256': hashlib.sha256(data).hexdigest(),
          'hardware_performance_measured': False}
a.report.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report))
