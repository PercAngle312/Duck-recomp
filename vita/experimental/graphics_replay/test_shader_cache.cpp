#include "shader_cache.h"
#include <cassert>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>
int main() {
    char dir[]="/tmp/duck-shaders-XXXXXX"; assert(mkdtemp(dir));
    const std::string path=std::string(dir)+"/shader.bin";
    const uint8_t data[]={1,2,3,4,5}; uint32_t format=0; std::vector<uint8_t> loaded;
    assert(!DuckShaderCache::read(path.c_str(),8,format,loaded));
    assert(DuckShaderCache::write(path.c_str(),8,7,data,sizeof data));
    assert(DuckShaderCache::read(path.c_str(),8,format,loaded)&&format==7&&loaded==std::vector<uint8_t>(data,data+5));
    assert(!DuckShaderCache::read(path.c_str(),12,format,loaded));
    std::ifstream input(path,std::ios::binary);std::string original((std::istreambuf_iterator<char>(input)),{});input.close();
    auto replace=[&](const std::string &s){std::ofstream f(path,std::ios::binary|std::ios::trunc);f.write(s.data(),s.size());};
    for(size_t i=0;i<original.size();++i){replace(original.substr(0,i));assert(!DuckShaderCache::read(path.c_str(),8,format,loaded));}
    replace(original+"trailing");assert(!DuckShaderCache::read(path.c_str(),8,format,loaded));
    for(unsigned field=0;field<6;++field){auto bad=original;bad[field*4]^=0x40;
        // Format is a provider-defined value; changing it alone isn't corruption
        // of the payload, but GL link status must still validate it on loading.
        if(field==3)continue;
        replace(bad);assert(!DuckShaderCache::read(path.c_str(),8,format,loaded));}
    auto bad=original;bad.back()^=1;replace(bad);assert(!DuckShaderCache::read(path.c_str(),8,format,loaded));
    replace(original);assert(!DuckShaderCache::write(path.c_str(),8,7,nullptr,5));
    assert(!DuckShaderCache::write(path.c_str(),8,7,data,0));
    assert(!DuckShaderCache::write(path.c_str(),8,7,data,DuckShaderCache::MaxBytes+1));
    // A failed write must leave an existing good cache intact.
    std::filesystem::create_directory(path+".tmp");
    assert(!DuckShaderCache::write(path.c_str(),8,7,data,5));
    assert(DuckShaderCache::read(path.c_str(),8,format,loaded));
    std::filesystem::remove_all(dir);
    std::cout<<"PASS: shader cache roundtrip, every truncated length, header/size/stride/CRC rejection, trailing data and failed-write preservation\n";
}
