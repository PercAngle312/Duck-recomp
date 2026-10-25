#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include "debugScreen.h"
#include "recomp.h"
#include "portable_mul.h"

unsigned int _newlib_heap_size_user=64*1024*1024;
int duck_boot_smoke_main(void);
int duck_module_execution_main(void);
void duck_vita_umul(uint64_t,uint64_t,uint64_t*,uint64_t*);
void duck_vita_smul(int64_t,int64_t,int64_t*,int64_t*);
static int alias_test(void) {
    uint8_t* rdram=calloc(1,8*1024*1024);
    if(!rdram)return 1;
    MEM_W(0x1000,0xFFFFFFFF80000000ull)=0x12345678;
    int good=MEM_W(0x1000,0xFFFFFFFFA0000000ull)==0x12345678;
    MEM_B(0x1005,0xFFFFFFFFA0000000ull)=0x5a;
    good=good && MEM_BU(0x1005,0xFFFFFFFF80000000ull)==0x5a;
    free(rdram);return good?0:2;
}
static int multiply_test(void) {
    uint64_t lo,hi;int64_t slo,shi;
    duck_vita_umul(UINT64_MAX,UINT64_MAX,&lo,&hi);
    if(lo!=1 || hi!=UINT64_MAX-1)return 1;
    duck_vita_smul(INT64_MIN,-1,&slo,&shi);
    return (uint64_t)slo==0x8000000000000000ull && shi==0?0:2;
}
int main(void) {
    psvDebugScreenInit();
    psvDebugScreenPrintf("DUCK DODGERS - VITA PORT BRING-UP\n\n");
    psvDebugScreenPrintf("Native ARM32 diagnostic, NOT the playable game.\n");
    psvDebugScreenPrintf("No ROM/assets installed or modified.\n\n");
    int mul=multiply_test();psvDebugScreenPrintf("128-bit multiply adapter: %s (%d)\n",mul?"FAIL":"PASS",mul);
    int alias=alias_test();psvDebugScreenPrintf("KSEG0/KSEG1 word/byte aliases: %s (%d)\n",alias?"FAIL":"PASS",alias);
    psvDebugScreenPrintf("Executing actual translated startup...\n");
    int boot=duck_boot_smoke_main();psvDebugScreenPrintf("Startup/BSS/stack/handoff: %s (%d)\n",boot?"FAIL":"PASS",boot);
    psvDebugScreenPrintf("Executing actual translated module leaf...\n");
    int module=duck_module_execution_main();psvDebugScreenPrintf("Module relocation / 28 RAM cases: %s (%d)\n",module?"FAIL":"PASS",module);
    psvDebugScreenPrintf("\nFull libultra runtime + Vita renderer still required.\n");
    psvDebugScreenPrintf("Triangle: exit. Photograph this screen for hardware results.\n");
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    while(1) {
        SceCtrlData pad={0};sceCtrlPeekBufferPositive(0,&pad,1);
        if(pad.buttons&SCE_CTRL_TRIANGLE)break;
        sceKernelDelayThread(16000);
    }
    sceKernelExitProcess(0);return 0;
}
