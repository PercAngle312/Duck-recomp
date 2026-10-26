#include "material_updates.h"
#include <cassert>
#include <cstring>
#include <cstdio>
struct Light {struct {uint8_t col[3],pad,colc[3],pad2;int8_t dir[3];}l;};
struct Color {uint8_t r,g,b,a;};
int main(){
 Light lights[8];memset(lights,0x5a,sizeof lights);
 assert(duck_update_light_color(lights,0,0xff000000,24));
 assert(lights[0].l.col[0]==255&&!lights[0].l.col[1]&&!lights[0].l.col[2]);
 assert(duck_update_light_color(lights,0,0x000000ff,24)); // black must replace red
 assert(!lights[0].l.col[0]&&!lights[0].l.col[1]&&!lights[0].l.col[2]);
 assert(duck_update_light_color(lights,24,0xffffff00,24));
 assert(lights[1].l.col[0]==255&&lights[1].l.col[1]==255&&lights[1].l.col[2]==255);
 assert(duck_update_light_color(lights,7*24+4,0x12345678,24));
 assert(lights[7].l.colc[0]==0x12&&lights[7].l.colc[1]==0x34&&lights[7].l.colc[2]==0x56);
 assert(duck_update_light_color(lights,7*32,0xabcdef00,32));
 assert(lights[7].l.col[0]==0xab&&lights[7].l.col[1]==0xcd&&lights[7].l.col[2]==0xef);
 Light before[8];memcpy(before,lights,sizeof lights);
 assert(!duck_update_light_color(lights,8*24,0,24));assert(!duck_update_light_color(lights,8,0,24));
 assert(!duck_update_light_color(lights,0,0,0));assert(!memcmp(before,lights,sizeof lights));
 assert(lights[0].l.dir[0]==0x5a&&lights[0].l.pad==0x5a);
 Color c;duck_update_vertex_color(c,0x12345678);assert(c.r==0x12&&c.g==0x34&&c.b==0x56&&c.a==0x78);
 const uint8_t black[]={0,0,0,255},red[]={255,0,0,255},green[]={0,255,0,255},blue[]={0,0,255,255},white[]={255,255,255,255},clear[]={255,0,0,0};
 assert(duck_rgba8_to_rgba5551(black)==0x0001);assert(duck_rgba8_to_rgba5551(red)==0xf801);
 assert(duck_rgba8_to_rgba5551(green)==0x07c1);assert(duck_rgba8_to_rgba5551(blue)==0x003f);
 assert(duck_rgba8_to_rgba5551(white)==0xffff);assert(duck_rgba8_to_rgba5551(clear)==0xf800);
 puts("PASS: red-to-black material update, ambient/last slots, color copies, both GBI strides, preserved directions and RGBA channels");
}
