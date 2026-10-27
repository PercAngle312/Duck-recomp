// Shared task renderer for live guest SP submission and the historical replay.
// Compilation and task submission do not establish rendered/gameplay success.
#include "stdafx.h"
#include "Core/Memory.h"
#include "SysVita/HLEGraphics/Accurate/Fast3DMemoryBridge.h"
#include "SysVita/HLEGraphics/Accurate/Fast3DCompat.h"
#include <vitaGL.h>
#include "task_bounds.h"
#include "fast3d_task.h"
#include "graphics_options.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <memory>
#include <psp2/io/stat.h>
#include <psp2/power.h>
#include <psp2/kernel/processmgr.h>
#ifndef DUCK_NATIVE_RESOLUTION
#define DUCK_NATIVE_RESOLUTION 1
#endif
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
static DuckGraphicsOptions graphics;
extern "C" int duck_fast3d_smooth_presentation(void){return graphics.smooth;}
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
 FILE *options=fopen("ux0:data/DuckRecomp/graphics.ini","r");
 graphics.load(options);if(options)fclose(options);
 // Request standard homebrew performance clocks without lowering a higher
 // user-selected setting. Report actual values and rejected requests.
 int cpu=scePowerGetArmClockFrequency(),bus=scePowerGetBusClockFrequency();
 int gpu=scePowerGetGpuClockFrequency(),xbar=scePowerGetGpuXbarClockFrequency();
 int cr=cpu<444?scePowerSetArmClockFrequency(444):0;
 int br=bus<222?scePowerSetBusClockFrequency(222):0;
 int gr=gpu<222?scePowerSetGpuClockFrequency(222):0;
 int xr=xbar<166?scePowerSetGpuXbarClockFrequency(166):0;
 fprintf(stderr,"Performance clocks: CPU=%d BUS=%d GPU=%d XBAR=%d MHz; requests=%d,%d,%d,%d\n",
    scePowerGetArmClockFrequency(),scePowerGetBusClockFrequency(),
    scePowerGetGpuClockFrequency(),scePowerGetGpuXbarClockFrequency(),cr,br,gr,xr);
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
 // App-owned, versioned derived data; never consume another application's
 // program binaries or the older unvalidated Fast3D cache format.
 sceIoMkdir("ux0:data/DuckRecomp/shaders-v007",0777);
 fprintf(stderr,"Initializing Fast3D interpreter\n");
 g_pMemoryBuffers[MEM_RD_RAM]=rdram;
 interpreter=std::make_shared<Fast::Interpreter>();Fast::GfxSetInstance(interpreter);
 interpreter->Init(DaedalusFast3D_GetWindowBackend(),DaedalusFast3D_GetRenderingAPI(),"Duck Vita graphics replay",false,960,544,0,0);
 interpreter->SetRdramMemory(rdram,gRamSize);interpreter->SetNativeDimensions(320,240);
 interpreter->SetDisplayConfiguration(graphics.display_width,graphics.display_height,
     graphics.display_x,0,graphics.aspect_delta);
#if DUCK_NATIVE_RESOLUTION
 // Internal resolution is independent of the final presentation rectangle.
 interpreter->mCurDimensions.width=graphics.width;interpreter->mCurDimensions.height=graphics.height;
#endif
 interpreter->GetCurrentRenderingAPI()->SetTextureFilter(graphics.filter==1?Fast::FILTER_LINEAR:
     graphics.filter==2?Fast::FILTER_NONE:Fast::FILTER_THREE_POINT);
 fprintf(stderr,"Rendering resolution: %ux%u; presentation %ux544; aspect=%s filter=%s\n",
         unsigned(interpreter->mCurDimensions.width),unsigned(interpreter->mCurDimensions.height),
         graphics.display_width,graphics.widescreen?"wide":"original",
         graphics.filter==1?"bilinear":graphics.filter==2?"nearest":"n64");
 bridge.SetInterpreter(interpreter.get());
 fprintf(stderr,"Fast3D interpreter ready\n");return 0;
}
extern "C" int duck_fast3d_submit_task(uint32_t task_address){
 uint64_t started=sceKernelGetProcessTimeWide();
 if(!interpreter)return -1;
 uint32_t p=task_address&0x1fffffffu;if(!checked_address(task_address,64,4)){fprintf(stderr,"Invalid task location: %08x\n",task_address);return -2;}
 uint32_t task[16];memcpy(task,g_pu8RamBase+p,sizeof task);
 if(task[0]!=1){fprintf(stderr,"Unsupported task type: %u\n",task[0]);return -3;}
 uint32_t code=task[4]&0x1fffffffu,data=task[6]&0x1fffffffu;
 if(!checked_address(task[4],task[5],8)||!checked_address(task[6],task[7],8)||!checked_address(task[12],8,8)){
  fprintf(stderr,"Invalid microcode or display-list span: code=%08x data=%08x list=%08x\n",task[4],task[6],task[12]);return -6;
 }
 UcodeInfo ucode=GBIMicrocode_DetectVersion(code,task[5],data,task[7]);
 // The bridge validates every source on first use in this task. No global
 // dirty-page fill or scan of historical texture stages is required.
 static bool first=true;
 if(first)fprintf(stderr,"Translating first display list\n");
 void *list=bridge.BuildDisplayList(task[12],ucode.version);if(!list)return -4;
 uint64_t translated=sceKernelGetProcessTimeWide();
 if(first)fprintf(stderr,"Display list translated; full_sync=%d\n",int(bridge.SawFullSync()));
 Fast::gfx_set_target_ucode(ucode_f3dex2);
 robin_hood::unordered_map<Mtx*,MtxF> replacements;
 // Interpreter::Run binds the draw framebuffer and performs its own clear.
 // External clears here inherit old scissor/depth masks and can clear the
 // previously bound readback target; avoid disturbing cached rendering state.
 GLenum pre_error=glGetError();
 if(pre_error!=GL_NO_ERROR)fprintf(stderr,"GL pre-task error=0x%x\n",unsigned(pre_error));
 interpreter->StartFrame();DaedalusFast3D_PrepareTask(true);
 if(first)fprintf(stderr,"Executing first Fast3D task\n");
 interpreter->Run(reinterpret_cast<Gfx*>(list),replacements);
 uint64_t drawn=sceKernelGetProcessTimeWide();
 if(first)fprintf(stderr,"First Fast3D task finished\n");
 if(bridge.SawFullSync()&&!interpreter->SyncColorImageToRdram(g_pu8RamBase,gRamSize))return -5;
 uint64_t readback=sceKernelGetProcessTimeWide();
 if(interpreter->mRendersToFb){
  // Run() leaves the default framebuffer clear; it doesn't present the game
  // FBO itself. Blit it explicitly, using its actual dimensions and orientation.
  interpreter->GetCurrentRenderingAPI()->CopyFramebuffer(0,interpreter->mGameFb,
      0,0,interpreter->mCurDimensions.width,interpreter->mCurDimensions.height,
      graphics.display_x,0,graphics.display_x+graphics.display_width,graphics.display_height);
 }
 interpreter->EndFrame();
 GLenum render_error=glGetError();
 if(first)fprintf(stderr,"Fast3D GL post-task error=0x%x\n",unsigned(render_error));
 if(render_error!=GL_NO_ERROR)return -7;
 vglSwapBuffers(GL_FALSE);
 uint64_t finished=sceKernelGetProcessTimeWide();
 static uint64_t last_report,total_translate,total_draw,total_readback,total_present;
 static unsigned measured_tasks;
 if(!last_report)last_report=started;
 total_translate+=translated-started;total_draw+=drawn-translated;
 total_readback+=readback-drawn;total_present+=finished-readback;++measured_tasks;
 if(finished-last_report>=5000000){
  fprintf(stderr,"Graphics timing: tasks=%u avg_us translate=%llu draw=%llu readback=%llu present=%llu light_updates=%u vertex_rgba_updates=%u texture_hits=%u texture_uploads=%u\n",
      measured_tasks,(unsigned long long)(total_translate/measured_tasks),
      (unsigned long long)(total_draw/measured_tasks),(unsigned long long)(total_readback/measured_tasks),
      (unsigned long long)(total_present/measured_tasks),interpreter->mDuckLightColorUpdates,interpreter->mDuckVertexColorUpdates,interpreter->mDuckTextureHits,interpreter->mDuckTextureUploads);
  last_report=finished;total_translate=total_draw=total_readback=total_present=0;measured_tasks=0;
 }
 if(first)fprintf(stderr,"First framebuffer swap complete\n");
 first=false;return bridge.SawFullSync()?0:1;
}

// Compatibility with the historical replay caller. The same renderer can now
// accept distinct graphics task addresses supplied by a live SP task handler.
extern "C" int duck_fast3d_replay(uint32_t task_address){
 int result=duck_fast3d_submit_task(task_address);
 return result==1?0:result;
}
