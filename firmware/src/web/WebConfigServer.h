#pragma once

#include "web/ScenarioConfigApi.h"
#include "web/WebApiContract.h"

namespace tongdou {

class WebConfigServer {
 public:
  explicit WebConfigServer(ScenarioConfigApi& scenarioApi);

  void begin();
  void update();
  ScenarioConfigApi& scenarioApi();

 private:
  ScenarioConfigApi& scenarioApi_;
};

}  // namespace tongdou
