"""Compare candidate timer service against a specified original source file."""
import argparse,ctypes,pathlib,random,subprocess,tempfile,json
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--reference',type=pathlib.Path,required=True);p.add_argument('--headers',type=pathlib.Path,required=True)
p.add_argument('--report',type=pathlib.Path,required=True);a=p.parse_args();root=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='duck-time-diff-') as tmp:
 libs=[]
 for name,source in [('reference',a.reference),('candidate',root/'src/os_time.c')]:
  lib=pathlib.Path(tmp)/(name+'.so')
  subprocess.run(['cc','-shared','-fPIC','-Wl,-Bsymbolic','-O2','-Wall','-Wextra','-Werror','-fsanitize=undefined',*(['-DDUCK_TIME_SCAN_INSTRUMENT'] if name=='candidate' else []),'-I'+str(a.headers),'-I'+str(root/'include'),str(source),str(root/'tests/time_differential.c'),'-lm','-o',str(lib)],check=True)
  dll=ctypes.CDLL(str(lib));dll.duck_test_time_step.argtypes=[ctypes.c_uint,ctypes.c_uint,ctypes.c_uint64,ctypes.c_uint64];dll.duck_test_time_step.restype=ctypes.c_uint64;libs.append(dll)
 rng=random.Random(0xd026);count=0
 for session in range(8):
  for dll in libs:dll.duck_test_time_reset()
  for index in range(5000):
   op=rng.choices(range(9),[20,12,40,1,3,5,5,3,3])[0];slot=rng.randrange(36)
   delay=rng.choice([0,1,2,100,1000,rng.randrange(100000),0x100000001,0xffffffffffffffff])
   interval=rng.choice([0,0,0,1,7,100,10000,0x100000000])
   if op==2:delay=rng.randrange(10000)
   if op==4:delay=rng.randrange(5)
   if op==8:delay=rng.randrange(1,4)
   args=(op,slot,delay,interval);values=[dll.duck_test_time_step(*args) for dll in libs]
   assert values[0]==values[1],(session,index,args,values)
   count+=1
 candidate=libs[1];candidate.duck_test_time_reset()
 visits=ctypes.c_uint64.in_dll(candidate,'duck_test_timer_slot_visits');before=visits.value
 for i in range(10000):candidate.duck_test_time_step(2,0,1,0)
 idle_visits=visits.value-before;assert idle_visits==32,idle_visits
 candidate.duck_test_time_step(0,0,1000000,0);before=visits.value
 for i in range(10000):candidate.duck_test_time_step(2,0,1,0)
 waiting_visits=visits.value-before;assert waiting_visits==32,waiting_visits
 report={'operations_compared':count,'sessions':8,'mismatches':0,'idle_poll_count':10000,'idle_timer_slot_visits':idle_visits,'future_timer_poll_count':10000,'future_timer_slot_visits':waiting_visits,'coverage':'timer full table/replacement/stop, deadlines/ties, overflow, periodic catchup, VI changes, Count/Compare, nonblocking drops, reentrant stop/rearm/poll','hardware_performance_measured':False}
 a.report.write_text(json.dumps(report,indent=2)+'\n');print(report)
