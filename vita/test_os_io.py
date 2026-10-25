"""Reproduce the private resident I/O batch with real VitaSDK tools.

The soft-float partial link preserves the attached historical ABI. A separate
hard-float CPU test ELF is rebuilt from source; incompatible objects are never
forced together. The test ELF has no unresolved symbols and is not a Vita game.
"""
import argparse, hashlib, json, os, re, subprocess
from pathlib import Path
root=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
for name in ('vitasdk','recomp-include','base-object','binding-source','output'):
    p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--queue-source',type=Path,default=root/'src/os_queue.c')
a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
sdk=a.vitasdk.resolve()/'bin'
cc=str(sdk/'arm-vita-eabi-gcc');ld=str(sdk/'arm-vita-eabi-ld');nm=str(sdk/'arm-vita-eabi-nm')
def run(command,env=None):
    r=subprocess.run(list(map(str,command)),capture_output=True,text=True,env=env)
    if r.returncode:raise RuntimeError('Command failed: '+str(command[0])+'\n'+r.stdout+r.stderr)
    return r.stdout
def undefined(path):return sorted({line.split()[-1] for line in run([nm,'-u',path]).splitlines() if line.strip()})
includes=['-I'+str(root/'include'),'-I'+str(a.recomp_include.resolve())]
host=out/'os-io-host'
run(['cc','-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
     *includes,root/'src/os_io.c',a.queue_source,root/'tests/os_io.c','-o',host])
# LeakSanitizer cannot enumerate tasks in this execution environment. ASan and
# UBSan remain enabled; the test explicitly frees its only RAM allocation.
host_log=run([host],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
(out/'host-test.log').write_text(host_log);print(host_log.strip())
common=['-std=c11','-O1','-mcpu=cortex-a9','-mfpu=neon','-ffunction-sections','-fdata-sections',
        '-ffreestanding','-fno-builtin','-fno-tree-loop-distribute-patterns',*includes]
run([cc,*common,'-mfloat-abi=soft','-c',root/'src/os_io.c','-o',out/'os_io.soft.arm.o'])
run([ld,'-r',a.base_object,out/'os_io.soft.arm.o','-o',out/'combined.arm.o'])
before=undefined(a.base_object);after=undefined(out/'combined.arm.o')
resolved=sorted(set(before)-set(after));introduced=sorted(set(after)-set(before))
expected=['osCreatePiManager_recomp','osPiReadIo_recomp','osPiStartDma_recomp','osSetEventMesg_recomp']
assert resolved==expected,(resolved,expected)
assert not introduced,introduced
(out/'before.txt').write_text('\n'.join(before)+'\n');(out/'remaining.txt').write_text('\n'.join(after)+'\n')
objects=[]
for source,name in [(root/'src/os_io.c','io'),(a.queue_source,'queue'),(a.binding_source,'bindings')]:
    target=out/(name+'.hard.arm.o');run([cc,*common,'-mfloat-abi=hard','-c',source,'-o',target]);objects.append(target)
bindings={}
source=a.binding_source.read_text()
for callback in expected:
    pattern=r'void\s+(\w+)\([^{}]*\)\s*\{\s*'+callback+r'\(ram,\s*ctx\);\s*\}'
    found=re.findall(pattern,source)
    assert len(found)==1,(callback,found)
    bindings[callback]=found[0]
(out/'private-bindings.json').write_text(json.dumps(bindings,indent=2)+'\n')
libgcc=run([cc,'-print-libgcc-file-name']).strip()
command=[ld,'--gc-sections','-Ttext=0x10000','-e',bindings['osPiReadIo_recomp']]
for symbol in [*bindings.values(),'duck_vita_os_io_reset','duck_vita_os_pi_service','duck_vita_os_event_emit']:
    command+=['-u',symbol]
run([*command,*objects,libgcc,'-o',out/'io-cpu-test.elf'])
assert not undefined(out/'io-cpu-test.elf')
groups={
 'compiler_and_libm':[s for s in after if s.startswith('__aeabi_') or s in ('fegetround','fesetround','lrintf','sqrtf')],
 'threading':['osCreateThread_recomp','osDestroyThread_recomp','osSetThreadPri_recomp','osStartThread_recomp','pause_self'],
 'vi_init_timer_interrupt':['osCreateViManager_recomp','osInitialize_recomp','osSetIntMask_recomp','osSetTimer_recomp'],
 'rsp_tasks':['osSpTaskLoad_recomp','osSpTaskStartGo_recomp','osSpTaskYield_recomp','osSpTaskYielded_recomp'],
 'dispatch_loader_diagnostics':['get_function','duck_before_free','duck_register_loaded_module','func_8000F810','do_break','switch_error']}
groups={name:[s for s in symbols if s in after] for name,symbols in groups.items()}
assert sorted(s for group in groups.values() for s in group)==after
report=dict(before_count=len(before),remaining_count=len(after),resolved=resolved,new_undefined=introduced,
 remaining_by_subsystem=groups,host_sanitizer_tests_passed=True,hard_float_test_elf_unresolved=0,
 soft_float_partial_link=True,full_game_linked=False,vita_os_tested=False,playable_game=False,
 base_sha256=hashlib.sha256(a.base_object.read_bytes()).hexdigest(),
 combined_sha256=hashlib.sha256((out/'combined.arm.o').read_bytes()).hexdigest())
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
