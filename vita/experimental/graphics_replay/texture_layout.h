#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

// LoadBlock with DXT=0 is the pre-swapped libultra *BlockS layout. Resolve
// its odd-row word exchange using the sampling tile's pitch, not a guess
// from the next few display-list commands. RGBA32 exchanges 8-byte halves.
inline void duck_unswizzle_block(uint8_t *dst,const uint8_t *src,size_t size,
                                 size_t pitch,unsigned group){
    std::memcpy(dst,src,size);
    if(!pitch || (group!=8 && group!=16) || pitch<group)return;
    const size_t half=group/2;
    for(size_t row=1;row<size/pitch;row+=2){
        uint8_t *p=dst+row*pitch;
        for(size_t x=0;x+group<=pitch;x+=group)
            for(size_t b=0;b<half;b++){
                uint8_t tmp=p[x+b];p[x+b]=p[x+half+b];p[x+half+b]=tmp;
            }
    }
}
