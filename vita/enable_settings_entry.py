"""Add the native settings loop to a private v00.17 game coordinator copy.

This patches authored application glue, never translated game functions. Input
is preserved and output must remain outside the public repository.
"""
from pathlib import Path
import argparse,re
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();root=Path(__file__).resolve().parents[1];target=a.output.resolve()
if target==a.input.resolve() or target==root or root in target.parents:p.error('Use a new private output outside the public repository')
s=a.input.read_text()
if 'duck_fast3d_settings_poll' in s or 'compare_audio_rsp' in s:p.error('Already integrated or instrumented coordinator; inspect it before patching')
for old,new in [
 ('#include "librecomp/rsp.hpp"','#include "librecomp/rsp.hpp"\n#include "paused_clock.h"'),
 ('extern "C" int duck_fast3d_submit_task(uint32_t);','extern "C" int duck_fast3d_submit_task(uint32_t);\nextern "C" int duck_fast3d_settings_poll(uint32_t,uint64_t);\nstatic DuckPausedClock menu_clock;\nstatic uint64_t guest_clock(void*){return menu_clock.read(duck_vita_clock_ticks(nullptr));}'),
 ('duck_vita_clock_ticks,nullptr,781250','guest_clock,nullptr,781250')]:
 if s.count(old)!=1:p.error('Unrecognized coordinator anchor: '+old)
 s=s.replace(old,new)
start=s.index(' for(;;){\n  rc=duck_vita_live_run');end=s.index('\n duck_vita_ai_vita_close();',start)
loop=r''' for(;;){
  uint64_t now=sceKernelGetProcessTimeWide(),clock_sample=duck_vita_clock_ticks(nullptr);
  SceCtrlData pad={};sceCtrlPeekBufferPositive(0,&pad,1);
  if((pad.buttons&(SCE_CTRL_SELECT|SCE_CTRL_START))==(SCE_CTRL_SELECT|SCE_CTRL_START)){
   if(!exit_since)exit_since=now;if(now-exit_since>=1000000)break;
  }else exit_since=0;
  bool menu=duck_fast3d_settings_poll(pad.buttons,now)!=0;
  menu_clock.set(menu,clock_sample);
  if(menu){
   if(duck_vita_ai_service()<0){fprintf(stderr,"Audio device failed while settings paused\n");return 9;}
   sceKernelDelayThread(16000);continue;
  }
  rc=duck_vita_live_run(&live,128);
  if(rc<0){fprintf(stderr,"Guest runtime failed: threads=%d SP=%d AI=%d\n",duck_vita_threads_error(),duck_vita_sp_os_last_result(),duck_vita_ai_error());return 9;}
  if(rc==DUCK_THREADS_IDLE){fprintf(stderr,"Guest became idle\n");return 10;}
  now=sceKernelGetProcessTimeWide();
  if(now-last_status>5000000){last_status=now;fprintf(stderr,"Guest status: polls=%u PI=%u SP completed=%u failed=%u loop_yields=%llu\n",live.scheduler_polls,live.pi_transfers,live.sp.completed,live.sp.failed,(unsigned long long)duck_vita_threads_checkpoint_yields());}
  if(rc==DUCK_THREADS_WAITING)sceKernelDelayThread(1000);
 }'''
s=s[:start]+loop+s[end:];s=re.sub(r'full guest build 00\.[0-9]+','full guest build 00.18',s)
target.parent.mkdir(parents=True,exist_ok=True);target.write_text(s);print(target)
