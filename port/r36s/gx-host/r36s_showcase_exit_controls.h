// R36S DarkOSRE showcase: resilient exit input with raw SDL joystick fallback.
//
// The GameCube input uses SDL_GameController, but a handheld's SELECT/START
// may not be mapped to SDL_CONTROLLER_BUTTON_BACK/START. Read raw SDL joystick
// buttons too, and provide an emergency START-only long hold. This module
// does not change normal gameplay controls or any stable executable.
#pragma once
#ifndef R36S_SHOWCASE_EXIT_CONTROLS_H
#define R36S_SHOWCASE_EXIT_CONTROLS_H

#include <SDL2/SDL.h>
#include <chrono>
#include <cstdlib>

namespace r36s_showcase_exit {

using Clock = std::chrono::steady_clock;

struct Snapshot {
  bool select = false;
  bool start = false;
  bool leftShoulder = false;
  bool rightShoulder = false;
  bool a = false;
  bool b = false;
  bool home = false;
};
struct Actions {
  bool toggleMenu = false;
  bool confirmExit = false;
  bool cancelMenu = false;
  bool emergencyExit = false;
};

// Menu: tap SELECT (or HOME), or hold START alone for 2.5 s, or hold
// START+L1+R1 for 1 s. Escape when SELECT isn't mapped at all:
// hold START for 6 seconds to exit without requiring menu/A/B.
// SELECT+START held for 1.5 seconds also quits directly, analogous to
// PortMaster's traditional SELECT+START hold-to-quit.
class Gestures {
 public:
  Actions update(const Snapshot& in,bool menuVisible,Clock::time_point now) {
    Actions result{};
    if ((in.select && !prevSelect_) || (in.home && !prevHome_))
      result.toggleMenu = true;

    // Only confirm with a NEW press on an ALREADY visible menu.
    if (menuVisible) {
      if (in.a && !prevA_) result.confirmExit=true;
      if (in.b && !prevB_) result.cancelMenu=true;
    }

    if (in.start) {
      if (!prevStart_) {
        startAt_=now;
        startMenuOpened_=false;
      }
      const auto held=now-startAt_;
      if (held>=std::chrono::milliseconds(6000))
        result.emergencyExit=true;
      else if (held>=std::chrono::milliseconds(2500) &&
               !startMenuOpened_ && !menuVisible && !in.select &&
               !(in.leftShoulder && in.rightShoulder)) {
        result.toggleMenu=true;
        startMenuOpened_=true;
      }
    } else {
      startMenuOpened_=false;
    }

    if (in.start && in.select) {
      if (!prevSelectStart_) selectStartAt_=now;
      if (now-selectStartAt_>=std::chrono::milliseconds(1500))
        result.emergencyExit=true;
    }

    const bool shoulderChord=in.start && in.leftShoulder && in.rightShoulder;
    if (shoulderChord) {
      if (!prevShoulderChord_) shoulderAt_=now;
      if (!shoulderMenuOpened_ &&
          now-shoulderAt_>=std::chrono::milliseconds(1000)) {
        result.toggleMenu=true;
        shoulderMenuOpened_=true;
      }
    } else {
      shoulderMenuOpened_=false;
    }

    prevSelect_=in.select;
    prevHome_=in.home;
    prevA_=in.a;
    prevB_=in.b;
    prevStart_=in.start;
    prevSelectStart_=in.start&&in.select;
    prevShoulderChord_=shoulderChord;
    return result;
  }

 private:
  bool prevSelect_=false, prevHome_=false;
  bool prevA_=false,prevB_=false,prevStart_=false;
  bool prevSelectStart_=false,prevShoulderChord_=false;
  bool startMenuOpened_=false,shoulderMenuOpened_=false;
  Clock::time_point startAt_{},selectStartAt_{},shoulderAt_{};
};

// SDL device adapter: use controller mapping when available. The matching
// raw joystick buttons are also read, so a missing SDL "BACK" mapping
// cannot prevent the user from leaving the port.
class Device {
 public:
  void attach(SDL_GameController* controller) {
    release();
    controller_=controller;
    if (controller_) joystick_=SDL_GameControllerGetJoystick(controller_);
    if (!joystick_ && SDL_NumJoysticks()>0) {
      joystick_=SDL_JoystickOpen(0);
      owned_=joystick_!=nullptr;
    }
    select_=mapped(SDL_CONTROLLER_BUTTON_BACK,0);
    start_=mapped(SDL_CONTROLLER_BUTTON_START,1);
    // Two common USB/handheld raw layouts: SELECT=8 START=9 and
    // SELECT=6 START=7. Only enable aliases if SDL has no mapping.
    // Explicit RE4_SHOWCASE_RAW_* overrides disable these aliases.
    altSelect_ = unbound(SDL_CONTROLLER_BUTTON_BACK,"RE4_SHOWCASE_RAW_SELECT") ? 6 : -1;
    altStart_ = unbound(SDL_CONTROLLER_BUTTON_START,"RE4_SHOWCASE_RAW_START") ? 7 : -1;
    l1_=mapped(SDL_CONTROLLER_BUTTON_LEFTSHOULDER,2);
    r1_=mapped(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,3);
    a_=mapped(SDL_CONTROLLER_BUTTON_A,4);
    b_=mapped(SDL_CONTROLLER_BUTTON_B,5);
    home_=mapped(SDL_CONTROLLER_BUTTON_GUIDE,6);
  }

  // These are optional for unusual firmware joystick mappings. Sample:
  // export RE4_SHOWCASE_RAW_SELECT=8 RE4_SHOWCASE_RAW_START=9
  // Only valid 0..63 indices accepted; invalid overrides are ignored.
  static int configured(int which,int fallback){
    static const char* keys[]={
      "RE4_SHOWCASE_RAW_SELECT","RE4_SHOWCASE_RAW_START",
      "RE4_SHOWCASE_RAW_L1","RE4_SHOWCASE_RAW_R1",
      "RE4_SHOWCASE_RAW_A","RE4_SHOWCASE_RAW_B",
      "RE4_SHOWCASE_RAW_HOME"
    };
    const char* e=std::getenv(keys[which]);
    if(!e||!*e)return fallback;
    char* end=nullptr;
    const long value=std::strtol(e,&end,10);
    return (end!=e && end && !*end && value>=0 && value<=63) ? int(value) : fallback;
  }

  Snapshot poll()const {
    Snapshot s{};
    if(controller_ && SDL_GameControllerGetAttached(controller_)) {
      auto gc=[this](SDL_GameControllerButton b) {
        return SDL_GameControllerGetButton(controller_,b)!=0;
      };
      s.select=gc(SDL_CONTROLLER_BUTTON_BACK);
      s.start=gc(SDL_CONTROLLER_BUTTON_START);
      s.leftShoulder=gc(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
      s.rightShoulder=gc(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
      s.a=gc(SDL_CONTROLLER_BUTTON_A);
      s.b=gc(SDL_CONTROLLER_BUTTON_B);
      s.home=gc(SDL_CONTROLLER_BUTTON_GUIDE);
    }
    // Independent raw poll even when a controller mapping exists. This is
    // the primary fix on devices whose SELECT does not generate GC "BACK".
    s.select|=read(select_)||read(altSelect_);
    s.start|=read(start_)||read(altStart_);
    s.leftShoulder|=read(l1_);
    s.rightShoulder|=read(r1_);
    s.a|=read(a_);
    s.b|=read(b_);
    s.home|=read(home_);
    return s;
  }

  bool matchesRaw(SDL_JoystickID id,int button,int role)const {
    if(!joystick_ || id!=SDL_JoystickInstanceID(joystick_))return false;
    switch(role){
      case 0:return matches(button,select_)||matches(button,altSelect_);
      case 1:return matches(button,start_)||matches(button,altStart_);
      case 2:return matches(button,a_);
      case 3:return matches(button,b_);
      case 4:return matches(button,home_);
    }
    return false;
  }

  void release(){
    if(owned_ && joystick_)SDL_JoystickClose(joystick_);
    joystick_=nullptr;
    owned_=false;
    controller_=nullptr;
  }
  ~Device(){release();}

 private:
  bool read(int index)const {
    return joystick_ && SDL_JoystickGetAttached(joystick_) &&
           index>=0 && index<SDL_JoystickNumButtons(joystick_) &&
           SDL_JoystickGetButton(joystick_,index)!=0;
  }
  static bool matches(int button,int index){return index>=0&&button==index;}
  int mapped(SDL_GameControllerButton key,int role)const {
    int fallback=-1;
    if(controller_ && SDL_GameControllerGetAttached(controller_)){
      const SDL_GameControllerButtonBind bind=
          SDL_GameControllerGetBindForButton(controller_,key);
      if(bind.bindType==SDL_CONTROLLER_BINDTYPE_BUTTON)
        fallback=bind.value.button;
    }
    if(fallback<0){
      switch(role){
        case 0: fallback=8; break; // common R36S SELECT
        case 1: fallback=9; break; // common R36S START
        case 2: fallback=4; break;
        case 3: fallback=5; break;
        case 4: fallback=0; break;
        case 5: fallback=1; break;
        default: fallback=-1;break;
      }
    }
    return configured(role,fallback);
  }
  bool unbound(SDL_GameControllerButton button,const char* env) const {
    if(std::getenv(env))return false;
    if(!controller_)return true;
    const SDL_GameControllerButtonBind bind=
        SDL_GameControllerGetBindForButton(controller_,button);
    return bind.bindType!=SDL_CONTROLLER_BINDTYPE_BUTTON;
  }

  SDL_GameController* controller_=nullptr;
  SDL_Joystick* joystick_=nullptr;
  bool owned_=false;
  int select_=-1,start_=-1,altSelect_=-1,altStart_=-1;
  int l1_=-1,r1_=-1,a_=-1,b_=-1,home_=-1;
};
} // namespace r36s_showcase_exit
#endif
