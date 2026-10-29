#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
// Cache owns snapshots, never pointers into guest memory. Host pointers belong
// to the task arena, so collisions cannot overwrite an earlier command's data.
template<size_t RawBytes,size_t ConvertedBytes,size_t Slots>
struct DuckConversionCache {
 struct Entry {
  alignas(16) uint8_t raw[RawBytes]{},converted[ConvertedBytes]{};
  uint32_t address=0,tag=0,build=0;size_t raw_size=0,converted_size=0;
  void *task_output=nullptr;bool valid=false;
 } entries[Slots];
 uint32_t hits=0,misses=0,task_reuses=0;
 static size_t slot(uint32_t address,uint32_t tag){return ((address>>4)^(address>>12)^(tag*0x9e3779b1u))%Slots;}
 Entry *find(uint32_t address,uint32_t tag,const void *raw,size_t raw_size,size_t converted_size){
  if(!raw||!raw_size||raw_size>RawBytes||!converted_size||converted_size>ConvertedBytes){++misses;return nullptr;}
  Entry &e=entries[slot(address,tag)];
  if(e.valid&&e.address==address&&e.tag==tag&&e.raw_size==raw_size&&e.converted_size==converted_size&&std::memcmp(e.raw,raw,raw_size)==0){++hits;return &e;}
  ++misses;return nullptr;
 }
 template<class Allocate>void *restore(Entry &e,uint32_t build,Allocate allocate){
  if(e.task_output&&e.build==build){++task_reuses;return e.task_output;}
  void *out=allocate(e.converted_size);if(!out)return nullptr;
  std::memcpy(out,e.converted,e.converted_size);e.task_output=out;e.build=build;return out;
 }
 void store(uint32_t address,uint32_t tag,const void *raw,size_t raw_size,const void *converted,size_t converted_size,void *task_output,uint32_t build){
  if(!raw||!converted||!raw_size||raw_size>RawBytes||!converted_size||converted_size>ConvertedBytes)return;
  Entry &e=entries[slot(address,tag)];std::memcpy(e.raw,raw,raw_size);std::memcpy(e.converted,converted,converted_size);
  e.address=address;e.tag=tag;e.raw_size=raw_size;e.converted_size=converted_size;e.task_output=task_output;e.build=build;e.valid=true;
 }
 void invalidate_task_pointers(){for(auto &e:entries){e.task_output=nullptr;e.build=0;}}
};
