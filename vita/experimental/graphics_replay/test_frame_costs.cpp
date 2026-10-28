#include "fps_counter.h"
#include <cassert>
#include <cstdio>
int main(){
 DuckFpsCounter fps;for(unsigned i=0;i<17;i++)fps.presented(uint64_t(i)*33333);
 assert(fps.ready&&fps.value==30&&fps.frame_us==33333);fps.reset();assert(!fps.ready);
 DuckFrameCosts costs;costs.sample(100,200,300,400,1000);
 costs.sample(491000,492000,494000,499000,501000);
 assert(costs.guest_us==490000&&costs.translate_us==1000&&costs.draw_us==2000&&costs.read_us==5000);
 costs.reset();assert(!costs.previous_end&&!costs.count&&!costs.guest_us);
 puts("PASS presented FPS window, stage averages and settings pause reset");
}
