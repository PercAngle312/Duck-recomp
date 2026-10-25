"""Differential host checks for ARM32 multiplication and generated memory access."""
from pathlib import Path
import argparse,ctypes,random,subprocess,tempfile
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--adapter-include',type=Path,required=True);a=p.parse_args()
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);source=root/'adapter.c'
 source.write_text('''#include "recomp.h"
#include "portable_mul.h"
void umul(uint64_t a,uint64_t b,uint64_t* lo,uint64_t* hi){duck_dmultu(a,b,lo,hi);}
void smul(int64_t a,int64_t b,int64_t* lo,int64_t* hi){duck_dmult(a,b,lo,hi);}
int memory(void){
 uint8_t* rdram=calloc(1,8*1024*1024);if(!rdram)return 1;
 MEM_W(0x1000,0xFFFFFFFF80000000ull)=0x12345678;
 if(MEM_W(0x1000,0xFFFFFFFFA0000000ull)!=0x12345678)return 2;
 MEM_H(0x1004,0xFFFFFFFFA0000000ull)=-123;
 if(MEM_H(0x1004,0xFFFFFFFF80000000ull)!=-123)return 3;
 MEM_B(0x1007,0xFFFFFFFFA0000000ull)=0x5a;
 if(MEM_BU(0x1007,0xFFFFFFFF80000000ull)!=0x5a)return 4;
 SD(0x1234567890abcdefull,0x1010,0xFFFFFFFFA0000000ull);
 if(load_doubleword(rdram,0xFFFFFFFF80000000ull,0x1010)!=0x1234567890abcdefull)return 5;
 free(rdram);return 0;
}
''')
 lib=root/'adapter.so'
 subprocess.run(['cc','-shared','-fPIC','-O2','-I'+str(a.adapter_include.resolve()),'-I'+str(Path(__file__).resolve().parent/'include'),str(source),'-o',str(lib)],check=True)
 dll=ctypes.CDLL(str(lib));u=ctypes.c_uint64;i=ctypes.c_int64
 dll.umul.argtypes=[u,u,ctypes.POINTER(u),ctypes.POINTER(u)];dll.smul.argtypes=[i,i,ctypes.POINTER(i),ctypes.POINTER(i)]
 rng=random.Random(0xD0D6E);mask=(1<<64)-1
 edges=[0,1,2,(1<<31)-1,1<<32,(1<<63)-1,1<<63,mask-1,mask]
 cases=[(x,y) for x in edges for y in edges]+[(rng.getrandbits(64),rng.getrandbits(64)) for _ in range(10000)]
 for x,y in cases:
  lo=u();hi=u();dll.umul(x,y,ctypes.byref(lo),ctypes.byref(hi));product=x*y
  assert (lo.value,hi.value)==(product&mask,(product>>64)&mask)
  sx=x if x<1<<63 else x-(1<<64);sy=y if y<1<<63 else y-(1<<64)
  slo=i();shi=i();dll.smul(sx,sy,ctypes.byref(slo),ctypes.byref(shi));product=sx*sy
  assert (slo.value&mask,shi.value&mask)==(product&mask,(product>>64)&mask)
 assert dll.memory()==0
 print(f'PASS: {len(cases)*2} signed/unsigned multiply comparisons; cached/uncached word, halfword, byte and doubleword memory aliases.')
