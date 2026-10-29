"""Compare mapped private utility functions and all context bytes on the host."""
import argparse,pathlib,json,subprocess,tempfile,os,hashlib
p=argparse.ArgumentParser(description=__doc__)
for name in ['source-root','mapping','candidate','headers','report']:p.add_argument('--'+name,type=pathlib.Path,required=True)
a=p.parse_args();root=pathlib.Path(__file__).resolve().parent;entries=json.loads(a.mapping.read_text());original=[];candidate=a.candidate.read_text()
for e in entries:
 s=(a.source_root/e['source']).read_text();start=s.index('RECOMP_FUNC void '+e['symbol']+'(');end=s.index('\n;}',start)+3;body=s[start:end]
 assert hashlib.sha256(body.encode()).hexdigest()==e['sha256'],'Reference function hash mismatch'
 original.append(body.replace(e['symbol'],'reference_'+e['kernel']))
 candidate=candidate.replace(e['symbol'],'candidate_'+e['kernel'])
prefix=r'''
#include "recomp.h"
#include <string.h>
#include <stdio.h>
static uint8_t reference_ram[0x800000],candidate_ram[0x800000];
void duck_vita_thread_checkpoint(uint8_t *r,recomp_context *c){(void)r;(void)c;}
static uint32_t random_state=0xd028;static uint32_t random_word(void){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
'''
suffix=r'''
int main(void){
 unsigned cases=0;
 for(unsigned op=0;op<3;op++)for(unsigned alignment=0;alignment<16;alignment++)for(unsigned trial=0;trial<256;trial++){
  unsigned n=trial<192?trial:(random_word()%4096),src=0x1000+alignment,dst=0x4000+(trial%16);
  // Include overlapping copies and cached/uncached aliases with mixed extension.
  if(op==1&&trial%4==0)dst=src+(trial%31);
  if(op==1&&trial%4==1)dst=src-(trial%31);
  for(unsigned i=0;i<0x10000;i++)reference_ram[i]=candidate_ram[i]=(uint8_t)random_word();
  recomp_context ref,got;uint8_t *b=(uint8_t*)&ref;
  for(unsigned i=0;i<sizeof ref;i++)b[i]=(uint8_t)random_word();
  ref.r4=(gpr)(int32_t)(0x80000000u|src);ref.r5=(gpr)(int32_t)(0x80000000u|dst);ref.r6=n;ref.r29=(gpr)(int32_t)0x8000f000u;
  if(trial%3==0)ref.r4=(uint32_t)ref.r4;
  if(trial%5==0)ref.r5=(uint32_t)ref.r5;
  if(trial%7==0)ref.r4^=0x20000000u;
  if(op==0)ref.r5=trial==255?(gpr)-1:n;
  if(op==2)ref.r5=random_word();
  got=ref;
  if(op==0){reference_zero(reference_ram,&ref);candidate_zero(candidate_ram,&got);}
  else if(op==1){reference_copy(reference_ram,&ref);candidate_copy(candidate_ram,&got);}
  else{reference_fill(reference_ram,&ref);candidate_fill(candidate_ram,&got);}
  if(memcmp(&ref,&got,sizeof ref)||memcmp(reference_ram,candidate_ram,0x10000)){
   fprintf(stderr,"Mismatch op=%u alignment=%u trial=%u length=%u\n",op,alignment,trial,n);
   for(unsigned i=0;i<32;i++){gpr a=((gpr*)&ref)[i],b=((gpr*)&got)[i];if(a!=b)fprintf(stderr,"r%u: %llx != %llx\n",i,(unsigned long long)a,(unsigned long long)b);}
   return 1;
  }
  ++cases;
 }
 printf("{\"cases\":%u,\"mismatches\":0,\"full_context_compared\":true,\"hardware_performance_measured\":false}\n",cases);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='duck-native-memory-') as tmp:
 t=pathlib.Path(tmp);source=t/'fixture.c';source.write_text(prefix+'\n'.join(original)+'\n'+candidate+'\n'+suffix);exe=t/'fixture'
 subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Wno-unused-variable','-Wno-unused-label','-fsanitize=address,undefined','-I'+str(a.headers),'-I'+str(root/'include'),str(source),str(root/'src/native_memory.c'),'-lm','-o',str(exe)],check=True)
 result=subprocess.run([str(exe)],capture_output=True,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 if result.returncode:raise RuntimeError(result.stdout+result.stderr)
 report=json.loads(result.stdout);report['coverage']='copy overlaps/alignment/aliases, zero lengths/negative size, fill stack write and complete register/context preservation';a.report.write_text(json.dumps(report,indent=2)+'\n');print(report)
