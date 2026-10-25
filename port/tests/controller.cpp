#include "../runtime/duck_controller_mapping.h"
#include <cstdio>
int main() {
using namespace duck_input;
Pad p; p.a=true; p.start=true; p.up=true; p.z=true; p.cr=true; p.x=2.f; p.y=-2.f;
auto s=map(p);
if(s.buttons!=(A|START|DU|Z|CR)||s.x!=1.f||s.y!=-1.f) {std::fputs("controller conversion failed\n",stderr);return 1;}
if(map(Pad{}).buttons!=0) return 2;
std::puts("controller mapping PASS"); return 0;
}
