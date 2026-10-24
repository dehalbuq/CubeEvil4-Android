// Tests synthetic SDL-independent gesture timelines plus a virtual joystick.
// No proprietary RE4 assets, saves, or DarkOSRE hardware are required.
#include "r36s_showcase_exit_controls.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
using namespace r36s_showcase_exit;
static const Clock::time_point origin = Clock::time_point{}+std::chrono::hours(1);
static Clock::time_point at(int milliseconds) {
  return origin+std::chrono::milliseconds(milliseconds);
}
int main(){
  {
    Gestures g;
    Snapshot s{};
    assert(!g.update(s,false,at(0)).toggleMenu);
    s.select=true;
    assert(g.update(s,false,at(40)).toggleMenu);
    assert(!g.update(s,true,at(100)).toggleMenu); // long press must not repeat
    s.select=false;
    g.update(s,true,at(150));
    s.select=true;
    assert(g.update(s,true,at(210)).toggleMenu);  // tap again closes
    s.select=false;
    g.update(s,false,at(280));
    s.home=true;
    assert(g.update(s,false,at(290)).toggleMenu);
    s.home=false;
    g.update(s,true,at(320));
    s.a=true;
    assert(g.update(s,true,at(335)).confirmExit);
    assert(!g.update(s,true,at(450)).confirmExit);
    s.a=false;g.update(s,true,at(500));
    s.b=true;
    assert(g.update(s,true,at(530)).cancelMenu);
  }
  {
    Gestures g;
    Snapshot s{};
    s.start=true;
    assert(!g.update(s,false,at(0)).toggleMenu);
    assert(!g.update(s,false,at(2499)).toggleMenu);
    assert(g.update(s,false,at(2500)).toggleMenu); // START-only opens menu
    assert(!g.update(s,true,at(3100)).toggleMenu);
    assert(!g.update(s,true,at(5999)).emergencyExit);
    assert(g.update(s,true,at(6000)).emergencyExit); // direct quit, even if menu stuck
  }
  {
    Gestures g;
    Snapshot s{};s.start=true;s.select=true;
    assert(g.update(s,false,at(0)).toggleMenu); // SELECT also opens menu
    assert(!g.update(s,true,at(1490)).emergencyExit);
    assert(g.update(s,true,at(1500)).emergencyExit); // SELECT+START direct quit
  }
  {
    Gestures g;
    Snapshot s{};s.start=true;s.leftShoulder=true;s.rightShoulder=true;
    assert(!g.update(s,false,at(0)).toggleMenu);
    assert(g.update(s,false,at(1000)).toggleMenu);
    assert(!g.update(s,true,at(1020)).toggleMenu);
  }
  {
    // Releasing START resets the emergency 6-second timer.
    Gestures g;
    Snapshot s{};s.start=true;
    g.update(s,false,at(0));
    s.start=false;
    g.update(s,false,at(3900));
    s.start=true;
    assert(!g.update(s,false,at(4000)).emergencyExit);
    assert(!g.update(s,false,at(8000)).emergencyExit);
  }
  {
    setenv("SDL_VIDEODRIVER","dummy",0);
    assert(SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)==0);
#if SDL_VERSION_ATLEAST(2,0,14)
    const int index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,12,0,0);
    assert(index>=0);
    Device d;
    d.attach(nullptr); // raw joystick fallback without SDL GameController
    SDL_Joystick* virtualDevice=SDL_JoystickFromDeviceIndex(index);
    assert(virtualDevice);
    assert(SDL_JoystickSetVirtualButton(virtualDevice,8,1)==0);
    assert(SDL_JoystickSetVirtualButton(virtualDevice,9,1)==0);
    SDL_JoystickUpdate();
    const auto input=d.poll();
    assert(input.select);
    assert(input.start);
    assert(d.matchesRaw(SDL_JoystickInstanceID(virtualDevice),8,0));
    assert(d.matchesRaw(SDL_JoystickInstanceID(virtualDevice),9,1));
    d.release();
    SDL_JoystickDetachVirtual(index);
#endif
    SDL_Quit();
  }
  std::cout<<"R36S_SHOWCASE_EXIT_INPUT_TEST_OK\n";
}
