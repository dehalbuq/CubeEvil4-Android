// R36S / DarkOSRE experimental showcase: deliberate, debounced exit controls.
// Never infer SELECT/HOME from arbitrary raw button numbers: in some mappings,
// joystick button 0 is the ordinary in-game A button. Treating it as HOME was
// causing the exit overlay to repeatedly appear during gameplay.
#pragma once
#ifndef R36S_SHOWCASE_EXIT_CONTROLS_H
#define R36S_SHOWCASE_EXIT_CONTROLS_H

#include <SDL2/SDL.h>
#include <chrono>
#include <cstdlib>

namespace r36s_showcase_exit {
using Clock = std::chrono::steady_clock;

struct Snapshot {
  bool select=false;
  bool start=false;
  bool a=false;
  bool b=false;
};

struct Actions {
  bool toggleMenu=false;
  bool confirmExit=false;
  bool cancelMenu=false;
  bool emergencyExit=false;
};

// Gameplay must not open the exit menu via a quick SELECT, HOME, A, B,
// or START press. Only intentional holds are accepted:
//  - SELECT held 2s toggles menu, if genuinely mapped
//  - START held 4s opens menu (once), 8s exits even if A/B don't work
//  - START+SELECT held 2s exits (only if both genuinely mapped).
// We additionally wait for a released state after startup, and never
// trigger a second menu toggle while the same press stays held.
class Gestures {
 public:
  Actions update(const Snapshot& in,bool menuVisible,Clock::time_point now) {
    Actions out{};
    if(!armed_) {
      // Many R36S input devices report a button held at app startup due
      // to the Ports launcher. Never interpret that as a showcase command.
      if(in.start||in.select)releaseAt_=Clock::time_point{};
      else if(releaseAt_==Clock::time_point{})releaseAt_=now;
      else if(now-releaseAt_>=std::chrono::milliseconds(350))armed_=true;
      prevA_=in.a;
      prevB_=in.b;
      prevStart_=in.start;
      prevSelect_=in.select;
      return out;
    }

    if(in.start && !prevStart_) {startAt_=now;startMenuFired_=false;}
    if(in.select && !prevSelect_) {selectAt_=now;selectMenuFired_=false;}

    if(in.start&&in.select) {
      if(!prevChord_)chordAt_=now;
      if(now-chordAt_>=std::chrono::seconds(2))
        out.emergencyExit=true;
    }

    if(in.start) {
      const auto held=now-startAt_;
      if(held>=std::chrono::seconds(8))out.emergencyExit=true;
      else if(held>=std::chrono::seconds(4) && !startMenuFired_ &&
              !menuVisible && !in.select) {
        out.toggleMenu=true;
        startMenuFired_=true;
      }
    } else {
      startMenuFired_=false;
    }

    // SELECT-only long press is distinct from normal SELECT tap (game map).
    // Once START has been added, use the chord exit, never toggle menu.
    if(in.select && !in.start && !selectMenuFired_ &&
       now-selectAt_>=std::chrono::seconds(2)) {
      out.toggleMenu=true;
      selectMenuFired_=true;
    }

    if(menuVisible && !out.toggleMenu && !out.emergencyExit) {
      if(in.a && !prevA_)out.confirmExit=true;
      else if(in.b && !prevB_)out.cancelMenu=true;
    }

    prevStart_=in.start;
    prevSelect_=in.select;
    prevChord_=in.start&&in.select;
    prevA_=in.a;
    prevB_=in.b;
    return out;
  }

 private:
  bool armed_=false;
  bool prevStart_=false,prevSelect_=false,prevChord_=false;
  bool prevA_=false,prevB_=false;
  bool startMenuFired_=false,selectMenuFired_=false;
  Clock::time_point releaseAt_{};
  Clock::time_point startAt_{},selectAt_{},chordAt_{};
};

// Use SDL's verified GameController binding where available. Raw physical
// indices are accepted only if explicitly specified in the environment.
// Exception: as emergency backup on firmware without SDL START binding,
// raw START index 9 is used; its eight-second hold cannot accidentally
// open the menu during ordinary short gameplay keypresses. A joystick
// with no SDL START binding can override that using RE4_SHOWCASE_RAW_START.
class Device {
 public:
  void attach(SDL_GameController* controller) {
    release();
    controller_=controller;
    if(controller_)joystick_=SDL_GameControllerGetJoystick(controller_);
    if(!joystick_ && SDL_NumJoysticks()>0) {
      joystick_=SDL_JoystickOpen(0);
      owned_=joystick_!=nullptr;
    }
    select_=mapped(SDL_CONTROLLER_BUTTON_BACK,"RE4_SHOWCASE_RAW_SELECT",-1);
    start_=mapped(SDL_CONTROLLER_BUTTON_START,"RE4_SHOWCASE_RAW_START",9);
    a_=mapped(SDL_CONTROLLER_BUTTON_A,"RE4_SHOWCASE_RAW_A",-1);
    b_=mapped(SDL_CONTROLLER_BUTTON_B,"RE4_SHOWCASE_RAW_B",-1);
  }

  // Index range for user-provided firmware override is [0, 63], and
  // invalid inputs are ignored. Returns fallback only if unset/invalid.
  static int configured(const char* key,int fallback) {
    const char* v=std::getenv(key);
    if(!v||!*v)return fallback;
    char* end=nullptr;
    long n=std::strtol(v,&end,10);
    return end!=v && end && !*end && n>=0 && n<=63
           ? static_cast<int>(n) : fallback;
  }

  Snapshot poll()const {
    Snapshot s{};
    if(controller_ && SDL_GameControllerGetAttached(controller_)) {
      auto down=[this](SDL_GameControllerButton button) {
        return SDL_GameControllerGetButton(controller_,button)!=0;
      };
      s.select=down(SDL_CONTROLLER_BUTTON_BACK);
      s.start=down(SDL_CONTROLLER_BUTTON_START);
      s.a=down(SDL_CONTROLLER_BUTTON_A);
      s.b=down(SDL_CONTROLLER_BUTTON_B);
    }
    // Raw only at a verified SDL binding or an explicit override.
    s.select |= rawDown(select_);
    s.start |= rawDown(start_);
    s.a |= rawDown(a_);
    s.b |= rawDown(b_);
    return s;
  }

  void release(){
    if(owned_ && joystick_)SDL_JoystickClose(joystick_);
    joystick_=nullptr;
    controller_=nullptr;
    owned_=false;
  }
  ~Device(){release();}

 private:
  bool rawDown(int index)const {
    return joystick_ && SDL_JoystickGetAttached(joystick_) &&
           index>=0 && index<SDL_JoystickNumButtons(joystick_) &&
           SDL_JoystickGetButton(joystick_,index)!=0;
  }
  int mapped(SDL_GameControllerButton key,const char* env,int fallback)const {
    int button=fallback;
    if(controller_&&SDL_GameControllerGetAttached(controller_)){
      SDL_GameControllerButtonBind bind=SDL_GameControllerGetBindForButton(controller_,key);
      if(bind.bindType==SDL_CONTROLLER_BINDTYPE_BUTTON)
        button=bind.value.button;
    }
    return configured(env,button);
  }

  SDL_GameController* controller_=nullptr;
  SDL_Joystick* joystick_=nullptr;
  bool owned_=false;
  int select_=-1,start_=-1,a_=-1,b_=-1;
};
}
#endif
