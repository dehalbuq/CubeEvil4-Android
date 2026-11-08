// CubeEvil4 R36S standalone button mapping diagnostic (NO game assets).
// Read-only Linux evdev current-state polling + SDL raw joystick buttons,
// useful when a DarkOSRE service prevents SDL GameController START/SELECT.
// Does not grab or modify any device. This binary runs *outside the game*.
#include <SDL2/SDL.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>
using Clock=std::chrono::steady_clock;
static constexpr unsigned kBpWord=sizeof(unsigned long)*8;
static constexpr unsigned kWords=(KEY_MAX+kBpWord)/kBpWord;
static bool isDown(const unsigned long* bits,unsigned key){
  return key<=KEY_MAX && (bits[key/kBpWord]&(1ul<<(key%kBpWord)))!=0;
}
struct Ev {
  int fd=-1;
  std::string name,path;
  std::array<unsigned long,kWords> last{};
  bool initial=false;
};
struct Joy {
  SDL_Joystick* dev=nullptr;
  SDL_JoystickID id=-1;
  std::array<Uint8,64> last{};
  bool initial=false;
};
int main(int argc,char**argv){
  int seconds=argc>1?std::atoi(argv[1]):12;
  seconds=std::max(5,std::min(seconds,30));
  std::setvbuf(stdout,nullptr,_IONBF,0);
  std::printf("R36S_INPUT_PROBE_SECONDS=%d\n",seconds);
  std::printf("INSTRUCTIONS=press_SELECT_START_A_B_once_each_during_probe\n");
  std::vector<Ev> inputs;
  if(DIR* dir=::opendir("/dev/input")){
    while(const dirent* de=::readdir(dir)){
      if(std::strncmp(de->d_name,"event",5)!=0)continue;
      const char* n=de->d_name+5;bool numeric=*n!=0;
      while(*n) {if(*n<'0'||*n>'9')numeric=false;++n;}
      if(!numeric)continue;
      std::string path=std::string("/dev/input/")+de->d_name;
      int fd=::open(path.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);
      if(fd<0){std::printf("EVDEV_OPEN_DENIED=%s\n",path.c_str());continue;}
      unsigned long keys[kWords]{};
      if(::ioctl(fd,EVIOCGBIT(EV_KEY,sizeof(keys)),keys)<0){
        ::close(fd);continue;
      }
      char name[128]{};
      ::ioctl(fd,EVIOCGNAME(sizeof(name)),name);
      Ev d{};
      d.fd=fd;d.path=path;d.name=name;
      inputs.push_back(d);
      std::printf("EVDEV_OPENED PATH=%s NAME=%s\n",path.c_str(),name);
    }
    ::closedir(dir);
  }else std::printf("EVDEV_ERROR=unable_to_open_/dev/input\n");
  std::vector<Joy> joysticks;
  bool hasSDL=SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_EVENTS)==0;
  if(hasSDL){
    SDL_JoystickEventState(SDL_ENABLE);
    std::printf("SDL_JOYSTICKS=%d\n",SDL_NumJoysticks());
    for(int i=0;i<SDL_NumJoysticks();i++){
      SDL_Joystick* dev=SDL_JoystickOpen(i);
      if(!dev)continue;
      Joy j{};j.dev=dev;j.id=SDL_JoystickInstanceID(dev);
      joysticks.push_back(j);
      std::printf("SDL_JOY_INDEX=%d ID=%d NAME=%s BUTTONS=%d\n",
          i,static_cast<int>(j.id),SDL_JoystickName(dev),SDL_JoystickNumButtons(dev));
    }
  }else std::printf("SDL_JOYSTICK_ERROR=%s\n",SDL_GetError());
  const auto start=Clock::now(),until=start+std::chrono::seconds(seconds);
  while(Clock::now()<until){
    if(hasSDL){SDL_PumpEvents();SDL_JoystickUpdate();}
    for(Joy& joy:joysticks){
      if(!SDL_JoystickGetAttached(joy.dev))continue;
      for(int b=0;b<std::min(SDL_JoystickNumButtons(joy.dev),64);b++){
        const Uint8 down=SDL_JoystickGetButton(joy.dev,b)?1:0;
        if(joy.initial && down!=joy.last[b]) {
          std::printf("SDL_RAW_BUTTON ID=%d BUTTON=%d STATE=%s\n",
                      static_cast<int>(joy.id),b,down?"DOWN":"UP");
        }
        joy.last[b]=down;
      }
      joy.initial=true;
    }
    for(Ev& d:inputs){
      std::array<unsigned long,kWords> down{};
      if(::ioctl(d.fd,EVIOCGKEY(sizeof(down)),down)<0)continue;
      if(d.initial){
        for(unsigned key=0;key<=KEY_MAX;key++){
          const bool before=isDown(d.last.data(),key);
          const bool after=isDown(down.data(),key);
          if(after!=before){
            std::printf("LINUX_KEY PATH=%s CODE=%u STATE=%s\n",
              d.path.c_str(),key,after?"DOWN":"UP");
          }
        }
      }
      d.last=down;d.initial=true;
    }
    SDL_Delay(40);
  }
  for(Joy& j:joysticks)if(j.dev)SDL_JoystickClose(j.dev);
  if(hasSDL)SDL_Quit();
  for(Ev& d:inputs)if(d.fd>=0)::close(d.fd);
  std::printf("R36S_INPUT_PROBE_COMPLETE=YES\n");
  return 0;
}
