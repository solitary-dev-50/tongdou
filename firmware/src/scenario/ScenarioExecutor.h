#pragma once

#include "scenario/ScenarioPlan.h"

namespace tongdou {

class ScenarioExecutor {
 public:
  void begin();
  void execute(const ScenarioPlan& plan);
  void update();

 private:
  ScenarioPlan activePlan_;
};

}  // namespace tongdou
