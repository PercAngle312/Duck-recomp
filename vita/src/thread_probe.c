/* Hardware validation for the native scheduling prerequisite, not the game. */
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <stdio.h>
#include "debugScreen.h"
unsigned int _newlib_heap_size_user=64*1024*1024;
int duck_threads_test_main(void);
static void finish(int status) {
 psvDebugScreenPrintf("\nTriangle: exit.\n");
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
 for(;;){SceCtrlData pad={0};sceCtrlPeekBufferPositive(0,&pad,1);
  if(pad.buttons&SCE_CTRL_TRIANGLE)sceKernelExitProcess(status);
  sceKernelDelayThread(16000);
 }
}
void __assert_func(const char *file,int line,const char *function,const char *expression) {
 psvDebugScreenPrintf("\nFAIL: %s:%d\n%s\n%s\n",file,line,function?function:"",expression);
 finish(1);for(;;){}
}
int main(void) {
 psvDebugScreenInit();
 psvDebugScreenPrintf("DUCK VITA - NATIVE THREAD TEST\n\n");
 psvDebugScreenPrintf("Runtime hardware test, NOT the playable game.\nNo ROM or save data accessed.\n\n");
 psvDebugScreenPrintf("Testing native stacks, blocking queues, priority,\nyields, floating-point state and thread cleanup...\n");
 int rc=duck_threads_test_main();
 psvDebugScreenPrintf("\nNative fiber / guest thread tests: %s (%d)\n",rc?"FAIL":"PASS",rc);
 finish(rc);return rc;
}
