#include "triangle_math_cache.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <initializer_list>
static uint32_t bits(float f){uint32_t b;std::memcpy(&b,&f,4);return b;}
static uint32_t rng=928273;
static uint32_t next(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(){
 DuckTriangleMathCache c;
 for(unsigned i=0;i<1000000;i++){
  unsigned slot=i&1,n=next(),d=1+next()%65536;
  assert(c.texture_height(slot,n,d)==n/d);assert(c.texture_height(slot,n,d)==n/d);
  unsigned axis=(i>>1)&1;uint32_t dimension=next()%8193;volatile float v=float(dimension);float expected=1.0f/v;
  assert(bits(c.texture_inverse(slot,axis,dimension))==bits(expected));assert(bits(c.texture_inverse(slot,axis,dimension))==bits(expected));
  uint32_t raw=next();float w;std::memcpy(&w,&raw,4);volatile float denom=w;expected=1.0f/denom;
  float first=c.vertex_inverse(i&255,w),second=c.vertex_inverse(i&255,w);
  if(std::isnan(expected)){assert(std::isnan(first)&&std::isnan(second));}
  else {assert(bits(first)==bits(expected)&&bits(second)==bits(expected));}
 }
 for(float w:{0.0f,-0.0f,1.0f,-1.0f,std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}){
  volatile float d=w;float expected=1.0f/d;assert(bits(c.vertex_inverse(3,w))==bits(expected));
 }
 assert(c.hits>=3000000);std::printf("PASS exact triangle divisions: 1000000 changing-input cases, signed zero, infinities and NaNs; hits=%u misses=%u\n",c.hits,c.misses);
}
