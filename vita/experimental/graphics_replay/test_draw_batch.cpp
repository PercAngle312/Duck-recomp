#include "draw_batch.h"
#include "frame_uniform_cache.h"
#include <cassert>
#include <cstdio>
int main(){
 for(size_t n=1;n<=1024;n++)for(size_t floats=1;floats<=64;floats++){
  assert(duck_vertex_batch_valid(floats,n*3*floats,n));assert(!duck_vertex_batch_valid(floats,n*3*floats+1,n));
 }
 assert(!duck_vertex_batch_valid(0,0,1)&&!duck_vertex_batch_valid(65,195,1));
 assert(!duck_vertex_batch_valid(4,1025*12,1025));assert(!duck_vertex_batch_valid(4,0,0));
 DuckUniformStamp stamp;assert(stamp.changed(1,1.0f));
 for(unsigned i=0;i<10000;i++)assert(!stamp.changed(1,1.0f));
 assert(stamp.changed(2,1.0f)&&stamp.changed(2,0.5f));assert(!stamp.changed(2,0.5f));
 assert(stamp.changed(2,-0.0f)&&stamp.changed(2,0.0f));
 DuckUniformStamp recreated;assert(recreated.changed(2,0.0f));
 puts("PASS 1024-triangle batch bounds, overflow rejection and per-program uniform cache invalidation");
}
