#include "duck_vita_threads.h"
#include <psp2/fiber.h>
#include <psp2/sysmodule.h>
#include <stdlib.h>
enum { FIBERS=16, STACK_BYTES=128*1024 };
static struct {SceFiber fiber;void *stack;void (*entry)(unsigned);} fibers[FIBERS];
int duck_fiber_prepare(void) {
    return sceSysmoduleIsLoaded(SCE_SYSMODULE_FIBER)==0 ? 0 :
        (sceSysmoduleLoadModule(SCE_SYSMODULE_FIBER)<0 ? -1 : 0);
}
static void enter(SceUInt32 slot,SceUInt32 argument) {
    (void)argument;fibers[slot].entry(slot);
    sceFiberReturnToThread(0,NULL);abort();
}
int duck_fiber_create(unsigned slot,void (*entry)(unsigned)) {
    if(slot>=FIBERS||fibers[slot].stack)return -1;
    void *stack=malloc(STACK_BYTES);if(!stack)return -1;
    char name[]="DuckGuest";
    int rc=_sceFiberInitializeImpl(&fibers[slot].fiber,name,enter,slot,stack,STACK_BYTES,NULL);
    if(rc<0){free(stack);return -1;}
    fibers[slot].stack=stack;fibers[slot].entry=entry;return 0;
}
int duck_fiber_run(unsigned slot) {
    SceUInt32 value=0;return sceFiberRun(&fibers[slot].fiber,0,&value)<0 ? -1 : 0;
}
void duck_fiber_suspend(unsigned slot) {
    (void)slot;SceUInt32 value=0;if(sceFiberReturnToThread(0,&value)<0)abort();
}
void duck_fiber_destroy(unsigned slot) {
    if(slot>=FIBERS||!fibers[slot].stack)return;
    if(sceFiberFinalize(&fibers[slot].fiber)<0)abort();
    free(fibers[slot].stack);fibers[slot].stack=NULL;
}
