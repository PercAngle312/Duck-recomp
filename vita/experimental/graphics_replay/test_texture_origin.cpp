#include "material_updates.h"
#include "texture_origin_generated.h"
#include <cassert>
#include <cstdio>
int main(){
 unsigned cases=0;
 for(unsigned size=0;size<4;size++)for(unsigned width=1;width<=33;width++){
  unsigned row=duck_texture_bytes(width,size),tmem=(row+7)/8*8;
  // A real one-row tile has explicit source pitch, even when TMEM pads it.
  unsigned actual=GetEffectiveLineSize(row,row,row,tmem,false);
  assert(actual==row&&row/actual==1);
  assert(duck_texture_source_pitch(row,row,row,tmem,false)==row);
  // Keep the width=1 LoadBlock sentinel behavior separate.
  assert(GetEffectiveLineSize(row,row,row,tmem,true)==tmem);
  assert(duck_texture_source_pitch(row,row,row,tmem,true)==tmem);cases++;
 }
 uint8_t red[]={0xf8,0x01},out[4]={};
 unsigned line=GetEffectiveLineSize(2,2,2,8,false);
 duck_decode_rgba16_rect(red,line/2,2/line,2,out);
 assert(out[0]==255&&out[1]==0&&out[2]==0&&out[3]==255);
 uint8_t indices[]={0x01,0x23,0x45,0x67};
 line=GetEffectiveLineSize(4,4,4,8,false);
 assert(line*2==8&&4/line==1);
 for(unsigned x=0;x<8;x++)assert(duck_texture_nibble(indices,line,x,0,0)==x);
 std::printf("PASS: %u tile/block origin cases plus one-row RGBA16 and CI4 fixtures\n",cases);
}
