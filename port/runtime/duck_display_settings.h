#pragma once
#include <cstdlib>
#include <algorithm>
namespace duck_display {
struct Settings { int width=960,height=720,msaa=0,display_hz=60; double resolution_scale=1.0; bool fullscreen=false,widescreen=false,experimental_hfr=false; };
inline int number(const char* key,int fallback,int lo,int hi) { const char* v=std::getenv(key);if(!v||!*v)return fallback; char* end=nullptr;long n=std::strtol(v,&end,10);return end==v||*end||n<lo||n>hi?fallback:static_cast<int>(n); }
inline Settings load() { Settings s; s.width=number("DUCK_WIDTH",960,320,7680);s.height=number("DUCK_HEIGHT",720,240,4320);
s.fullscreen=number("DUCK_FULLSCREEN",0,0,1)!=0;s.widescreen=number("DUCK_WIDESCREEN",0,0,1)!=0;
s.msaa=number("DUCK_MSAA",0,0,8);if(s.msaa!=0&&s.msaa!=2&&s.msaa!=4&&s.msaa!=8)s.msaa=0;
s.resolution_scale=number("DUCK_RENDER_PERCENT",100,50,400)/100.0;
s.experimental_hfr=number("DUCK_EXPERIMENTAL_HFR",0,0,1)!=0;
s.display_hz=s.experimental_hfr?number("DUCK_DISPLAY_HZ",60,30,240):60;return s; }
}
