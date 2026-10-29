#include "command_scan.h"
#include <cstddef>
extern "C" void *memcpy(void *d,const void *s,size_t n){auto *a=(volatile uint8_t*)d;auto *b=(const volatile uint8_t*)s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
extern "C" void *memset(void *d,int x,size_t n){auto *a=(volatile uint8_t*)d;for(size_t i=0;i<n;i++)a[i]=(uint8_t)x;return d;}
alignas(16) static uint32_t commands[131080];
static uint32_t random_state=0xD027;
static uint32_t random_word(){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
static uint32_t reference(const uint32_t *p,unsigned count,unsigned end,unsigned branch){for(unsigned i=0;i<count;i++)if((p[i*2]>>24)==end||((p[i*2]>>24)==branch&&((p[i*2]>>16)&255)==1))return i+1;return 0;}
extern "C" unsigned duck_test_command_scan(){
 unsigned cases=0;
 for(unsigned gbi2=0;gbi2<2;gbi2++)for(unsigned length=0;length<=256;length++)for(unsigned variant=0;variant<12;variant++){
  for(unsigned i=0;i<2048;i++)commands[i]=random_word();
  unsigned end=gbi2?0xdf:0xb8,branch=gbi2?0xde:0x06;
  unsigned offset=variant&1;uint32_t *p=commands+offset; // 4-byte alignment, including vld2 loads at +4.
  if(length&&variant<8){unsigned pos=(variant<4?variant:length-1)%length;p[pos*2]=variant&2?end<<24:(branch<<24)|((variant&4?0:1)<<16);}
  if(duck_count_commands(p,length,end,branch,1)!=reference(p,length,end,branch))return 0;
  ++cases;
 }
 for(unsigned i=0;i<131080;i++)commands[i]=0;
 commands[65535*2]=0xdf000000u;
 if(duck_count_commands(commands,65536,0xdf,0xde,1)!=65536)return 0;
 if(duck_count_commands(commands,65535,0xdf,0xde,1)!=0)return 0;
 if(duck_count_commands(nullptr,10,0xdf,0xde,1))return 0;
 return cases;
}
