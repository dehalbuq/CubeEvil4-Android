// Synthetic offline test only: checks virtual view and fixed-length
// [PRINT_PAGE] patching without any extracted commercial GameCube files.
#include "r36s_showcase_clean_data.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>

static std::string slurp(const std::filesystem::path &f) {
  std::ifstream in(f,std::ios::binary);
  return {std::istreambuf_iterator<char>{in},std::istreambuf_iterator<char>{}};
}
static void save(const std::filesystem::path &f,const std::string &s){
  std::ofstream out(f,std::ios::binary);
  out.write(s.data(),static_cast<std::streamsize>(s.size()));
}
int main(){
  namespace fs=std::filesystem;
  std::string test="[PRINT_PAGE]\r\n  17\r\n[PROCESS_BAR] ON\r\n";
  assert(ShowcaseCleanData::disableDebugPrintPage(test));
  assert(test=="[PRINT_PAGE]\r\n  00\r\n[PROCESS_BAR] ON\r\n");
  assert(!ShowcaseCleanData::disableDebugPrintPage(test="[PLAYER] 3\r\n"));
  std::string tmp="/tmp/showcase-test-XXXXXX";
  std::vector<char> tag(tmp.begin(),tmp.end());
  tag.push_back('\0');
  char* base=mkdtemp(tag.data());
  assert(base);
  fs::path root(base),data=root/"game_data",core=root/"core";
  fs::create_directories(data/"disc0/files/debug");
  fs::create_directories(data/"disc0/sys");
  fs::create_directories(data/"memcard");
  fs::create_directories(data/"disc1");
  save(data/"disc0/sys/boot.bin","G4BE08");
  save(data/"disc0/files/debug/config.txt","[PRINT_PAGE] 17\r\n[PROCESS_BAR] ON\r\n");
  save(data/"disc0/files/debug/other.txt","HELLO");
  save(data/"disc0/files/synthetic.dat","PIXELS");
  save(data/"memcard/save.dat","SAVE");
  {
    ShowcaseCleanData view;
    assert(view.prepare(core.string(),data.string()));
    assert(fs::exists(view.path()+"/disc0/sys/boot.bin"));
    assert(fs::exists(view.path()+"/disc0/files/synthetic.dat"));
    assert(fs::exists(view.path()+"/memcard/save.dat"));
    assert(fs::exists(view.path()+"/disc0/files/debug/other.txt"));
    assert(slurp(data/"disc0/files/debug/config.txt")=="[PRINT_PAGE] 17\r\n[PROCESS_BAR] ON\r\n");
    assert(slurp(fs::path(view.path())/"disc0/files/debug/config.txt")=="[PRINT_PAGE] 00\r\n[PROCESS_BAR] ON\r\n");
    assert(!fs::is_symlink(fs::path(view.path())/"disc0/files/debug/config.txt"));
    assert(fs::is_symlink(fs::path(view.path())/"memcard"));
    assert(fs::is_symlink(fs::path(view.path())/"disc0/sys"));
  }
  assert(slurp(data/"disc0/files/debug/config.txt")=="[PRINT_PAGE] 17\r\n[PROCESS_BAR] ON\r\n");
  assert(slurp(data/"memcard/save.dat")=="SAVE");
  std::error_code ec;
  fs::remove_all(root,ec);
  assert(!ec);
  std::cout<<"SHOWCASE_CLEAN_CONFIG_OVERLAY_TEST_OK\n";
  return 0;
}
