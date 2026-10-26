// Shared task renderer for live guest SP submission and the historical replay.
// Compilation and task submission do not establish rendered/gameplay success.
#include "stdafx.h"
#include "Core/Memory.h"
#include "SysVita/HLEGraphics/Accurate/Fast3DMemoryBridge.h"
#include "SysVita/HLEGraphics/Accurate/Fast3DCompat.h"
#include <vitaGL.h>
#include "task_bounds.h"
#include "fast3d_task.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <memory>
u32 gRamSize=0x800000;
void *g_pMemoryBuffers[NUM_MEM_BUFFERS]={};
u8 g_RDRAMDirtyPages[RDRAM_DIRTY_PAGE_COUNT]={};
int gPostProcessing=0;
void RDRAM_MarkDirtyRange(u32 address,u32 size){
 address&=0x7fffff;if(!size||address>=gRamSize)return;
 uint64_t last=(uint64_t)address+size-1;if(last>=gRamSize)last=gRamSize-1;
 for(u32 i=address>>RDRAM_DIRTY_PAGE_SHIFT;i<=(u32(last)>>RDRAM_DIRTY_PAGE_SHIFT);++i)g_RDRAMDirtyPages[i]=1;
}
void RDRAM_ClearDirtyPages(){memset(g_RDRAMDirtyPages,0,sizeof g_RDRAMDirtyPages);}
// This adapter presents to vitaGL's default framebuffer. Postprocessing is off.
u32 GraphicsContextVita_GetPostProcessFramebuffer(){return 0;}
static std::shared_ptr<Fast::Interpreter> interpreter;
static Fast3DMemoryBridge bridge;
static bool init_attempted;
// The pinned vitaGL exports this initializer; it sets its own compiler state
// and tries both supported module paths. GL_SHADER_COMPILER is always true in
// that revision, so querying the GL capability cannot detect a missing module.
extern "C" GLboolean start_shader_compiler(void);
static void shader_log(const char *message,shark_log_level level,int line){
 fprintf(stderr,"shader[%d] line %d: %s\n",int(level),line,message?message:"(no message)");
}
// Reject wrapped, unaligned and out-of-RDRAM task spans before Fast3D sees them.
static bool range(u32 address,u32 bytes){return address<gRamSize&&bytes<=gRamSize-address;}
static bool checked_address(u32 guest,u32 size,u32 alignment){
 return duck_task_span_valid(guest,size,alignment,gRamSize)!=0;
}
UcodeInfo GBIMicrocode_DetectVersion(u32 code,u32 code_size,u32 data,u32 data_size){
 if(!range(code,code_size)||!range(data,data_size)||!data_size||data_size>4096){fprintf(stderr,"Invalid microcode span code=%08x bytes=%u data=%08x bytes=%u\n",code,code_size,data,data_size);abort();}
 char text[4097];for(u32 i=0;i<data_size;i++)text[i]=(char)g_pu8RamBase[(data+i)^3u];text[data_size]=0;
 // Whitelist the task captured from actual game startup. Unknown microcode
 // fails explicitly; never interpret a different microcode as F3DEX2.
 static const char signature[]="RSP Gfx ucode F3DEX.NoN   fifo 2.08";
 for(u32 i=0;i+sizeof(signature)-1<=data_size;i++)if(!memcmp(text+i,signature,sizeof(signature)-1))return {nullptr,GBI_2};
 fprintf(stderr,"Unsupported graphics microcode\n");abort();
}
extern "C" int duck_fast3d_init(uint8_t *rdram){
 if(!rdram||init_attempted)return -1;
 init_attempted=true;
 // At the pinned vitaGL revision, this returns resolution-fallback state,
 // not initialization success. GL_FALSE is the normal native-size result.
 GLboolean fallback=vglInitExtended(0,960,544,64*1024*1024,SCE_GXM_MULTISAMPLE_NONE);
 fprintf(stderr,"vitaGL returned resolution_fallback=%u (0 is normal)\n",unsigned(fallback));
 GLint viewport[4]={};glGetIntegerv(GL_VIEWPORT,viewport);
 fprintf(stderr,"vitaGL viewport: %d %d %d %d; GL error=0x%x\n",viewport[0],viewport[1],viewport[2],viewport[3],unsigned(glGetError()));
 shark_install_log_cb(shader_log);
 fprintf(stderr,"Initializing runtime shader compiler\n");
 if(!start_shader_compiler()){
  fprintf(stderr,"Graphics unavailable: could not initialize libshacccg.suprx.\n"
                 "Checked ur0:/data/libshacccg.suprx and ur0:data/external/libshacccg.suprx.\n"
                 "Guest startup stopped before shader submission. No game frame rendered.\n");
  fflush(stderr);
  return -8;
 }
 fprintf(stderr,"Runtime shader compiler initialized; shader compilation still requires validation\n");
 fprintf(stderr,"Initializing Fast3D interpreter\n");
 g_pMemoryBuffers[MEM_RD_RAM]=rdram;
 interpreter=std::make_shared<Fast::Interpreter>();Fast::GfxSetInstance(interpreter);
 interpreter->Init(DaedalusFast3D_GetWindowBackend(),DaedalusFast3D_GetRenderingAPI(),"Duck Vita graphics replay",false,960,544,0,0);
 interpreter->SetRdramMemory(rdram,gRamSize);interpreter->SetNativeDimensions(320,240);
 interpreter->SetDisplayConfiguration(725,544,117,0,1.0f);
 bridge.SetInterpreter(interpreter.get());
 fprintf(stderr,"Fast3D interpreter ready\n");return 0;
}
extern "C" int duck_fast3d_submit_task(uint32_t task_address){
 if(!interpreter)return -1;
 uint32_t p=task_address&0x1fffffffu;if(!checked_address(task_address,64,4)){fprintf(stderr,"Invalid task location: %08x\n",task_address);return -2;}
 uint32_t task[16];memcpy(task,g_pu8RamBase+p,sizeof task);
 if(task[0]!=1){fprintf(stderr,"Unsupported task type: %u\n",task[0]);return -3;}
 uint32_t code=task[4]&0x1fffffffu,data=task[6]&0x1fffffffu;
 if(!checked_address(task[4],task[5],8)||!checked_address(task[6],task[7],8)||!checked_address(task[12],8,8)){
  fprintf(stderr,"Invalid microcode or display-list span: code=%08x data=%08x list=%08x\n",task[4],task[6],task[12]);return -6;
 }
 UcodeInfo ucode=GBIMicrocode_DetectVersion(code,task[5],data,task[7]);
 // All guest writes are dirty until translated-store instrumentation is added.
 memset(g_RDRAMDirtyPages,1,sizeof g_RDRAMDirtyPages);
 static bool first=true;
 if(first)fprintf(stderr,"Translating first display list\n");
 void *list=bridge.BuildDisplayList(task[12],ucode.version);if(!list)return -4;
 if(first)fprintf(stderr,"Display list translated; full_sync=%d\n",int(bridge.SawFullSync()));
 Fast::gfx_set_target_ucode(ucode_f3dex2);
 robin_hood::unordered_map<Mtx*,MtxF> replacements;
 // Ensure stale backbuffer pixels cannot masquerade as successful graphics.
 glViewport(117,0,725,544);
 glClearColor(0.015f,0.02f,0.045f,1.0f);
 glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
 GLenum pre_error=glGetError();
 if(pre_error!=GL_NO_ERROR)fprintf(stderr,"GL pre-task error=0x%x\n",unsigned(pre_error));
 interpreter->StartFrame();DaedalusFast3D_PrepareTask(true);
 if(first)fprintf(stderr,"Executing first Fast3D task\n");
 interpreter->Run(reinterpret_cast<Gfx*>(list),replacements);
 if(first)fprintf(stderr,"First Fast3D task finished\n");
 if(bridge.SawFullSync()&&!interpreter->SyncColorImageToRdram(g_pu8RamBase,gRamSize))return -5;
 interpreter->EndFrame();
 GLenum render_error=glGetError();
 if(first)fprintf(stderr,"Fast3D GL post-task error=0x%x\n",unsigned(render_error));
 if(render_error!=GL_NO_ERROR)return -7;
 vglSwapBuffers(GL_FALSE);
 if(first)fprintf(stderr,"First framebuffer swap complete\n");
 first=false;return bridge.SawFullSync()?0:1;
}

// Compatibility with the historical replay caller. The same renderer can now
// accept distinct graphics task addresses supplied by a live SP task handler.
extern "C" int duck_fast3d_replay(uint32_t task_address){
 int result=duck_fast3d_submit_task(task_address);
 return result==1?0:result;
}
