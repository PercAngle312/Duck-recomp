// Freestanding ARM32 fixture: no Vita services or game data. Called by an
// ARM instruction runner; returns zero after testing production NEON helpers.
#include "framebuffer_pack.h"
extern "C" void *memcpy(void *d,const void *s,size_t n){
    auto *dst=(volatile uint8_t*)d;auto *src=(const volatile uint8_t*)s;
    for(size_t i=0;i<n;i++)dst[i]=src[i];return d;
}
alignas(16) static uint16_t pixels[65539];
alignas(16) static uint8_t ram[131104];
extern "C" int duck_framebuffer_neon_test(){
    for(unsigned i=0;i<65539;i++)pixels[i]=uint16_t(i);
    duck_abgr1555_to_rgba5551(pixels,65539);
    for(unsigned i=0;i<65539;i++){
        unsigned x=uint16_t(i),expected=((x&31)<<11)|((x&992)<<1)|((x&31744)>>9)|((x&32768)>>15);
        if(pixels[i]!=expected)return 1;
    }
    const unsigned sizes[]={0,1,2,7,8,17,65539};
    for(unsigned alignment=0;alignment<=2;alignment+=2)for(unsigned count:sizes){
        for(unsigned i=0;i<sizeof ram;i++)ram[i]=0xa5;
        duck_store_rgba5551(ram,8+alignment,pixels,count);
        for(unsigned i=0;i<count;i++){
            unsigned p=(8+alignment+i*2)^2;
            if(ram[p]!=uint8_t(pixels[i])||ram[p+1]!=uint8_t(pixels[i]>>8))return 2;
        }
        // Confirm all bytes outside the (possibly halfword-twiddled) range.
        for(unsigned p=0;p<sizeof ram;p++){
            unsigned logical=p^2;
            if((logical<8+alignment||logical>=8+alignment+count*2)&&ram[p]!=0xa5)return 3;
        }
    }
    return 0;
}
