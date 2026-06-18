#pragma once

#include "web/ScenarioConfigStore.h"

namespace tongdou {

class ScenarioConfigApi {
 public:
  explicit ScenarioConfigApi(ScenarioConfigStore& store);

  void begin();
  void update();

 private:
  ScenarioConfigStore& store_;
};

}  // namespace tongdou
