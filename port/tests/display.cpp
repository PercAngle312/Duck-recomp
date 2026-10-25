#include "../runtime/duck_display_settings.h"
#include <cstdio>
#define CHECK(cond) do { if (!(cond)) { std::fprintf(stderr,"CHECK failed: %s at line %d\\n",#cond,__LINE__); return 1; } } while(0)
#include <cstdlib>
static int set_value(const char* key,const char* value) {
#ifdef _WIN32
  return _putenv_s(key,value);
#else
  return setenv(key,value,1);
#endif
}
int main(){using duck_display::load;CHECK(load().width==960&&load().display_hz==60);
CHECK(set_value("DUCK_WIDTH","1920")==0);set_value("DUCK_HEIGHT","1080");set_value("DUCK_RENDER_PERCENT","200");set_value("DUCK_MSAA","4");
CHECK(load().width==1920&&load().height==1080&&load().resolution_scale==2.0&&load().msaa==4);
set_value("DUCK_DISPLAY_HZ","120");CHECK(load().display_hz==60);
set_value("DUCK_EXPERIMENTAL_HFR","1");CHECK(load().display_hz==120);
set_value("DUCK_WIDTH","oops");CHECK(load().width==960);
set_value("DUCK_MSAA","3");CHECK(load().msaa==0);
}
