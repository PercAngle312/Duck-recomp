#include "duck_vita_time.h"
#include <psp2/kernel/processmgr.h>
uint64_t duck_vita_clock_ticks(void *unused) {
 (void)unused;
 uint64_t us=sceKernelGetProcessTimeWide();
 /* Divide first to retain the fractional tick contribution without an early
    multiplication overflow. The 64-bit result spans millennia. */
 return (us/8)*375+((us%8)*375)/8;
}
