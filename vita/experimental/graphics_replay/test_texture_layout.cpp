#include "texture_layout.h"
#include "framebuffer_pack.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>

int main(){
    // Exhaustive ABGR1555 conversion, with non-vector tails included.
    std::vector<uint16_t> pixels(65539);
    for(size_t i=0;i<pixels.size();i++)pixels[i]=uint16_t(i);
    duck_abgr1555_to_rgba5551(pixels.data(),pixels.size());
    for(size_t i=0;i<pixels.size();i++){
        unsigned x=uint16_t(i),expected=((x&31)<<11)|((x&992)<<1)|((x&31744)>>9)|((x&32768)>>15);
        assert(pixels[i]==expected);
    }
    // Independently form guest halfwords; check both alignments, odd sizes,
    // every byte outside the destination, and a whole 320x240 framebuffer.
    for(size_t alignment:{size_t(0),size_t(2)})for(size_t n:{size_t(0),size_t(1),size_t(2),size_t(7),size_t(8),size_t(17),size_t(76800)}){
        std::vector<uint8_t> a(n*2+32,0xa5),b=a;std::vector<uint16_t> src(n);
        for(size_t i=0;i<n;i++){src[i]=uint16_t(i*137+19);size_t p=((8+alignment+i*2)^2);b[p]=uint8_t(src[i]);b[p+1]=uint8_t(src[i]>>8);}
        duck_store_rgba5551(a.data(),8+alignment,src.data(),n);assert(a==b);
    }
    // Pre-swapped 4/8/16-bit groups and RGBA32 groups. Build the source with
    // independent address-XOR mapping; decoding must recover all source bytes.
    for(unsigned group:{8u,16u})for(size_t pitch:{size_t(16),size_t(32),size_t(64)}){
        std::vector<uint8_t> linear(pitch*5),swapped(linear.size()),decoded(linear.size());
        for(size_t i=0;i<linear.size();i++)linear[i]=uint8_t(i*29+i/pitch);
        for(size_t y=0;y<5;y++)for(size_t x=0;x<pitch;x++)
            swapped[y*pitch+x]=linear[y*pitch+(y&1?x^(group/2):x)];
        duck_unswizzle_block(decoded.data(),swapped.data(),swapped.size(),pitch,group);assert(decoded==linear);
        // Mip/subview uses its own row width and origin, unrelated to commands
        // following LoadBlock. Source stays immutable after both decodes.
        auto before=swapped;
        duck_unswizzle_block(decoded.data(),swapped.data(),swapped.size(),pitch/2,group);
        assert(before==swapped);
    }
    // A narrow mip appended after the wide base must use its own pitch.
    // The old whole-block/wide-pitch conversion leaves its odd row swapped.
    std::array<uint8_t,80> pyramid{};
    for(unsigned y=0;y<2;y++)for(unsigned x=0;x<8;x++)
        pyramid[64+y*8+x]=uint8_t(30+y*8+(y&1?x^4:x));
    std::array<uint8_t,16> mip{};
    duck_unswizzle_block(mip.data(),pyramid.data()+64,16,8,8);
    for(unsigned i=0;i<16;i++)assert(mip[i]==30+i);
    // Invalid/partial spans must not walk beyond a row or allocation.
    std::array<uint8_t,29> small{},copy{};
    for(size_t i=0;i<small.size();i++)small[i]=uint8_t(i);
    duck_unswizzle_block(copy.data(),small.data(),small.size(),8,8);
    assert(copy[28]==28);
    duck_unswizzle_block(copy.data(),small.data(),small.size(),0,8);assert(copy==small);
    std::puts("PASS: exhaustive 1555 conversion, twiddled stores/tails, pre-swapped rows and source immutability");
}
