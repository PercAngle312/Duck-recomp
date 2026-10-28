#include "duck_vita_game_loop.h"
#include <string.h>
int duck_vita_game_loop_attach(DuckVitaGameLoop *loop,DuckVitaScheduler *scheduler,
 DuckVitaRecompStore *contexts,const DuckVitaDispatchTable *functions,
 uint8_t *rdram,DuckVitaGraphicsSubmit renderer){
 if(!loop||!scheduler||!contexts||!rdram||!renderer||
    duck_vita_dispatch_validate(functions)!=0)return -1;
 memset(loop,0,sizeof(*loop));
 loop->scheduler=scheduler;loop->contexts=contexts;
 loop->functions=functions;loop->rdram=rdram;
 if(duck_vita_sp_runtime_attach(&loop->sp,&loop->events,rdram,renderer)!=0){
  memset(loop,0,sizeof(*loop));return -2;
 }
 loop->attached=1;return 0;
}
int duck_vita_game_loop_step(DuckVitaGameLoop *loop,uint32_t verified_vram){
 if(!loop||!loop->attached||!loop->scheduler||!loop->contexts||
    !loop->functions||!loop->rdram)return -1;
 return duck_vita_dispatch_vram(loop->contexts,loop->scheduler,loop->rdram,
                                loop->functions,verified_vram);
}
void duck_vita_game_loop_detach(DuckVitaGameLoop *loop){
 if(!loop)return;
 if(loop->attached)duck_vita_sp_runtime_detach(&loop->sp);
 memset(loop,0,sizeof(*loop));
}
