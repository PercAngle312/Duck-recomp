// Experimental task replay adapter for the pinned Daedalus Fast3D backend.
// Not wired into the guest SP runtime; does not claim rendered/gameplay success.
#include "stdafx.h"
#include "Core/Memory.h"
#include "SysVita/HLEGraphics/Accurate/Fast3DMemoryBridge.h"
#include "SysVita/HLEGraphics/Accurate/Fast3DCompat.h"
#include <vitaGL.h>
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
static bool range(u32 address,u32 bytes){return address<gRamSize&&bytes<=gRamSize-address;}
UcodeInfo GBIMicrocode_DetectVersion(u32 code,u32 code_size,u32 data,u32 data_size){
 if(!range(code,code_size)||!range(data,data_size)||!data_size||data_size>4096)abort();
 char text[4097];for(u32 i=0;i<data_size;i++)text[i]=(char)g_pu8RamBase[(data+i)^3u];text[data_size]=0;
 // Whitelist the task captured from actual game startup. Unknown microcode
 // fails explicitly; never interpret a different microcode as F3DEX2.
 for(u32 i=0;i+64<data_size;i++)if(!memcmp(text+i,"RSP Gfx ucode F3DEX.NoN   fifo 2.08",33))return {nullptr,GBI_2};
 fprintf(stderr,"Unsupported graphics microcode\n");abort();
}
extern "C" int duck_fast3d_init(uint8_t *rdram){
 if(!rdram||interpreter)return -1;
 if(!vglInitExtended(0,960,544,64*1024*1024,SCE_GXM_MULTISAMPLE_NONE))return -2;
 g_pMemoryBuffers[MEM_RD_RAM]=rdram;
 interpreter=std::make_shared<Fast::Interpreter>();Fast::GfxSetInstance(interpreter);
 interpreter->Init(DaedalusFast3D_GetWindowBackend(),DaedalusFast3D_GetRenderingAPI(),"Duck Vita graphics replay",false,960,544,0,0);
 interpreter->SetRdramMemory(rdram,gRamSize);interpreter->SetNativeDimensions(320,240);
 interpreter->SetDisplayConfiguration(725,544,117,0,1.0f);
 bridge.SetInterpreter(interpreter.get());return 0;
}
extern "C" int duck_fast3d_replay(uint32_t task_address){
 if(!interpreter)return -1;
 uint32_t p=task_address&0x1fffffffu;if(!range(p,64)||(p&3))return -2;
 uint32_t task[16];memcpy(task,g_pu8RamBase+p,sizeof task);
 if(task[0]!=1)return -3;
 uint32_t code=task[4]&0x1fffffffu,data=task[6]&0x1fffffffu;
 UcodeInfo ucode=GBIMicrocode_DetectVersion(code,task[5],data,task[7]);
 // All guest writes are dirty until translated-store instrumentation is added.
 memset(g_RDRAMDirtyPages,1,sizeof g_RDRAMDirtyPages);
 void *list=bridge.BuildDisplayList(task[12],ucode.version);if(!list)return -4;
 Fast::gfx_set_target_ucode(ucode_f3dex2);
 robin_hood::unordered_map<Mtx*,MtxF> replacements;
 interpreter->StartFrame();DaedalusFast3D_PrepareTask(true);
 interpreter->Run(reinterpret_cast<Gfx*>(list),replacements);
 if(bridge.SawFullSync()&&!interpreter->SyncColorImageToRdram(g_pu8RamBase,gRamSize))return -5;
 interpreter->EndFrame();vglSwapBuffers(GL_FALSE);return 0;
}
