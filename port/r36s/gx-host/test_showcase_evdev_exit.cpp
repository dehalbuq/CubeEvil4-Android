// Offline synthetic Linux evdev mapping tests; no input device is opened.
// Validates R36S kernel key codes without guessing raw A/B button indices.
#include "r36s_showcase_evdev_exit.h"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace r36s_showcase_exit;
using Exit=EvdevButtons;
#if !defined(__linux__)
#error R36S native evdev tests require Linux
#endif
static void on(unsigned long* bits,int code){
  bits[static_cast<unsigned>(code)/Exit::kBitsPerWord] |=
      1ul << (static_cast<unsigned>(code)%Exit::kBitsPerWord);
}
int main() {
  unsigned long keys[Exit::kWordCount]{};
  auto mapping=Exit::choose(keys);
  assert(mapping.start==-1 && mapping.select==-1);
  // A, B, and the D-pad must NEVER map to exit (previous phantom menu bug).
  for(int code: {BTN_EAST,BTN_SOUTH,BTN_NORTH,BTN_WEST,
                 BTN_DPAD_UP,BTN_DPAD_DOWN,BTN_TL,BTN_TR})on(keys,code);
  mapping=Exit::choose(keys);
  assert(mapping.start==-1 && mapping.select==-1);
  on(keys,BTN_START);on(keys,BTN_SELECT);
  mapping=Exit::choose(keys);
  assert(mapping.start==BTN_START && mapping.select==BTN_SELECT);
  assert(Exit::bit(keys,BTN_START));
  assert(Exit::bit(keys,BTN_SELECT));
  assert(!Exit::bit(keys,BTN_MODE));
  assert(!Exit::bit(keys,-1));
  assert(!Exit::bit(keys,KEY_MAX+1));
  // Pressed state test: holding A/B/L/R must not set start/select.
  unsigned long down[Exit::kWordCount]{};
  on(down,BTN_EAST);on(down,BTN_TL);on(down,BTN_TR);
  assert(!Exit::bit(down,mapping.start) && !Exit::bit(down,mapping.select));
  on(down,BTN_START);
  assert(Exit::bit(down,mapping.start) && !Exit::bit(down,mapping.select));
  std::memset(keys,0,sizeof(keys));
  // An older RK handheld can report OGA special BTN_TRIGGER_HAPPY keys.
  on(keys,BTN_TRIGGER_HAPPY1);on(keys,BTN_TRIGGER_HAPPY6);
  mapping=Exit::choose(keys);
  assert(mapping.start==BTN_TRIGGER_HAPPY6 && mapping.select==BTN_TRIGGER_HAPPY1);
  std::memset(keys,0,sizeof(keys));
  on(keys,BTN_TRIGGER_HAPPY1);on(keys,BTN_TRIGGER_HAPPY2);
  mapping=Exit::choose(keys);
  assert(mapping.start==BTN_TRIGGER_HAPPY2 && mapping.select==BTN_TRIGGER_HAPPY1);
  std::memset(keys,0,sizeof(keys));
  on(keys,BTN_TRIGGER_HAPPY6); // no paired Select => don't guess
  mapping=Exit::choose(keys);
  assert(mapping.start==-1 && mapping.select==-1);
  // A START-only device may still expose the 8s emergency-exit gesture.
  std::memset(keys,0,sizeof(keys));
  on(keys,BTN_START);
  mapping=Exit::choose(keys);
  assert(mapping.start==BTN_START && mapping.select==-1);
  std::cout<<"R36S_EVDEV_EXIT_CODE_TEST_OK\n";
  return 0;
}
