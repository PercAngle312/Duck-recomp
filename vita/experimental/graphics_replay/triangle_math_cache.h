#pragma once
#include <cstdint>
#include <cstring>
// Exact memoization only: keys include every operand, no approximations or
// dependency on a missing guest-memory invalidation hook.
struct DuckTriangleMathCache {
    struct Ratio {uint32_t numerator=0,denominator=0,value=0;bool valid=false;} height[2];
    struct Inverse {uint32_t key=0;float value=0;bool valid=false;} dimensions[4],vertex[256];
    uint32_t hits=0,misses=0;
    uint32_t texture_height(unsigned slot,uint32_t n,uint32_t d){
        Ratio &e=height[slot];
        if(e.valid&&e.numerator==n&&e.denominator==d){++hits;return e.value;}
        ++misses;e.numerator=n;e.denominator=d;e.value=n/d;e.valid=true;return e.value;
    }
    float inverse(Inverse &e,float value){
        uint32_t key;std::memcpy(&key,&value,sizeof key);
        if(e.valid&&e.key==key){++hits;return e.value;}
        ++misses;e.key=key;e.value=1.0f/value;e.valid=true;return e.value;
    }
    float texture_inverse(unsigned slot,unsigned axis,uint32_t value){return inverse(dimensions[slot*2+axis],float(value));}
    float vertex_inverse(unsigned index,float w){return inverse(vertex[index],w);}
};
