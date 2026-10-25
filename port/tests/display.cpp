#include "../runtime/duck_display_settings.h"
#include <cassert>
#include <cstdlib>
static void set_value(const char* key,const char* value) {
#ifdef _WIN32
  assert(_putenv_s(key,value)==0);
#else
  assert(setenv(key,value,1)==0);
#endif
}
int main(){using duck_display::load;assert(load().width==960&&load().display_hz==60);
set_value("DUCK_WIDTH","1920");set_value("DUCK_HEIGHT","1080");set_value("DUCK_RENDER_PERCENT","200");set_value("DUCK_MSAA","4");
assert(load().width==1920&&load().height==1080&&load().resolution_scale==2.0&&load().msaa==4);
set_value("DUCK_DISPLAY_HZ","120");assert(load().display_hz==60);
set_value("DUCK_EXPERIMENTAL_HFR","1");assert(load().display_hz==120);
set_value("DUCK_WIDTH","oops");assert(load().width==960);
set_value("DUCK_MSAA","3");assert(load().msaa==0);
}
