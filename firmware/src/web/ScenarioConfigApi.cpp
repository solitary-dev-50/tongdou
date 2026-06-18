#include "web/ScenarioConfigApi.h"

namespace tongdou {

ScenarioConfigApi::ScenarioConfigApi(ScenarioConfigStore& store) : store_(store) {}

void ScenarioConfigApi::begin() {
  (void)store_;
}

void ScenarioConfigApi::update() {}

}  // namespace tongdou
