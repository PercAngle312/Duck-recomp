#include "duck_vita_rsp_multiply.h"
#include <cstdint>
#include <cstring>

// Freestanding test executable only; production uses VitaSDK libc.
extern "C" void *memcpy(void *dst,const void *src,size_t n){
    auto *d=(volatile uint8_t*)dst;auto *s=(const volatile uint8_t*)src;
    for(size_t i=0;i<n;i++)d[i]=s[i];return dst;
}
extern "C" void *memset(void *dst,int value,size_t n){
    auto *d=(volatile uint8_t*)dst;for(size_t i=0;i<n;i++)d[i]=(uint8_t)value;return dst;
}
extern "C" int memcmp(const void *a,const void *b,size_t n){
    auto *x=(const volatile uint8_t*)a;auto *y=(const volatile uint8_t*)b;
    for(size_t i=0;i<n;i++)if(x[i]!=y[i])return x[i]<y[i]?-1:1;return 0;
}

static uint32_t random_state=0x17df3021;
static uint32_t random_word(){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
extern "C" {volatile unsigned duck_multiply_failure;}
template<bool Accumulate,bool Unsigned>
static bool exercise(unsigned iteration,unsigned alias){
    alignas(16) int16_t a[8],b[8];
    alignas(16) uint16_t l[8],m[8],h[8],result[8];
    uint16_t el[8],em[8],eh[8],expected[8];
    constexpr int16_t edge[]={-32768,-32767,-1,0,1,16384,32766,32767};
    for(unsigned i=0;i<8;i++){
        a[i]=iteration<64?edge[(iteration/8+i)%8]:(int16_t)random_word();
        b[i]=iteration<64?edge[iteration%8]:(int16_t)random_word();
        l[i]=(uint16_t)random_word();m[i]=(uint16_t)random_word();h[i]=(uint16_t)random_word();
        // Include carry boundaries and nonsaturated accumulator results.
        if(iteration%4==0)h[i]=(m[i]&0x8000)?0xffff:0;
        if(iteration%8==0)l[i]=m[i]=h[i]=0xffff;
        uint64_t accumulator=Accumulate?((uint64_t)h[i]<<32)|((uint64_t)m[i]<<16)|l[i]:0x8000;
        accumulator+=(int64_t)a[i]*b[i]*2;
        el[i]=(uint16_t)accumulator;em[i]=(uint16_t)(accumulator>>16);eh[i]=(uint16_t)(accumulator>>32);
        int32_t upper=(int32_t)(((uint32_t)eh[i]<<16)|em[i]);
        expected[i]=Unsigned?((int16_t)eh[i]<0?0:eh[i]||(int16_t)em[i]<0?0xffff:em[i]):
            upper>32767?0x7fff:upper< -32768?0x8000:(uint16_t)upper;
    }
    void *dst=alias==0?result:alias==1?(void*)a:(void*)b;
    duck_rsp_multiply<Accumulate,Unsigned>(dst,a,b,l,m,h);
    return !std::memcmp(dst,expected,16)&&!std::memcmp(l,el,16)&&!std::memcmp(m,em,16)&&!std::memcmp(h,eh,16);
}
extern "C" unsigned duck_test_rsp_multiply(){
    random_state=0x17df3021;unsigned count=0;
    for(unsigned i=0;i<8192;i++)for(unsigned alias=0;alias<3;alias++){
#define CHECK(a,u) if(!exercise<a,u>(i,alias)){duck_multiply_failure=count+1;return 0;}count++
        CHECK(false,false);CHECK(false,true);CHECK(true,false);CHECK(true,true);
#undef CHECK
    }
    duck_multiply_failure=0;return count;
}
