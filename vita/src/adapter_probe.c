#include "portable_mul.h"
/* External entry points keep the hardware/emulated-ARM checks from becoming
   constant-folded assertions inside the diagnostic screen. */
__attribute__((noinline)) void duck_vita_umul(uint64_t a,uint64_t b,uint64_t* lo,uint64_t* hi) {duck_dmultu(a,b,lo,hi);}
__attribute__((noinline)) void duck_vita_smul(int64_t a,int64_t b,int64_t* lo,int64_t* hi) {duck_dmult(a,b,lo,hi);}
