#pragma once
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <array>
#include <string>
namespace duck::rom {
enum class Status { found, missing, invalid, ambiguous };
struct Result { Status status=Status::missing; std::filesystem::path path; };
inline bool is_n64_big_endian(const std::filesystem::path& p) {
  std::error_code ec;
  if (!std::filesystem::is_regular_file(p,ec) || ec || std::filesystem::file_size(p,ec)!=16777216ULL || ec) return false;
  std::ifstream in(p,std::ios::binary);
  std::array<unsigned char,4> magic{};
  in.read(reinterpret_cast<char*>(magic.data()),4);
  if (!in || magic != std::array<unsigned char,4>{0x80,0x37,0x12,0x40}) return false;
  // Recognize the supported Duck Dodgers USA multilingual cartridge header.
  // Cartridge CRC+ID validation is not a cryptographic identity check.
  std::array<unsigned char,8> crc{};
  in.seekg(0x10);
  in.read(reinterpret_cast<char*>(crc.data()),crc.size());
  if (!in || crc != std::array<unsigned char,8>{0xFB,0xB9,0xF1,0xFA,0x6B,0xF8,0x86,0x89}) return false;
  std::array<unsigned char,3> id{};
  in.seekg(0x3B);
  in.read(reinterpret_cast<char*>(id.data()),id.size());
  return in.good() && id == std::array<unsigned char,3>{'N','D','U'};
}
inline Result locate(const std::filesystem::path& executable_directory) {
  const auto folder=executable_directory/"roms";
  std::error_code ec;
  if (!std::filesystem::is_directory(folder,ec)) return {};
  Result r;
  for (const auto& entry:std::filesystem::directory_iterator(folder,ec)) {
    if(ec) break;
    auto extension=entry.path().extension().string();
    for(char& c:extension) if(c>='A'&&c<='Z') c=char(c-'A'+'a');
    if(extension!=".z64") continue;
    if (!is_n64_big_endian(entry.path())) {
      if(r.status==Status::missing) r.status=Status::invalid;
      continue;
    }
    if(r.status==Status::found) return {Status::ambiguous,{}};
    r={Status::found,entry.path()};
  }
  return r;
}
}
