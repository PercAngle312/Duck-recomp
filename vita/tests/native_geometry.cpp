#include "native_geometry.h"
#include <cstddef>
extern "C" void *memcpy(void *d,const void *s,size_t n){auto*a=(volatile uint8_t*)d;auto*b=(const volatile uint8_t*)s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
extern "C" void *memset(void *d,int x,size_t n){auto*a=(volatile uint8_t*)d;for(size_t i=0;i<n;i++)a[i]=(uint8_t)x;return d;}
extern "C" int fegetround(){unsigned f;asm volatile("vmrs %0, fpscr":"=r"(f));return f&0xc00000;}
static void rounding(unsigned mode){unsigned f;asm volatile("vmrs %0, fpscr":"=r"(f));f=(f&~0xc00000u)|mode;asm volatile("vmsr fpscr, %0"::"r"(f));}
static uint32_t state=0xd028;static uint32_t random_word(){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
static bool same(float a,float b){uint32_t x,y;memcpy(&x,&a,4);memcpy(&y,&b,4);return x==y;}
extern "C" unsigned duck_test_native_geometry(){
 unsigned comparisons=0;rounding(FE_TONEAREST);
 for(unsigned n=0;n<16384;n++){
  alignas(16) float matrix[4][4],actual[16],expected[16];alignas(16) uint32_t raw[16];
  for(unsigned i=0;i<16;i++){uint32_t bits=(random_word()&0x807fffffu)|((67+random_word()%121)<<23);if(n%32==0)bits&=0x80000000u;memcpy(&matrix[i/4][i%4],&bits,4);raw[i]=random_word();}
  if(!duck_native_position_matrix(matrix))return 0;
  float x=(int16_t)random_word(),y=(int16_t)random_word(),z=(int16_t)random_word();
  duck_position_transform(actual,matrix,x,y,z);
  for(unsigned j=0;j<4;j++){expected[j]=((x*matrix[0][j]+y*matrix[1][j])+z*matrix[2][j])+matrix[3][j];if(!same(actual[j],expected[j]))return 0;comparisons++;}
  duck_fixed_matrix(actual,raw);
  for(unsigned i=0;i<8;i++){expected[i*2]=(float)(int32_t)((raw[i]&0xffff0000u)|(raw[i+8]>>16))/65536.0f;expected[i*2+1]=(float)(int32_t)((raw[i]<<16)|(raw[i+8]&0xffffu))/65536.0f;}
  for(unsigned i=0;i<16;i++){if(!same(actual[i],expected[i]))return 0;comparisons++;}
 }
 float m[4][4]={};const uint32_t bad[]={1,0x00800000u,0x7f800000u,0x7fc00001u,0x7fffffffu};
 for(uint32_t bits:bad){memcpy(&m[0][0],&bits,4);if(duck_native_position_matrix(m))return 0;}
 m[0][0]=1;
 const unsigned modes[]={0x400000u,0x800000u,0xc00000u};
 for(unsigned mode:modes){rounding(mode);if(duck_native_position_matrix(m))return 0;}
 rounding(FE_TONEAREST);return comparisons;
}
