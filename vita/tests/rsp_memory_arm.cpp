#include "duck_vita_rsp_memory.h"
extern "C" unsigned duck_rsp_memory_reference(unsigned,unsigned,unsigned,uint8_t*);
extern "C" unsigned duck_rsp_memory_fast(unsigned,unsigned,unsigned,uint8_t*);
alignas(16) uint8_t dmem[4096];
uint16_t rspReciprocals[512],rspInverseSquareRoots[512];
alignas(16) static uint8_t a[4112],b[4112];
alignas(16) static uint8_t ram[8192];
extern "C" void *memcpy(void *d,const void *s,size_t n){
 auto *dst=(volatile uint8_t*)d;auto *src=(const volatile uint8_t*)s;
 for(size_t i=0;i<n;i++)dst[i]=src[i];return d;
}
extern "C" void *memset(void *d,int value,size_t n){
 auto *dst=(volatile uint8_t*)d;for(size_t i=0;i<n;i++)dst[i]=uint8_t(value);return d;
}
extern "C" int duck_rsp_memory_arm_test(){
 const unsigned addresses[]={0,1,2,3,4,7,8,15,16,17,24,31,32,255,256,
     1023,1024,2048,4080,4088,4095,0xfffffff0u,0xffffffffu,0x80000ff8u};
 unsigned count=0;
 for(unsigned e=0;e<16;e++)for(unsigned address:addresses)for(unsigned op=0;op<8;op++){
  unsigned n=duck_rsp_memory_reference(e,address,op,a);
  if(n!=duck_rsp_memory_fast(e,address,op,b))return 1;
  for(unsigned i=0;i<n;i++)if(a[i]!=b[i])return 2+(int)count;
  count++;
 }
 // Verify every DMA byte count against the pinned per-byte twiddle model.
 // Guard bytes before and after each transfer must stay untouched.
 for(unsigned n=0;n<=4096;n++){
  for(unsigned i=0;i<sizeof ram;i++)ram[i]=uint8_t(i*13+(i>>3));
  for(unsigned i=0;i<sizeof dmem;i++)dmem[i]=0xa5;
  if(!duck_rsp_dma_read(dmem,ram,0,8,n))return -1;
  for(unsigned i=0;i<4096;i++){
   uint8_t expected=i<n?ram[(8+i)^3]:0xa5;
   if(dmem[i^3]!=expected)return -2;
  }
  for(unsigned i=0;i<sizeof ram;i++)ram[i]=0xa5;
  if(!duck_rsp_dma_write(ram,dmem,8,0,n))return -3;
  for(unsigned i=0;i<sizeof ram;i++){
   unsigned logical=i^3;
   uint8_t expected=logical>=8&&logical<8+n?dmem[(logical-8)^3]:0xa5;
   if(ram[i]!=expected)return -4;
  }
 }
 if(duck_rsp_dma_read(dmem,ram,1,8,8)||duck_rsp_dma_write(ram,dmem,9,0,8)||
    duck_rsp_dma_read(dmem,ram,4092,8,8)||duck_rsp_dma_write(ram,dmem,8,4092,8)||
    duck_rsp_dma_read(dmem,ram,0,0x800000,1))return -5;
 return 0;
}
