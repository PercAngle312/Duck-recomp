// A replay of the first captured game graphics task, not a playable game.
#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/ctrl.h>
#include <cstdio>
#include <cstdlib>
extern "C" int duck_fast3d_init(unsigned char*);
extern "C" int duck_fast3d_replay(unsigned);
unsigned int _newlib_heap_size_user=160*1024*1024;
int main(){
 sceIoMkdir("ux0:data/DuckRecomp",0777);
 freopen("ux0:data/DuckRecomp/graphics-replay.log","w",stderr);setbuf(stderr,nullptr);
 fprintf(stderr,"Duck Vita graphics task replay — NOT THE GAME\n");
 unsigned task=0;FILE *meta=fopen("ux0:data/DuckRecomp/first-task-address.txt","r");
 if(!meta||fscanf(meta,"%x",&task)!=1){fprintf(stderr,"Missing private task address file\n");return 1;}fclose(meta);
 FILE *data=fopen("ux0:data/DuckRecomp/first-task-rdram.private.bin","rb");
 unsigned char *ram=(unsigned char*)malloc(0x800000);
 if(!data||!ram||fread(ram,1,0x800000,data)!=0x800000||fgetc(data)!=EOF){fprintf(stderr,"Expected private 8 MiB RAM snapshot\n");return 2;}fclose(data);
 fprintf(stderr,"Initializing real vitaGL/Fast3D renderer\n");int result=duck_fast3d_init(ram);
 if(result){fprintf(stderr,"Renderer initialization failed: %d\n",result);return 3;}
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
 for(unsigned frame=0;;++frame){
  result=duck_fast3d_replay(task);if(result){fprintf(stderr,"Task replay failed: %d at frame %u\n",result,frame);return 4;}
  if(frame==0)fprintf(stderr,"Task submitted to renderer; visual output requires verification\n");
  SceCtrlData pad={};sceCtrlPeekBufferPositive(0,&pad,1);if(pad.buttons&SCE_CTRL_START)break;
 }
 fprintf(stderr,"Replay exited by Start\n");sceKernelExitProcess(0);return 0;
}
