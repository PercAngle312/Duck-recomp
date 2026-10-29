"""Execute the production texture-cache method against a draw-order spy.

Host contract check, not a GPU/gameplay test. --source is prepared interpreter.cpp.
"""
import argparse,pathlib,subprocess,tempfile,os
p=argparse.ArgumentParser();p.add_argument('--source',type=pathlib.Path,required=True);a=p.parse_args()
s=a.source.read_text();start=s.index('bool Interpreter::TextureCacheLookup(');body=s.index('{',start);depth=1;end=body+1
while depth:
 if s[end]=='{':depth+=1
 if s[end]=='}':depth-=1
 end+=1
method=s[start:end]
fixture=r'''
#include <map>
#include <list>
#include <vector>
#include <utility>
#include <cstdint>
#include <cassert>
#include <cstdio>
constexpr unsigned TEXTURE_CACHE_MAX_SIZE=2;constexpr int SHADER_MAX_TEXTURES=6;
struct TextureCacheKey {int id;bool operator<(const TextureCacheKey &o)const{return id<o.id;}};
struct TextureCacheValue;
using TextureCacheMap=std::map<TextureCacheKey,TextureCacheValue>;
struct LruEntry {TextureCacheMap::iterator it;};
struct TextureCacheValue {uint32_t texture_id=0;std::list<LruEntry>::iterator lru_location;};
using TextureCacheNode=TextureCacheMap::value_type;
struct Spy {
 unsigned next_id=0,bound[6]={};std::vector<unsigned> commands;
 unsigned NewTexture(){return ++next_id;}
 void SelectTexture(int slot,unsigned id){bound[slot]=id;commands.push_back(100+id);}
 void SetSamplerParameters(int,bool,int,int){commands.push_back(200);}
};
struct Interpreter {
 struct {TextureCacheMap map;std::list<LruEntry> lru;std::vector<uint32_t> free_texture_ids;} mTextureCache;
 struct {TextureCacheNode *mTextures[6]={};} mRenderingState;
 Spy *mRapi;
 unsigned mDuckTextureHits=0,mDuckTextureUploads=0,mDuckTextureBatchKeeps=0,mBufVboNumTris=0;
 void Flush(){if(mBufVboNumTris){mRapi->commands.push_back(mBufVboNumTris);mBufVboNumTris=0;}}
 bool TextureCacheLookup(int,const TextureCacheKey&);
};
'''
tests=r'''
int main(){
 Spy spy;Interpreter g;g.mRapi=&spy;
 assert(!g.TextureCacheLookup(0,{1}));g.mBufVboNumTris=3;spy.commands.clear();
 assert(g.TextureCacheLookup(0,{1}));assert(g.mBufVboNumTris==3&&g.mDuckTextureBatchKeeps==1);
 assert(spy.commands.size()==1&&spy.commands[0]==101);
 // An upload flushes before rebinding or ID reuse.
 spy.commands.clear();assert(!g.TextureCacheLookup(0,{2}));assert(spy.commands[0]==3&&spy.commands[1]==102);
 g.mBufVboNumTris=4;spy.commands.clear();assert(g.TextureCacheLookup(0,{1}));assert(spy.commands[0]==4&&spy.commands[1]==101);
 // A framebuffer binding invalidates the tracked cache node. Rebinding the
 // formerly-selected node must flush, despite equal source/key identity.
 g.mRenderingState.mTextures[0]=nullptr;g.mBufVboNumTris=7;spy.commands.clear();
 assert(g.TextureCacheLookup(0,{1})&&spy.commands[0]==7&&spy.commands[1]==101);
 // Fill second sampler, then force LRU eviction with pending geometry.
 assert(g.TextureCacheLookup(1,{2}));g.mBufVboNumTris=5;spy.commands.clear();
 assert(!g.TextureCacheLookup(0,{3}));assert(spy.commands[0]==5&&spy.commands[1]==101);
 assert(g.mRenderingState.mTextures[1]&&g.mRenderingState.mTextures[1]->first.id==2);
 assert(!g.mTextureCache.map.count({1})&&g.mTextureCache.map.count({3}));
 puts("PASS production texture batching: same-node batches retained; binding switches, framebuffer transitions, uploads and eviction flush in order");
}
'''
with tempfile.TemporaryDirectory(prefix='duck-batches-') as tmp:
 t=pathlib.Path(tmp);src=t/'contract.cpp';src.write_text(fixture+'\n'+method+'\n'+tests);exe=t/'contract'
 subprocess.run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True,timeout=15,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
# Ensure the actual caller has no pre-lookup flush and framebuffer use clears
# the tracked node, as required by the executable contract above.
assert 'if (mRdp->textures_changed[i]) {\n                Flush();' not in s
assert 'mRenderingState.mTextures[i] = nullptr;\n            mRapi->SelectTextureFb' in s
