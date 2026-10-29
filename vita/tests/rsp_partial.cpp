#include "duck_vita_rsp_partial.h"
#include <cstddef>
extern "C" void *memcpy(void *d,const void *s,size_t n){auto *a=(volatile uint8_t*)d;auto *b=(const volatile uint8_t*)s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
extern "C" void *memset(void *d,int x,size_t n){auto *a=(volatile uint8_t*)d;for(size_t i=0;i<n;i++)a[i]=(uint8_t)x;return d;}
alignas(16) static uint8_t memory[4096];
extern "C" unsigned duck_test_rsp_partial(){
 unsigned cases=0;
 for(unsigned seed=0;seed<4;seed++){
  for(unsigned i=0;i<4096;i++)memory[i]=(uint8_t)(i*29+(i>>4)*7+seed*31);
  for(unsigned address=0;address<4096;address++)for(unsigned right=0;right<2;right++){
   alignas(16) uint8_t actual[16],expected[16];
   for(unsigned j=0;j<16;j++)actual[j]=expected[j]=(uint8_t)(j*17+address+seed);
   // Original runtime's logical-byte loop, independently expressed here.
   unsigned p=address;
   if(right){unsigned start=16-(p&15);p&=~15u;for(unsigned j=start;j<16;j++)expected[15-j]=memory[((p++)&4095)^3];}
   else for(unsigned j=0;j<16-(p&15);j++)expected[15-j]=memory[((address+j)&4095)^3];
   // High address bits and unsigned wrap must not change DMEM addressing.
   const uint32_t high=(seed==1?0x80000000u:seed==2?0xfffff000u:0u);
   if(right)duck_rsp_load_partial<true>(actual,memory,address|high);
   else duck_rsp_load_partial<false>(actual,memory,address|high);
   for(unsigned j=0;j<16;j++)if(actual[j]!=expected[j])return 0;
   ++cases;
  }
 }
 return cases;
}
