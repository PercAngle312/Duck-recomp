/* Test backend only. Native builds use the real Vita fiber API. */
#define _XOPEN_SOURCE 700
#include "duck_vita_threads.h"
#include <ucontext.h>
#include <stdlib.h>
static ucontext_t host;
int duck_fiber_prepare(void){return 0;}
static struct {ucontext_t context;void *stack;void (*entry)(unsigned);} fibers[16];
static void enter(unsigned slot){fibers[slot].entry(slot);abort();}
int duck_fiber_create(unsigned slot,void (*entry)(unsigned)) {
 if(slot>=16||fibers[slot].stack)return -1;
 fibers[slot].stack=malloc(128*1024);if(!fibers[slot].stack)return -1;
 if(getcontext(&fibers[slot].context)){free(fibers[slot].stack);fibers[slot].stack=0;return -1;}
 fibers[slot].entry=entry;fibers[slot].context.uc_stack.ss_sp=fibers[slot].stack;
 fibers[slot].context.uc_stack.ss_size=128*1024;fibers[slot].context.uc_link=&host;
 makecontext(&fibers[slot].context,(void(*)(void))enter,1,slot);return 0;
}
int duck_fiber_run(unsigned slot){return swapcontext(&host,&fibers[slot].context);}
void duck_fiber_suspend(unsigned slot){if(swapcontext(&fibers[slot].context,&host))abort();}
void duck_fiber_destroy(unsigned slot){free(fibers[slot].stack);fibers[slot].stack=0;}
