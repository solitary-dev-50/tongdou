#pragma once

#include "web/ScenarioConfigApi.h"

namespace tongdou {

class WebConfigServer {
 public:
  explicit WebConfigServer(ScenarioConfigApi& scenarioApi);

  void begin();
  void update();

 private:
  ScenarioConfigApi& scenarioApi_;
};

}  // namespace tongdou
