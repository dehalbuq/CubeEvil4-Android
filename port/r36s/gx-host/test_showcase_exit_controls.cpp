// Regression tests: gameplay A/B presses must NEVER show the exit overlay.
// No real game files, graphical driver, or physical joystick needed.
#include "r36s_showcase_exit_controls.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
using namespace r36s_showcase_exit;
static Clock::time_point at(int ms){
  return Clock::time_point{}+std::chrono::hours(1)+std::chrono::milliseconds(ms);
}
static void warm(Gestures& g) {
  g.update({},false,at(0));
  g.update({},false,at(400));
}
int main() {
  {
    Gestures g;
    warm(g);
    Snapshot s{};
    // The old bug: physical raw A index 0 incorrectly served as HOME
    // and made the exit dialog appear on *every* ordinary A tap.
    for (int i=0;i<70;i++) {
      s.a=true;s.b=false;
      const auto a=g.update(s,false,at(500+i*100));
      assert(!a.toggleMenu && !a.emergencyExit);
      s.a=false;s.b=true;
      const auto b=g.update(s,false,at(550+i*100));
      assert(!b.toggleMenu && !b.emergencyExit);
    }
  }
  {
    Gestures g;warm(g);
    Snapshot s{};
    s.start=true;
    auto a=g.update(s,false,at(500));
    assert(!a.toggleMenu);
    assert(!g.update(s,false,at(3000)).toggleMenu);
    assert(!g.update(s,false,at(4490)).toggleMenu);
    assert(g.update(s,false,at(4500)).toggleMenu); // 4s hold
    assert(!g.update(s,true,at(7500)).toggleMenu); // no repeated toggles
    assert(!g.update(s,true,at(8490)).emergencyExit);
    assert(g.update(s,true,at(8500)).emergencyExit); // 8s emergency
  }
  {
    Gestures g;warm(g);
    Snapshot s{};
    s.select=true;
    assert(!g.update(s,false,at(1000)).toggleMenu);
    assert(!g.update(s,false,at(2000)).toggleMenu);
    assert(g.update(s,false,at(3000)).toggleMenu); // SELECT hold 2 seconds
    assert(!g.update(s,true,at(4000)).toggleMenu);
    s.select=false;
    g.update(s,true,at(4100));
    s.select=true;
    g.update(s,true,at(4200));
    assert(g.update(s,true,at(6200)).toggleMenu); // intentional second hold
  }
  {
    Gestures g;warm(g);
    Snapshot s{};s.start=true;s.select=true;
    g.update(s,false,at(500));
    assert(!g.update(s,false,at(2499)).emergencyExit);
    assert(g.update(s,false,at(2500)).emergencyExit); // 2s chord
  }
  {
    // A button held at launch cannot trigger menu when gesture arms.
    Gestures g;
    Snapshot s{};
    s.start=true;
    g.update(s,false,at(0));
    g.update(s,false,at(60000));
    assert(!g.update(s,false,at(64000)).toggleMenu);
    s.start=false;g.update(s,false,at(64100));
    assert(!g.update(s,false,at(64400)).toggleMenu);
    g.update(s,false,at(64500));
    s.start=true;
    assert(!g.update(s,false,at(64600)).toggleMenu);
    assert(!g.update(s,false,at(68000)).toggleMenu);
    assert(g.update(s,false,at(68600)).toggleMenu);
  }
  {
    Gestures g;warm(g);
    Snapshot s{};
    s.a=true;
    assert(!g.update(s,false,at(500)).confirmExit);
    assert(!g.update(s,true,at(600)).confirmExit);
    s.a=false;g.update(s,true,at(700));
    s.a=true;assert(g.update(s,true,at(710)).confirmExit);
    assert(!g.update(s,true,at(720)).confirmExit);
    s.a=false;g.update(s,true,at(730));
    s.b=true;assert(g.update(s,true,at(750)).cancelMenu);
    assert(!g.update(s,true,at(800)).cancelMenu);
  }
  {
    // No guessed raw SELECT/HOME or A. SDL-verified controller mapping,
    // or an explicit hardware override only.
    assert(Device::configured("RE4_SHOWCASE_RAW_SELECT",-1)==-1);
    assert(Device::configured("RE4_SHOWCASE_RAW_START",9)==9);
    setenv("RE4_SHOWCASE_RAW_SELECT","8",1);
    setenv("RE4_SHOWCASE_RAW_START","7",1);
    assert(Device::configured("RE4_SHOWCASE_RAW_SELECT",-1)==8);
    assert(Device::configured("RE4_SHOWCASE_RAW_START",9)==7);
    setenv("RE4_SHOWCASE_RAW_SELECT","-8",1);
    assert(Device::configured("RE4_SHOWCASE_RAW_SELECT",-1)==-1);
    unsetenv("RE4_SHOWCASE_RAW_SELECT");
    unsetenv("RE4_SHOWCASE_RAW_START");
  }
  std::cout<<"R36S_NO_PHANTOM_MENU_TEST_OK\n";
}
