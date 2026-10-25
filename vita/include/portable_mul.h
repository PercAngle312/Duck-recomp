#ifndef DUCK_PORTABLE_MUL_H
#define DUCK_PORTABLE_MUL_H
#include <stdint.h>
/* Exact 64x64 -> 128-bit product using four ARM32-friendly limbs. */
static inline void duck_dmultu(uint64_t a,uint64_t b,uint64_t* lo,uint64_t* hi) {
    uint64_t p0=(uint64_t)(uint32_t)a*(uint32_t)b;
    uint64_t p1=(uint64_t)(uint32_t)a*(b>>32);
    uint64_t p2=(a>>32)*(uint32_t)b;
    uint64_t p3=(a>>32)*(b>>32);
    uint64_t middle=(p0>>32)+(uint32_t)p1+(uint32_t)p2;
    *lo=(middle<<32)|(uint32_t)p0;
    *hi=p3+(p1>>32)+(p2>>32)+(middle>>32);
}
static inline void duck_dmult(int64_t a,int64_t b,int64_t* lo,int64_t* hi) {
    uint64_t low,high;
    duck_dmultu((uint64_t)a,(uint64_t)b,&low,&high);
    high-=(a<0?(uint64_t)b:0)+(b<0?(uint64_t)a:0);
    *lo=(int64_t)low;*hi=(int64_t)high;
}
#endif
