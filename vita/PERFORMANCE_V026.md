# v026 additional CPU/audio candidate

Builds on v025; preserve both installers for hardware comparison. No measured
Vita FPS improvement is claimed.

## Native unaligned vector loads

The private generated audio routine contains 46 LQV<0> and 28 LRV<0> instruction
sites. Previously only aligned LQV had a native load path; unaligned LQV and LRV
copied selected bytes in scalar loops. New exact NEON table gathers load one
aligned 16-byte DMEM block and merge only the selected register bytes. LQV's
aligned case keeps its direct load; LRV at offset zero returns without touching
memory. Other element selectors retain the original runtime implementation.
The generator remains unchanged. Add `--native-partial-loads` to the existing
NEON rebuild configuration; the flag patches private header copies only.

32,768 Cortex-A9 emulated comparisons cover both operations at every DMEM
address, four input patterns, high address bits and all preserved register
bytes. This proves byte results for the helpers, not hardware timing or complete
real-game audio. The complete synthetic task comparison additionally checks
load/save, clear/move, interleave and mixer tasks against v025 across RDRAM/DMEM.

## Scheduler timer fast path

The timer service caches the earliest active record until a timer is added,
replaced, stopped or expired. It still samples the clock and updates Count /
Compare at every service; VI events, event ordering, periodic catchup and the
128-event budget remain unchanged. Invalidation occurs before sending expiry
messages, since a queue send can switch guest fibers and mutate timers.

The reference/candidate differential check executes 40,000 randomized operations
across eight fresh sessions with zero mismatches. It compares guest timer memory,
return registers, event messages, time statistics, faults and Compare state.
Coverage includes full tables, replacement, deadlines and ties, overflows,
periodic catchup, VI changes, dropped messages and reentrant stop/rearm/poll.
The existing blocked-fiber timer regression passes with UBSan.

Test-only instrumentation verifies that 10,000 idle polls inspect 32 timer slots
in total (one initial scan), versus 320,000 previously. The same result holds
with a future timer pending. Instrumentation is absent from production builds.
This reduces service work; it does not predict the overall FPS gain.

## Reproduce checks

Use the private recomp headers and VitaSDK already used by the existing port.
Extract the original timer implementation outside the checkout:

```
git show 397854d35b4448d1ef138a76483166c70cfe3ac0:vita/src/os_time.c > /tmp/os_time-v025.c
python3 vita/test_time_differential.py --reference /tmp/os_time-v025.c \
  --headers /path/to/private/headers --report /tmp/time-comparison.json
arm-vita-eabi-g++ -std=c++20 -O2 -ffreestanding -fno-builtin \
  -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -I vita/include \
  -nostdlib -Wl,-Ttext=0x10000 -Wl,-e,duck_test_rsp_partial \
  vita/tests/rsp_partial.cpp -o /tmp/rsp-partial.elf
python3 vita/tests/run_rsp_partial_arm.py /tmp/rsp-partial.elf \
  --report /tmp/partial-comparison.json
```

The ARM runner uses Unicorn 2.1.4. Install v026 over DUCKVIT04, compare the same
scene at Native / AA Off / Live Stats On, and check sound, controls, saves and
framebuffer effects. v025 drawing/readback changes and their safe fallbacks are
retained. Real hardware validation is still required.
