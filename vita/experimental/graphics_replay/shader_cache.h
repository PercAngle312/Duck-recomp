#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
// Private, versioned envelope around vitaGL program binaries. Derived data is
// optional: rejected, interrupted or unwritable caches fall back to compilation.
namespace DuckShaderCache {
constexpr uint32_t Magic = 0x374b4344; // DCK7, little-endian ARM
constexpr uint32_t MaxBytes = 2 * 1024 * 1024;
struct Header { uint32_t magic, version, floats, format, bytes, checksum; };
static_assert(sizeof(Header) == 24);
inline uint32_t checksum(const uint8_t *data, size_t size) {
    uint32_t crc = ~0u;
    for (size_t i=0; i<size; ++i) {
        crc ^= data[i];
        for (unsigned j=0; j<8; ++j) crc=(crc>>1)^((crc&1)?0xedb88320u:0);
    }
    return ~crc;
}
inline bool read(const char *path, uint32_t floats, uint32_t &format, std::vector<uint8_t> &data) {
    data.clear();
    FILE *f=std::fopen(path,"rb"); if (!f) return false;
    Header h{};
    bool ok=std::fread(&h,1,sizeof h,f)==sizeof h && h.magic==Magic && h.version==1 &&
            h.floats==floats && h.bytes>0 && h.bytes<=MaxBytes;
    if (ok) {
        data.resize(h.bytes);
        ok=std::fread(data.data(),1,h.bytes,f)==h.bytes && std::fgetc(f)==EOF && !std::ferror(f) &&
           checksum(data.data(),data.size())==h.checksum;
    }
    if (std::fclose(f)!=0) ok=false;
    if (!ok) { data.clear(); return false; }
    format=h.format; return true;
}
inline bool write(const char *path, uint32_t floats, uint32_t format, const uint8_t *data, size_t bytes) {
    if (!data || !bytes || bytes>MaxBytes) return false;
    const std::string temporary=std::string(path)+".tmp";
    FILE *f=std::fopen(temporary.c_str(),"wb"); if (!f) return false;
    Header h{Magic,1,floats,format,(uint32_t)bytes,checksum(data,bytes)};
    bool ok=std::fwrite(&h,1,sizeof h,f)==sizeof h && std::fwrite(data,1,bytes,f)==bytes;
    if (std::fflush(f)!=0) ok=false;
    if (std::fclose(f)!=0) ok=false;
    if (ok) ok=std::rename(temporary.c_str(),path)==0;
    if (!ok) std::remove(temporary.c_str());
    return ok;
}
}
