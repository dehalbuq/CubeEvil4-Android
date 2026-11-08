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

  // Preserve the original disc byte-for-byte. The debug text setting can
  // be present in either the conventional disc0/files/ tree or a flat disc0/
  // layout. Report the real reason when creating the overlay fails.
  std::string error;
  bool prepare(const std::string &coreDir, const std::string &dataDir) {
    namespace fs=std::filesystem;
    cleanup();
    error.clear();
    try {
      const fs::path original=fs::absolute(fs::path(dataDir));
      const fs::path disc0=original/"disc0";
      const bool withFiles=fs::is_directory(disc0/"files");
      const fs::path files=withFiles?disc0/"files":disc0;
      const fs::path config=files/"debug"/"config.txt";
      std::ifstream source(config,std::ios::binary);
      if(!source){error="debug_config_unavailable: "+config.string();return false;}
      std::string patched(std::istreambuf_iterator<char>{source},
                          std::istreambuf_iterator<char>{});
      if(!disableDebugPrintPage(patched)){
        error="PRINT_PAGE_tag_absent_or_non_numeric: "+config.string();
        return false;
      }
      const fs::path cache=fs::path(coreDir)/"cache";
      fs::create_directories(cache);
      std::string pattern=(cache/"showcase-clean-XXXXXX").string();
      std::vector<char> tmp(pattern.begin(),pattern.end());
      tmp.push_back('\0');
      char* created=mkdtemp(tmp.data());
      if(!created){error="mkdtemp_failed: "+pattern;return false;}
      root=created;
      const fs::path dst(root),dstDisc0=dst/"disc0";
      const fs::path dstFiles=withFiles?dstDisc0/"files":dstDisc0;
      fs::create_directories(dstFiles/"debug");
      for(const auto& item:fs::directory_iterator(original)) {
        if(item.path().filename()=="disc0")continue;
        fs::create_symlink(item.path(),dst/item.path().filename());
      }
      for(const auto& item:fs::directory_iterator(disc0)) {
        if(item.path().filename()==(withFiles?"files":"debug"))continue;
        fs::create_symlink(item.path(),dstDisc0/item.path().filename());
      }
      if(withFiles) {
        for(const auto& item:fs::directory_iterator(files)) {
          if(item.path().filename()=="debug")continue;
          fs::create_symlink(item.path(),dstFiles/item.path().filename());
        }
      }
      for(const auto& item:fs::directory_iterator(files/"debug")) {
        if(item.path().filename()=="config.txt")continue;
        fs::create_symlink(item.path(),dstFiles/"debug"/item.path().filename());
      }
      std::ofstream target(dstFiles/"debug"/"config.txt",
                           std::ios::binary|std::ios::trunc);
      if(!target) {
        error="overlay_write_open_failed";
        cleanup();
        return false;
      }
      target.write(patched.data(),static_cast<std::streamsize>(patched.size()));
      target.close();
      if(!target) {
        error="overlay_write_failed";
        cleanup();
        return false;
      }
      return true;
    }catch(const std::exception& e){
      error=std::string("overlay_exception: ")+e.what();
      cleanup();
      return false;
    }
  }
};
#endif
