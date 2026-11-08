// R36S DarkOSRE: independent physical START/SELECT state from Linux evdev.
// The game can use SDL_GameController for movement while the firmware handles
// START/SELECT as Linux BTN_START/BTN_SELECT (or BTN_TRIGGER_HAPPY*).
// Do NOT infer A/HOME from raw joystick index 0: that caused phantom menus.
//
// Unlike read(EV_KEY) events, EVIOCGKEY reports current button state, and
// can still work if a firmware hotkey daemon has grabbed input events.
// O_RDONLY only; no EVIOCGRAB, no global hotkey modifications, no kernel writes.
#pragma once
#ifndef R36S_SHOWCASE_EVDEV_EXIT_H
#define R36S_SHOWCASE_EVDEV_EXIT_H

#if defined(__linux__)
#include <linux/input.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
#endif

namespace r36s_showcase_exit {

struct KernelButtons {
  bool start=false,select=false;
};

#if defined(__linux__)
class EvdevButtons {
 public:
  static constexpr unsigned kBitsPerWord=sizeof(unsigned long)*8u;
  static constexpr unsigned kWordCount=(KEY_MAX+kBitsPerWord)/kBitsPerWord;
  struct Layout { int start=-1,select=-1; };
  static bool bit(const unsigned long* words,int code) {
    return code>=0 && code<=KEY_MAX &&
           (words[static_cast<unsigned>(code)/kBitsPerWord] &
           (1ul<<(static_cast<unsigned>(code)%kBitsPerWord)))!=0;
  }
  static Layout choose(const unsigned long* capabilities) {
    Layout out{};
    // GPIO gamepads (including R36S DTS mappings).
    if(bit(capabilities,BTN_START)||bit(capabilities,BTN_SELECT)) {
      if(bit(capabilities,BTN_START))out.start=BTN_START;
      if(bit(capabilities,BTN_SELECT))out.select=BTN_SELECT;
      return out;
    }
    // Older RK3326 / OGA-style retrogame joypad mappings: OGA F1/F6.
    // Only identify these as controller hotkeys if there is a paired SELECT.
    if(bit(capabilities,BTN_TRIGGER_HAPPY1)) {
      if(bit(capabilities,BTN_TRIGGER_HAPPY6)) {
        out.start=BTN_TRIGGER_HAPPY6;
        out.select=BTN_TRIGGER_HAPPY1;
      } else if(bit(capabilities,BTN_TRIGGER_HAPPY2)) {
        out.start=BTN_TRIGGER_HAPPY2;
        out.select=BTN_TRIGGER_HAPPY1;
      }
    }
    return out;
  }

  void scan() {
    closeAll();
    if(const char* disabled=std::getenv("RE4_SHOWCASE_NO_EVDEV"))
      if(std::strcmp(disabled,"1")==0)return;
    DIR* dir=::opendir("/dev/input");
    if(!dir)return;
    while(const dirent* item=::readdir(dir)) {
      if(std::strncmp(item->d_name,"event",5)!=0)continue;
      const char* digits=item->d_name+5;
      if(!*digits)continue;
      bool valid=true;
      for(const char* p=digits;*p;p++)if(*p<'0'||*p>'9')valid=false;
      if(!valid)continue;
      const std::string path=std::string("/dev/input/")+item->d_name;
      int fd=::open(path.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);
      if(fd<0)continue;
      unsigned long supported[kWordCount]{};
      const int got=::ioctl(fd,EVIOCGBIT(EV_KEY,sizeof(supported)),supported);
      if(got<0) {::close(fd);continue;}
      const Layout layout=choose(supported);
      if(layout.start<0) {::close(fd);continue;}
      char name[128]{};
      ::ioctl(fd,EVIOCGNAME(sizeof(name)),name);
      Device info{};
      info.fd=fd;
      info.path=path;
      info.name=name;
      info.layout=layout;
      devices_.push_back(info);
      // Physical handheld has a small finite number of input devices.
      if(devices_.size()>=12)break;
    }
    ::closedir(dir);
  }

  // Passive poll independent of SDL's controller event queue. A device
  // denied by Linux permissions is safely ignored; the SDL route remains.
  KernelButtons poll()const {
    KernelButtons answer{};
    for(const Device& d:devices_) {
      unsigned long down[kWordCount]{};
      if(::ioctl(d.fd,EVIOCGKEY(sizeof(down)),down)<0)continue;
      answer.start |= bit(down,d.layout.start);
      answer.select |= bit(down,d.layout.select);
    }
    return answer;
  }
  void report(FILE* f)const {
    if(!f)return;
    std::fprintf(f,"R36S_EVDEV_SUPPORTED_DEVICES=%zu\n",devices_.size());
    for(const Device& d:devices_) {
      std::fprintf(f,"R36S_EVDEV_INPUT PATH=%s NAME=%s START_CODE=%d SELECT_CODE=%d\n",
                   d.path.c_str(),d.name.c_str(),d.layout.start,d.layout.select);
    }
  }
  void closeAll() {
    for(Device& d:devices_)if(d.fd>=0)::close(d.fd);
    devices_.clear();
  }
  ~EvdevButtons(){closeAll();}
  EvdevButtons()=default;
  EvdevButtons(const EvdevButtons&)=delete;
  EvdevButtons& operator=(const EvdevButtons&)=delete;
 private:
  struct Device {
    int fd=-1;
    std::string path,name;
    Layout layout;
  };
  std::vector<Device> devices_;
};
#else
class EvdevButtons {
 public:
  void scan(){}
  KernelButtons poll()const{return {};}
  void report(void*)const{}
  void closeAll(){}
};
#endif
} // namespace r36s_showcase_exit
#endif
