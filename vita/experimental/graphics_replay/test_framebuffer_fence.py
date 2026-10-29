"""Run the actual pinned-library fence wait against a mocked scene/GXM API."""
import argparse,pathlib,subprocess,tempfile,os
p=argparse.ArgumentParser();p.add_argument('--gxm',type=pathlib.Path,required=True);a=p.parse_args()
s=a.gxm.read_text();start=s.index('int duck_vgl_wait_read_framebuffer(void)');method=s[start:]
fixture=r'''
#include "framebuffer_fence.h"
#include <assert.h>
#include <stdio.h>
#define THREAD_SAFE()
#define GL_TRUE 1
typedef struct {unsigned id;} framebuffer;
typedef struct {volatile unsigned *address;unsigned value;} SceGxmNotification;
static unsigned completed;
static SceGxmNotification query_fence={&completed,20};
static DuckFramebufferFences duck_framebuffer_fences;
static framebuffer *active_read_fb,*duck_scene_target;
static int duck_fence_wrap,needs_end_scene,dirty_framebuffer,reset_count,wait_count,wait_error;
static unsigned waited_serial;
static void scene_reset(void){
 reset_count++;if(duck_scene_target)duck_fence_record(&duck_framebuffer_fences,duck_scene_target,++query_fence.value);
 duck_scene_target=active_read_fb;
}
static int sceGxmNotificationWait(const SceGxmNotification *n){wait_count++;waited_serial=n->value;if(wait_error)return wait_error;completed=n->value;return 0;}
'''
tests=r'''
int main(void){
 framebuffer source={1},later={2};active_read_fb=&source;duck_scene_target=&later;
 duck_fence_record(&duck_framebuffer_fences,&source,12);completed=10;
 assert(!duck_vgl_wait_read_framebuffer()&&wait_count==1&&waited_serial==12&&query_fence.value==20&&!reset_count);
 // Already-complete source does not wait on later context work.
 assert(!duck_vgl_wait_read_framebuffer()&&wait_count==1&&!reset_count);
 // Active source scene must be closed before obtaining its fence.
 duck_scene_target=&source;needs_end_scene=1;assert(!duck_vgl_wait_read_framebuffer()&&reset_count==1&&waited_serial==21&&dirty_framebuffer);
 needs_end_scene=0;duck_fence_record(&duck_framebuffer_fences,&source,22);query_fence.value=22;completed=20;wait_error=-9;
 assert(duck_vgl_wait_read_framebuffer()==-9);wait_error=0;
 // Missing, collided, invalid/future and wrapped serials request the fallback.
 duck_fence_record(&duck_framebuffer_fences,&source,0);assert(duck_vgl_wait_read_framebuffer()<0);
 duck_fence_record(&duck_framebuffer_fences,&source,23);assert(duck_vgl_wait_read_framebuffer()<0);
 duck_fence_record(&duck_framebuffer_fences,&source,12);duck_fence_wrap=1;assert(duck_vgl_wait_read_framebuffer()<0);
 duck_fence_wrap=0;active_read_fb=0;assert(duck_vgl_wait_read_framebuffer()<0);
 uintptr_t first=0x1000,other=first+16;while(duck_fence_slot(other)!=duck_fence_slot(first))other+=16;
 duck_fence_record(&duck_framebuffer_fences,(void*)first,5);duck_fence_record(&duck_framebuffer_fences,(void*)other,6);
 assert(!duck_fence_lookup(&duck_framebuffer_fences,(void*)first,22));
 puts("PASS actual FBO fence wait: source-only wait, active scene close, completion, errors, collisions, invalid/future serials and wrap fallback");
}
'''
with tempfile.TemporaryDirectory(prefix='duck-fences-') as tmp:
 t=pathlib.Path(tmp);src=t/'fence.c';src.write_text(fixture+'\n'+method+'\n'+tests);exe=t/'fence'
 subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(pathlib.Path(__file__).resolve().parent),str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True,timeout=10,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
