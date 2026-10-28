#include "combiner_cycle.h"
#include <cassert>
#include <cstdio>
static uint32_t rgb(unsigned a,unsigned b,unsigned c,unsigned d){return (a&15)|((b&15)<<4)|((c&31)<<8)|((d&7)<<13);}
static uint32_t alpha(unsigned a,unsigned b,unsigned c,unsigned d){return (a&7)|((b&7)<<3)|((c&7)<<6)|((d&7)<<9);}
static uint32_t mux(unsigned color,unsigned a){return rgb(15,15,31,color)|(alpha(7,7,7,a)<<16);}
int main(){
    // Deliberately different cycles: shade/opaque then primitive/transparent.
    // The old selector returned shade and the wrong alpha in one-cycle mode.
    uint64_t first=mux(4,6),second=mux(3,7),packed=first|(second<<28);
    uint64_t selected=duck_combiner_shader_mux(packed,true);
    assert(selected==second&&((selected>>13)&7)==3&&((selected>>25)&7)==7);
    assert(duck_combiner_shader_mux(packed,false)==packed);
    // Vary every packed source bit independently, including all alpha fields.
    for(unsigned bit=0;bit<56;bit++){
        uint64_t word=1ull<<bit;
        assert(duck_combiner_shader_mux(word,true)==(bit<28?0:1ull<<(bit-28)));
        assert(duck_combiner_shader_mux(word,false)==word);
    }
    // Identical-cycle macros are unchanged. COPY/FILL synthetic modes keep
    // their first set and must call the helper with oneCycle=false.
    assert(duck_combiner_shader_mux(first|(first<<28),true)==first);
    assert(duck_combiner_shader_mux(first,false)==first);
    assert(duck_combiner_shader_mux(0,true)==0);
    puts("PASS combiner cycle selection, RGB/alpha muxes, all 56 source bits and COPY/FILL preservation");
}
