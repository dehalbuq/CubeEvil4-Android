// R36S showcase: create a temporary non-mutating view of user-owned game data.
// Only debug/config.txt is copied, with [PRINT_PAGE] numeric value forced to 0.
// Original discs, FST, game resources and memory card remain in their locations.
// Called only by RE4_R36S_SHOWCASE, not by the diagnostic/stable executables.
#pragma once
#ifndef R36S_SHOWCASE_CLEAN_DATA_H
#define R36S_SHOWCASE_CLEAN_DATA_H

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <system_error>

struct ShowcaseCleanData {
  std::string root;
  ShowcaseCleanData() = default;
  ShowcaseCleanData(const ShowcaseCleanData&) = delete;
  ShowcaseCleanData& operator=(const ShowcaseCleanData&) = delete;
  ~ShowcaseCleanData(){cleanup();}

  static bool disableDebugPrintPage(std::string &data){
    // The original code parses a [PRINT_PAGE] tag and then num_get() after
    // whitespace. 0 disables EprintfDrawing() and DebugControl() overlays.
    const std::string key="[PRINT_PAGE]";
    std::size_t pos=data.find(key);
    if(pos==std::string::npos)return false;
    pos+=key.size();
    while(pos<data.size() && std::isspace(static_cast<unsigned char>(data[pos])))pos++;
    if(pos==data.size() || !std::isdigit(static_cast<unsigned char>(data[pos])))return false;
    while(pos<data.size() && std::isdigit(static_cast<unsigned char>(data[pos])))
      data[pos++]='0';  // preserve file length, FAT/FST expected file size
    return true;
  }

  void cleanup(){
    if(root.empty())return;
    std::error_code ec;
    std::filesystem::remove_all(root,ec);
    root.clear();
  }
  const std::string& path() const {return root;}

  bool prepare(const std::string &coreDir, const std::string &dataDir) {
    namespace fs=std::filesystem;
    cleanup();
    try {
      const fs::path original=fs::absolute(fs::path(dataDir));
      const fs::path config=original/"disc0"/"files"/"debug"/"config.txt";
      std::ifstream source(config,std::ios::binary);
      if(!source)return false;
      std::string originalConfig(std::istreambuf_iterator<char>{source},
                                 std::istreambuf_iterator<char>{});
      if(!disableDebugPrintPage(originalConfig))return false;
      const fs::path cache=fs::path(coreDir)/"cache";
      fs::create_directories(cache);
      std::string pattern=(cache/"showcase-clean-XXXXXX").string();
      std::vector<char> tmp(pattern.begin(),pattern.end());
      tmp.push_back('\0');
      char* created=mkdtemp(tmp.data());
      if(!created)return false;
      root=created;
      const fs::path dst(root);
      fs::create_directories(dst/"disc0"/"files"/"debug");
      // Mirror the original extracted ISO directory structure by symlinks,
      // *not* copies. Only the specific debug config is new and writable.
      for(const auto& item:fs::directory_iterator(original)) {
        if(item.path().filename()=="disc0")continue;
        fs::create_symlink(item.path(),dst/item.path().filename());
      }
      const fs::path disc0=original/"disc0";
      for(const auto& item:fs::directory_iterator(disc0)) {
        if(item.path().filename()=="files")continue;
        fs::create_symlink(item.path(),dst/"disc0"/item.path().filename());
      }
      const fs::path files=disc0/"files";
      for(const auto& item:fs::directory_iterator(files)) {
        if(item.path().filename()=="debug")continue;
        fs::create_symlink(item.path(),dst/"disc0"/"files"/item.path().filename());
      }
      const fs::path debug=files/"debug";
      for(const auto& item:fs::directory_iterator(debug)) {
        if(item.path().filename()=="config.txt")continue;
        fs::create_symlink(item.path(),dst/"disc0"/"files"/"debug"/item.path().filename());
      }
      std::ofstream target(dst/"disc0"/"files"/"debug"/"config.txt",
                           std::ios::binary|std::ios::trunc);
      if(!target) {cleanup();return false;}
      target.write(originalConfig.data(),static_cast<std::streamsize>(originalConfig.size()));
      target.close();
      if(!target) {cleanup();return false;}
      return true;
    }catch(const std::exception&){
      cleanup();
      return false;
    }
  }
};
#endif
