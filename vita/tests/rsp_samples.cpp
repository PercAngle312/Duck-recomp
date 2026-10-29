#include "duck_vita_rsp_samples.h"
#include "duck_vita_rsp_elements.h"
#include <cstddef>
extern "C" void *memcpy(void*d,const void*s,size_t n){auto*a=(volatile uint8_t*)d;auto*b=(const volatile uint8_t*)s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
extern "C" void *memset(void*d,int v,size_t n){auto*a=(volatile uint8_t*)d;for(size_t i=0;i<n;i++)a[i]=(uint8_t)v;return d;}
struct alignas(16) Vector {union{uint64_t u128[2];uint16_t words[8];};};
static Vector select(const Vector&v,unsigned e){switch(e){
#define CASE(n) case n:return duck_rsp_element<n>(v)
CASE(0);CASE(1);CASE(2);CASE(3);CASE(4);CASE(5);CASE(6);CASE(7);CASE(8);CASE(9);CASE(10);CASE(11);CASE(12);CASE(13);CASE(14);CASE(15);
#undef CASE
}return v;}
alignas(16) static uint8_t dm[4096],expected[4096];
static uint32_t rng=0xd029;static uint32_t random_word(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
extern "C" unsigned duck_test_rsp_samples(){
 unsigned cases=0;
 for(unsigned seed=0;seed<1024;seed++){
  Vector input;for(auto &w:input.words)w=(uint16_t)random_word();
  for(unsigned e=0;e<16;e++){
   Vector out=select(input,e);
   for(unsigned j=0;j<8;j++){unsigned logical=7-j;unsigned chosen=e<2?logical:e<4?((logical&~1u)|(e-2)):e<8?((logical&4)|(e-4)):e-8;
    if(out.words[j]!=input.words[7-chosen])return 0;
   }++cases;
  }
 }
 for(unsigned seed=0;seed<2;seed++){
  for(unsigned i=0;i<4096;i++)dm[i]=expected[i]=(uint8_t)(i*29+(i>>4)*7+seed*71);
  for(unsigned address=0;address<4096;address++){
   uint32_t high=seed?0xffff0000u:0;alignas(16) uint8_t actual[16],want[16],source[8],prior[8];unsigned offset=(address&1)?8:0;
   for(unsigned i=0;i<16;i++)actual[i]=want[i]=(uint8_t)random_word();
   for(unsigned i=0;i<8;i++)source[i]=(uint8_t)random_word();bool valid=address<=4088;
   if(duck_rsp_sample_load8(actual+offset,dm,address|high)!=valid)return 0;
   if(valid)for(unsigned i=0;i<8;i++)want[offset+i]=dm[(address+7-i)^3];
   for(unsigned i=0;i<16;i++)if(actual[i]!=want[i])return 0;++cases;
   if(valid)for(unsigned i=0;i<8;i++){prior[i]=expected[(address+i)^3];expected[(address+i)^3]=source[7-i];}
   if(duck_rsp_sample_store8(dm,address|high,source)!=valid)return 0;
   for(unsigned i=0;i<4096;i++)if(dm[i]!=expected[i])return 0;++cases;
   if(valid)for(unsigned i=0;i<8;i++)dm[(address+i)^3]=expected[(address+i)^3]=prior[i];
  }
 }
 return cases;
}
