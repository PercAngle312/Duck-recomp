#include "duck_vita_sse2neon.h"
// Compile the same signed multiply, shuffle, saturating-add and unaligned
// load/store intrinsics used by the translated ARM32 audio vector backend.
extern "C" void duck_neon_compile_probe(const int16_t *a,const int16_t *b,int16_t *out){
 auto x=_mm_loadu_si128((const __m128i*)a),y=_mm_loadu_si128((const __m128i*)b);
 auto p=_mm_mulhi_epi16(x,y);auto q=_mm_shuffle_epi32(p,0x1b);
 _mm_storeu_si128((__m128i*)out,_mm_adds_epi16(q,x));
}
