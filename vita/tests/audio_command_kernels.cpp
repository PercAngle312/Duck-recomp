#include "duck_vita_audio_commands.h"
#include <cstddef>
extern "C" void *memcpy(void*d,const void*s,size_t n){auto*a=(volatile uint8_t*)d;auto*b=(const volatile uint8_t*)s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
extern "C" void *memset(void*d,int v,size_t n){auto*a=(volatile uint8_t*)d;for(size_t i=0;i<n;i++)a[i]=(uint8_t)v;return d;}
alignas(16) static uint8_t actual[4096],expected[4096];
static uint32_t state=0xd028;static unsigned random_word(){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
static bool equal(){for(unsigned i=0;i<4096;i++)if(actual[i]!=expected[i])return false;return true;}
static int16_t sample(const uint8_t*p){int16_t v;memcpy(&v,p,2);return v;}
static int16_t mixed(int16_t a,int16_t b,int16_t f,int16_t g){int64_t v=(int64_t(a)*f+int64_t(b)*g)*2+32768;v>>=16;return v>32767?32767:v< -32768?-32768:(int16_t)v;}
extern "C" unsigned duck_test_audio_commands(){
 unsigned cases=0;const unsigned lengths[]={0,1,15,16,17,31,32,64,368,512,65535};
 for(unsigned seed=0;seed<2048;seed++){
  for(unsigned i=0;i<4096;i++)actual[i]=expected[i]=(uint8_t)random_word();
  uint32_t address=seed%4096,n=lengths[seed%11],a=address,b=n,bytes=((n?n:1)+15u)&~15u;
  bool fast=bytes>=32&&!(address&3u)&&bytes<=4096-address;
  if(duck_audio_clear_prefix(actual,a,b)!=fast)return 0;
  if(fast){for(unsigned i=0;i<bytes-16;i++)expected[(address+i)^3]=0;if(a!=address+bytes-16||b!=n-(bytes-16))return 0;}
  if(!equal())return 0;++cases;
  for(unsigned i=0;i<4096;i++)actual[i]=expected[i]=(uint8_t)random_word();
  uint32_t source=seed%4096,dest=(seed*7)%4096,c=dest;a=source;b=n;
  fast=bytes>=32&&!((source|dest)&3u)&&bytes<=4096-source&&bytes<=4096-dest;
  if(duck_audio_move_prefix(actual,a,c,b)!=fast)return 0;
  if(fast)for(unsigned j=0;j<bytes-16;j+=16){uint8_t temp[16];for(unsigned i=0;i<16;i++)temp[i]=expected[(source+j+i)^3];for(unsigned i=0;i<16;i++)expected[(dest+j+i)^3]=temp[i];}
  if(!equal())return 0;++cases;
  for(unsigned i=0;i<4096;i++)actual[i]=expected[i]=(uint8_t)random_word();
  duck_audio_interleave_prefix(actual);
  for(unsigned i=0;i<176;i++){int16_t l=sample(expected+((0x9d0+i*2)^2)),r=sample(expected+((0xb40+i*2)^2));memcpy(expected+((0x4f0+i*4)^2),&l,2);memcpy(expected+((0x4f0+i*4+2)^2),&r,2);}
  if(!equal())return 0;++cases;
  for(unsigned i=0;i<4096;i++)actual[i]=expected[i]=(uint8_t)random_word();
  source=0x500+(seed%16)*16;dest=0x500+((seed*7)%16)*16;
  int16_t factor=seed%4==0?-32768:seed%4==1?32767:(int16_t)random_word(),gain=(int16_t)random_word();
  alignas(16) uint8_t av[16],bv[16],ca[16],cb[16],nexta[16],nextb[16];memcpy(ca,expected+dest,16);memcpy(cb,expected+source,16);
  a=dest;c=source;b=368;
  if(!duck_audio_mix_prefix(actual,a,c,b,factor,gain,av,bv))return 0;
  for(unsigned j=0;j<352;j+=16){int16_t values[8];for(unsigned i=0;i<8;i++)values[i]=mixed(sample(ca+i*2),sample(cb+i*2),factor,gain);memcpy(nexta,expected+dest+j+16,16);memcpy(nextb,expected+source+j+16,16);memcpy(expected+dest+j,values,16);memcpy(ca,nexta,16);memcpy(cb,nextb,16);}
  for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)if(av[i*4+j]!=ca[(3-i)*4+j]||bv[i*4+j]!=cb[(3-i)*4+j])return 0;
  if(a!=dest+352||c!=source+352||b!=16||!equal())return 0;++cases;
 }
 return cases;
}
