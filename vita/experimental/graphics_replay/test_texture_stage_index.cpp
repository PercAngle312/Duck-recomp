#include "texture_stage_index.h"
#include <unordered_map>
#include <cassert>
#include <cstdio>
int main(){
 std::unordered_map<DuckTextureStageKey,uint32_t,DuckTextureStageKey::Hasher> map;
 const DuckTextureStageKey original{0x1000,512,32,8};map.emplace(original,17);
 assert(map.at(original)==17);
 DuckTextureStageKey changed=original;changed.size++;assert(map.find(changed)==map.end());
 changed=original;changed.address++;assert(map.find(changed)==map.end());
 changed=original;changed.row_bytes++;assert(map.find(changed)==map.end());
 changed=original;changed.swizzle_group=16;assert(map.find(changed)==map.end());
 // Rehash/reallocation does not invalidate the array indices stored as values.
 for(unsigned i=0;i<4096;i++)map.emplace(DuckTextureStageKey{0x2000+i*8,32+i,16,8},i);
 assert(map.at(original)==17);
 for(unsigned i=0;i<4096;i++)assert(map.at(DuckTextureStageKey{0x2000+i*8,32+i,16,8})==i);
 puts("PASS: exact staged texture identity and stable indices through cache growth");
}
