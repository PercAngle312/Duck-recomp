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
  return in.good() && magic == std::array<unsigned char,4>{0x80,0x37,0x12,0x40};
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
