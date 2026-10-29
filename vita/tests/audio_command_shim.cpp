#include <cstddef>
#include <cstdint>
alignas(16) uint8_t dmem[4096];
uint16_t rspReciprocals[512],rspInverseSquareRoots[512];
extern "C" {uint8_t duck_command_state[772];}
extern "C" void *memcpy(void *d,const void *s,size_t n){auto*a=(volatile uint8_t*)d;auto*b=(const volatile uint8_t*)s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
extern "C" void *memset(void *d,int x,size_t n){auto*a=(volatile uint8_t*)d;for(size_t i=0;i<n;i++)a[i]=(uint8_t)x;return d;}
extern "C" void duck_test_capture(const void *vpu,const uint32_t *registers){
 memcpy(duck_command_state,vpu,644);duck_command_state[644]=((const uint8_t*)vpu)[644];
 memset(duck_command_state+645,0,3);memcpy(duck_command_state+648,registers,124);
}
extern "C" int __cxa_guard_acquire(uint64_t*g){return !*(uint8_t*)g;}
extern "C" void __cxa_guard_release(uint64_t*g){*(uint8_t*)g=1;}
extern "C" void __assert_func(const char*,int,const char*,const char*){asm volatile("bkpt #0");for(;;){}}
extern "C" int printf(const char*,...){asm volatile("bkpt #0");return 0;}
extern "C" void abort(){asm volatile("bkpt #0");for(;;){}}
extern "C" void __aeabi_unwind_cpp_pr1(){asm volatile("bkpt #0");}
