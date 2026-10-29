#include "duck_vita_rsp_mixed.h"
#include <cstdint>
#include <cstddef>
extern "C" void *memcpy(void *d,const void *s,size_t n){auto *a=(volatile uint8_t*)d;auto *b=(const volatile uint8_t*)s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
extern "C" void *memset(void *d,int x,size_t n){auto *a=(volatile uint8_t*)d;for(size_t i=0;i<n;i++)a[i]=(uint8_t)x;return d;}
static uint32_t rng=0x68413df;
static uint32_t random_word(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
extern "C" {volatile unsigned duck_mixed_failure;}
template<bool Accumulate,bool SignedSource,bool Low=false>
bool exercise(unsigned iteration,unsigned alias){
 alignas(16) uint16_t a[8],b[8],l[8],m[8],h[8],out[8],el[8],em[8],eh[8],expected[8];
 constexpr uint16_t edge[]={0,1,0x7fff,0x8000,0xffff,0x8001,0x4000,0xfffe};
 for(unsigned i=0;i<8;i++){
  a[i]=iteration<64?edge[(iteration/8+i)%8]:(uint16_t)random_word();b[i]=iteration<64?edge[iteration%8]:(uint16_t)random_word();
  l[i]=(uint16_t)random_word();m[i]=(uint16_t)random_word();h[i]=(uint16_t)random_word();
  if(iteration%4==0)h[i]=(m[i]&0x8000)?0xffff:0;
  if(iteration%8==0)l[i]=m[i]=h[i]=0xffff;
  uint64_t acc=Accumulate?((uint64_t)h[i]<<32)|((uint64_t)m[i]<<16)|l[i]:0;
  if constexpr(Low)acc=(uint32_t(a[i])*b[i])>>16;
  else acc+=SignedSource?int64_t((int16_t)a[i])*b[i]:int64_t(a[i])*(int16_t)b[i];
  el[i]=(uint16_t)acc;em[i]=(uint16_t)(acc>>16);eh[i]=(uint16_t)(acc>>32);
  if constexpr(Low)expected[i]=el[i];
  else if constexpr(!Accumulate)expected[i]=SignedSource?em[i]:el[i];
  else{
   bool negative=(int16_t)eh[i]<0;
   bool fits=negative?(eh[i]==0xffff&&(int16_t)em[i]<0):(eh[i]==0&&(int16_t)em[i]>=0);
   expected[i]=fits?(SignedSource?em[i]:el[i]):SignedSource?(negative?0x8000:0x7fff):(negative?0:0xffff);
  }
 }
 auto *dst=alias==0?out:alias==1?a:b;
 if constexpr(Low)duck_rsp_multiply_low(dst,a,b,l,m,h);
 else duck_rsp_multiply_mixed<Accumulate,SignedSource>(dst,a,b,l,m,h);
 for(unsigned i=0;i<8;i++)if(dst[i]!=expected[i]||l[i]!=el[i]||m[i]!=em[i]||h[i]!=eh[i])return false;
 return true;
}
extern "C" unsigned duck_test_rsp_mixed(){
 unsigned count=0;
 for(unsigned i=0;i<8192;i++)for(unsigned alias=0;alias<3;alias++){
#define CHECK(a,s,l) if(!exercise<a,s,l>(i,alias)){duck_mixed_failure=count+1;return 0;}count++
  CHECK(false,true,false);CHECK(false,false,false);CHECK(true,true,false);CHECK(true,false,false);CHECK(false,false,true);
#undef CHECK
 }
 duck_mixed_failure=0;return count;
}
