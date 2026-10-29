"""Compare actual bridge conversion methods against the uncached implementation."""
import argparse,pathlib,subprocess,tempfile,json
p=argparse.ArgumentParser(description=__doc__)
for name in ['source','reference','report']:p.add_argument('--'+name,type=pathlib.Path,required=True)
a=p.parse_args();here=pathlib.Path(__file__).resolve().parent
names=['ConvertMatrix','ConvertVertices','ConvertViewport','ConvertLight']
def method(text,name):
 start=text.index('void* Fast3DMemoryBridge::'+name+'(');end=text.index('\n}\n',start)+3;return text[start:end]
prefix=r'''
#include "conversion_cache.h"
#include "native_geometry.h"
#include <vector>
#include <memory>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fenv.h>
using u8=uint8_t;using u16=uint16_t;using s16=int16_t;using u32=uint32_t;using s32=int32_t;
static u8 memory[0x800000],*g_pu8RamBase=memory;static u32 gRamSize=sizeof memory;
struct HostVtx {float x,y,z;u16 flag;s16 s,t;u8 c0,c1,c2,c3;u16 pad;};
struct HostViewport {s16 scale[4],trans[4];};
struct BridgeBase {
 u32 mBuildSerial=1,segments[16]{};unsigned allocations=0,reads=0;
 std::vector<std::unique_ptr<u8[]>> arena;
 DuckConversionCache<64,64,128> mDuckMatrixCache;
 DuckConversionCache<516,768,256> mDuckVertexCache;
 DuckConversionCache<20,16,64> mDuckViewportCache;
 DuckConversionCache<28,24,64> mDuckLightCache;
 void *Alloc(size_t n){++allocations;arena.emplace_back(new u8[n?n:1]);return arena.back().get();}
 u32 Resolve(u32 a){u32 seg=a>>24;return (seg&&seg<16)?(segments[seg]+(a&0xffffff))&0xffffff:a&0xffffff;}
 u32 Read32(u32 a){++reads;u32 v;memcpy(&v,memory+a,4);return v;}
 s16 ReadS16(u32 a){++reads;s16 v;memcpy(&v,memory+(a^2),2);return v;}
 u16 ReadU16(u32 a){return (u16)ReadS16(a);}
 u8 ReadU8(u32 a){++reads;return memory[a^3];}
 void begin(){arena.clear();if(++mBuildSerial==0){mBuildSerial=1;mDuckMatrixCache.invalidate_task_pointers();mDuckVertexCache.invalidate_task_pointers();mDuckViewportCache.invalidate_task_pointers();mDuckLightCache.invalidate_task_pointers();}}
};
struct Fast3DMemoryBridge:BridgeBase{void *ConvertMatrix(u32);void *ConvertVertices(u32,u32);void *ConvertViewport(u32);void *ConvertLight(u32,u32);};
struct ReferenceBridge:BridgeBase{void *ConvertMatrix(u32);void *ConvertVertices(u32,u32);void *ConvertViewport(u32);void *ConvertLight(u32,u32);};
'''
suffix=r'''
static uint32_t random_state=0xd027;
static unsigned random_word(){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
int main(){
 Fast3DMemoryBridge candidate;ReferenceBridge reference;
 for(unsigned i=0;i<sizeof memory;i++)memory[i]=(u8)random_word();
 unsigned comparisons=0;
 for(unsigned frame=0;frame<128;frame++){
  candidate.begin();reference.begin();
  candidate.segments[1]=reference.segments[1]=(frame%2)*0x10000;
  const unsigned rounds[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};assert(!fesetround(rounds[frame%4]));
  std::vector<std::pair<void*,std::vector<u8>>> retained;
  for(unsigned i=0;i<256;i++){
   unsigned op=i%4,count=1+(random_word()%32),address=0x1000+(i%32)*0x400;
   if(op==1||op==2)address+=(i&4)?2:0;
   if(i%3==0)memory[(address+(random_word()%16))^3]^=0x80;
   u32 guest=0x01000000|address;size_t bytes=op==0?64:op==1?24*count:op==2?16:(i&4)?24:12;
   for(unsigned repeat=0;repeat<3;repeat++){
    void *actual=op==0?candidate.ConvertMatrix(guest):op==1?candidate.ConvertVertices(guest,count):op==2?candidate.ConvertViewport(guest):candidate.ConvertLight(guest,(u32)bytes);
    void *expected=op==0?reference.ConvertMatrix(guest):op==1?reference.ConvertVertices(guest,count):op==2?reference.ConvertViewport(guest):reference.ConvertLight(guest,(u32)bytes);
    assert(actual&&expected&&!memcmp(actual,expected,bytes));++comparisons;
    if(repeat==0)retained.push_back({actual,std::vector<u8>((u8*)actual,(u8*)actual+bytes)});
    if(repeat==1&&i%5==0)memory[(candidate.Resolve(guest)+2)^3]^=1;
   }
   // Direct-map evictions and same-address mutations may not change prior commands.
   for(const auto &saved:retained)assert(!memcmp(saved.first,saved.second.data(),saved.second.size()));
  }
 }
 // Halfword-aligned inputs depend on physical bytes outside the nominal span.
 candidate.begin();void *first=candidate.ConvertVertices(0x80001002,1);std::vector<u8> saved((u8*)first,(u8*)first+24);
 memory[0x1000]^=0x40;void *second=candidate.ConvertVertices(0x80001002,1);assert(memcmp(saved.data(),second,24));assert(!memcmp(first,saved.data(),24));
 // Bounds remain enforced before looking at a matching cache entry.
 assert(!candidate.ConvertMatrix(0x807fffc4)&&!candidate.ConvertMatrix(0x80001002));
 assert(!candidate.ConvertVertices(0x807ffff2,1)&&!candidate.ConvertVertices(0x80001000,33)&&!candidate.ConvertVertices(0x80001000,0));
 assert(!candidate.ConvertViewport(0x807ffff2)&&!candidate.ConvertLight(0x807ffff0,24));
 candidate.mBuildSerial=0xffffffff;candidate.begin();assert(candidate.mBuildSerial==1);assert(candidate.ConvertMatrix(0x80001000));
 assert(candidate.reads<reference.reads&&candidate.allocations<reference.allocations);
 printf("{\"comparisons\":%u,\"mismatches\":0,\"candidate_scalar_reads\":%u,\"reference_scalar_reads\":%u,\"candidate_allocations\":%u,\"reference_allocations\":%u,\"hardware_performance_measured\":false}\n",comparisons,candidate.reads,reference.reads,candidate.allocations,reference.allocations);
}
'''
source=a.source.read_text();reference=a.reference.read_text()
with tempfile.TemporaryDirectory(prefix='duck-conversion-') as tmp:
 t=pathlib.Path(tmp);cpp=t/'fixture.cpp';cpp.write_text(prefix+'\n'+''.join(method(source,n) for n in names)+'\n'+''.join(method(reference,n).replace('Fast3DMemoryBridge::','ReferenceBridge::') for n in names)+'\n'+suffix);exe=t/'fixture'
 subprocess.run(['c++','-std=c++17','-O2','-frounding-math','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(here),str(cpp),'-o',str(exe)],check=True)
 import os
 result=subprocess.run([str(exe)],capture_output=True,text=True,check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 report=json.loads(result.stdout);report['actual_production_methods_executed']=True;report['coverage']='source mutation, segmented remaps, count/size changes, hash-slot collisions, same-task pointer lifetime, frame reset/wrap, halfword byte lanes, bounds and all four rounding modes';a.report.write_text(json.dumps(report,indent=2)+'\n');print(report)
