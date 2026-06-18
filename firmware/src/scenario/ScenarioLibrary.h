#pragma once

#include <stddef.h>

#include "scenario/ScenarioRule.h"

namespace tongdou {

class ScenarioLibrary {
 public:
  const ScenarioRule* rules() const;
  size_t size() const;
};

}  // namespace tongdou
