#pragma once
#include <algorithm>
#include <cstdint>
namespace duck_input {
enum Button : uint16_t { A=0x8000,B=0x4000,Z=0x2000,START=0x1000,DU=0x0800,DD=0x0400,DL=0x0200,DR=0x0100,L=0x0020,R=0x0010,CU=0x0008,CD=0x0004,CL=0x0002,CR=0x0001 };
struct Pad {bool a=false,b=false,z=false,start=false,up=false,down=false,left=false,right=false,l=false,r=false,cu=false,cd=false,cl=false,cr=false;float x=0,y=0;};
struct State {uint16_t buttons=0;float x=0,y=0;};
inline State map(const Pad& p) {State s;
if(p.a)s.buttons|=A;if(p.b)s.buttons|=B;if(p.z)s.buttons|=Z;if(p.start)s.buttons|=START;
if(p.up)s.buttons|=DU;if(p.down)s.buttons|=DD;if(p.left)s.buttons|=DL;if(p.right)s.buttons|=DR;
if(p.l)s.buttons|=L;if(p.r)s.buttons|=R;if(p.cu)s.buttons|=CU;if(p.cd)s.buttons|=CD;if(p.cl)s.buttons|=CL;if(p.cr)s.buttons|=CR;
s.x=std::clamp(p.x,-1.f,1.f);s.y=std::clamp(p.y,-1.f,1.f);return s;}
}
