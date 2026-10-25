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
    out.write(magic,4);out.seekp(16777216-1);out.put('\0');
  }
  auto found=duck::rom::locate(root);
  if(found.status!=duck::rom::Status::found||found.path!=fake)return 4;
  fs::copy_file(fake,root/"roms"/"Duck2.z64",fs::copy_options::overwrite_existing,ec);
  if(ec||duck::rom::locate(root).status!=duck::rom::Status::ambiguous)return 5;
  fs::remove_all(root,ec);
  if(ec)return 6;
  std::puts("PASS portable ROM autodetection");return 0;
}
