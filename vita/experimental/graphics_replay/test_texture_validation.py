"""Exercise actual bridge staging/build methods with controlled RAM and GPU seams.

The fixture substitutes hashing, RAM copying and GPU cache callbacks; it tests
invalidation and staging, not display-list interpretation or the NEON hash.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source',type=Path,required=True)
p.add_argument('--reference',type=Path,required=True)
p.add_argument('--report',type=Path,required=True)
a=p.parse_args()
here=Path(__file__).resolve().parent
prefix=r'''
#include "texture_stage_index.h"
#include <unordered_map>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <malloc.h>
#include <cassert>
#include <cstdio>
using u8=uint8_t;using u32=uint32_t;using u64=uint64_t;using GBIVersion=int;
constexpr unsigned RDRAM_DIRTY_PAGE_SHIFT=12,RDRAM_DIRTY_PAGE_COUNT=2048;
u32 gRamSize=8*1024*1024;
u8 ram[8*1024*1024],g_RDRAMDirtyPages[RDRAM_DIRTY_PAGE_COUNT];
void RDRAM_ClearDirtyPages(){memset(g_RDRAMDirtyPages,0,sizeof g_RDRAMDirtyPages);}
struct HostGfx {u32 w0,w1;};
struct GPU {
 unsigned deletes=0;
 void TextureCacheDelete(void*){deletes++;}
 void RegisterStagedTextureSource(void*,u32,u32){}
};
class Fast3DMemoryBridge {
public:
 struct TextureStage {u32 address,size,hash[4],validatedBuild,rowBytes;u8 swizzleGroup;bool dirty;u8 *data;};
 TextureStage *mTextureStages=nullptr;
 u32 mTextureStageCount=0,mTextureStageCapacity=0,mBuildSerial=0;
 std::unordered_map<DuckTextureStageKey,u32,DuckTextureStageKey::Hasher> mTextureStageIndex;
 GPU gpu;GPU *mInterpreter=&gpu;
 bool mTranslationFailed=false,mSawFullSync=false,mHaveTextureImage=false;
 u32 mTextureImageW0=0,mTextureImageResolved=0;void *mTextureImageCommand=nullptr;u8 *mTextureImageStaging=nullptr;
 unsigned hashes=0;uint64_t historical_visits=0;HostGfx command{};
 ~Fast3DMemoryBridge(){for(unsigned i=0;i<mTextureStageCount;i++)free(mTextureStages[i].data);free(mTextureStages);}
 void Reset(){}
 void *TranslateList(u32,GBIVersion&,unsigned){mHaveTextureImage=true;mTextureImageCommand=&command;return &command;}
 void CopyRdramBytes(u8 *out,u32 address,u32 size){for(u32 i=0;i<size;i++)out[i]=ram[(address+i)^3];}
 void HashRdramBytes(u32 address,u32 size,u32 *h){
  hashes++;for(unsigned j=0;j<4;j++)h[j]=j+1;
  for(u32 i=0;i<size;i++)h[i%4]=h[i%4]*16777619u^ram[address+i];
 }
 bool StageTextureImage(u32,u32=0,u32=0,u8=0);
 void *BuildDisplayList(u32,GBIVersion);
 void begin(){memset(g_RDRAMDirtyPages,1,sizeof g_RDRAMDirtyPages);assert(BuildDisplayList(0,0));}
 void use(unsigned address,unsigned bytes,unsigned pitch=0,unsigned group=0){
  mTextureImageResolved=address;assert(StageTextureImage(bytes,0,pitch,group));
  for(unsigned i=0;i<bytes;i++){
   unsigned logical=i;
   if(pitch&&group&&(i/pitch)%2){unsigned within=i%pitch;if(within/group*group+group<=pitch)logical=(i/pitch)*pitch+within/group*group+(within%group+group/2)%group;}
   assert(mTextureImageStaging[i]==ram[(address+logical)^3]);
  }
 }
};
'''
suffix=r'''
int main(){
 Fast3DMemoryBridge b;
 for(unsigned i=0;i<sizeof ram;i++)ram[i]=(u8)(i*17+(i>>8));
 b.begin();for(unsigned i=0;i<4096;i++)b.use(i*64,64);
 unsigned expected_deletes=0;
 for(unsigned frame=0;frame<128;frame++){
  for(unsigned j=0;j<8;j++)ram[((frame*31+j*257)%4096)*64]^=0x31;
  b.begin();unsigned before=b.hashes;
  for(unsigned j=0;j<8;j++){
   unsigned at=(frame*31+j*257)%4096*64;
   for(unsigned repeat=0;repeat<4;repeat++)b.use(at,64);
   expected_deletes++;
  }
  assert(b.hashes-before==8&&b.gpu.deletes==expected_deletes);
 }
 // Swizzled rows and source layout are independently keyed and revalidated.
 b.begin();b.use(0x60000,128,32,8);b.use(0x60000,128,32,16);
 ram[0x60024]^=0x77;b.begin();b.use(0x60000,128,32,8);b.use(0x60000,128,32,16);
 unsigned hashes=b.hashes,deletes=b.gpu.deletes;uint64_t visits=b.historical_visits;
#if LAZY_VALIDATION
 // A dormant serial=1 tag must not survive counter wrap and hide a mutation.
 b.mTextureStages[0].validatedBuild=1;b.mBuildSerial=UINT32_MAX;ram[0]^=1;
 b.begin();b.use(0,64);assert(b.gpu.deletes==deletes+1);
 assert(visits==0);
#else
 assert(visits>=4096*128);
#endif
 printf("{\"hashes\":%u,\"texture_updates\":%u,\"historical_stage_visits\":%llu}\n",hashes,deletes,(unsigned long long)visits);
}
'''

def methods(path):
    s=path.read_text()
    start=s.index('bool Fast3DMemoryBridge::StageTextureImage(')
    end=s.index('\nvoid* Fast3DMemoryBridge::ConvertMatrix(',start)
    stage=s[start:end]
    start=s.index('void* Fast3DMemoryBridge::BuildDisplayList(')
    build=s[start:]
    # This public pinned source ends with BuildDisplayList.
    assert build.rstrip().endswith('}')
    build=build.replace('TextureStage& stage = mTextureStages[i];',
                        'historical_visits++; TextureStage& stage = mTextureStages[i];')
    return stage+'\n'+build

results={}
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp)
    for name,path,lazy in [('reference',a.reference,0),('lazy',a.source,1)]:
        cpp=root/(name+'.cpp');exe=root/name
        cpp.write_text(prefix+methods(path)+suffix)
        subprocess.run(['c++','-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-Wno-unused-variable','-fsanitize=address,undefined',
                        '-DLAZY_VALIDATION='+str(lazy),'-I'+str(here),str(cpp),'-o',str(exe)],check=True)
        results[name]=json.loads(subprocess.check_output([str(exe)],text=True))
assert results['reference']['hashes']==results['lazy']['hashes']
assert results['reference']['texture_updates']==results['lazy']['texture_updates']
a.report.write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results))
