#pragma once

#include <stdint.h>

namespace tongdou {

enum class PersonalityStyle : uint8_t {
  Gentle,
  Balanced,
  Dramatic,
};

struct ScenarioContext {
  bool quietMode = false;
  bool lowBattery = false;
  bool userPresent = false;
  bool charging = false;
  uint8_t snoozeCount = 0;
  PersonalityStyle personality = PersonalityStyle::Balanced;
};

}  // namespace tongdou
