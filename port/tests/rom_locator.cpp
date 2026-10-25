#include "../runtime/rom_locator.h"
#include <cstdio>
#include <fstream>
int main() {
  namespace fs=std::filesystem;
  auto root=fs::temp_directory_path()/"duck-rom-locator-test";
  std::error_code ec;
  fs::remove_all(root,ec);
  fs::create_directories(root/"roms",ec);
  if(ec)return 1;
  if(duck::rom::locate(root).status!=duck::rom::Status::missing)return 2;
  auto fake=root/"roms"/"Duck.z64";
  {std::ofstream out(fake,std::ios::binary);out<<"junk";}
  if(duck::rom::locate(root).status!=duck::rom::Status::invalid)return 3;
  {
    std::ofstream out(fake,std::ios::binary|std::ios::trunc);
    const char magic[]={char(0x80),char(0x37),char(0x12),char(0x40)};
    out.write(magic,4);
    const unsigned char crc[]={0xFB,0xB9,0xF1,0xFA,0x6B,0xF8,0x86,0x89};
    out.seekp(0x10);out.write(reinterpret_cast<const char*>(crc),8);
    out.seekp(0x3B);out.write("NDU",3);
    out.seekp(16777216-1);out.put('\0');
  }
  auto found=duck::rom::locate(root);
  if(found.status!=duck::rom::Status::found||found.path!=fake)return 4;
  // A different game with the correct byte order and size must be rejected.
  {
    std::fstream io(fake,std::ios::binary|std::ios::in|std::ios::out);
    io.seekp(0x10);io.put(char(0));
  }
  if(duck::rom::locate(root).status!=duck::rom::Status::invalid)return 7;
  {
    std::fstream io(fake,std::ios::binary|std::ios::in|std::ios::out);
    io.seekp(0x10);io.put(char(0xFB));
  }
  if(duck::rom::locate(root).status!=duck::rom::Status::found)return 8;
  fs::copy_file(fake,root/"roms"/"Duck2.z64",fs::copy_options::overwrite_existing,ec);
  if(ec||duck::rom::locate(root).status!=duck::rom::Status::ambiguous)return 5;
  fs::remove_all(root,ec);
  if(ec)return 6;
  std::puts("PASS portable ROM autodetection");return 0;
}
