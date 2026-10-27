#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <initializer_list>
extern "C" unsigned duck_rsp_memory_reference(unsigned,unsigned,unsigned,uint8_t*);
extern "C" unsigned duck_rsp_memory_fast(unsigned,unsigned,unsigned,uint8_t*);
alignas(16) uint8_t dmem[4096];
uint16_t rspReciprocals[512],rspInverseSquareRoots[512];
int main(){
 uint8_t a[4112],b[4112];unsigned cases=0;
 for(unsigned e=0;e<16;e++)for(unsigned address=0;address<4096;address++)for(unsigned op=0;op<8;op++){
  unsigned n=duck_rsp_memory_reference(e,address,op,a);
  assert(n==duck_rsp_memory_fast(e,address,op,b));
  if(std::memcmp(a,b,n)){std::fprintf(stderr,"Mismatch e=%u address=%u op=%u\n",e,address,op);return 1;}
  cases++;
 }
 for(unsigned e=0;e<16;e++)for(unsigned address:{0xfffffff0u,0xffffffffu,0x80000ff8u})for(unsigned op=0;op<8;op++){
  unsigned n=duck_rsp_memory_reference(e,address,op,a);assert(n==duck_rsp_memory_fast(e,address,op,b));assert(!std::memcmp(a,b,n));cases++;
 }
 std::printf("PASS: %u reference/patched RSP memory cases; all elements and DMEM addresses, wraps and negative offsets\n",cases);
}
