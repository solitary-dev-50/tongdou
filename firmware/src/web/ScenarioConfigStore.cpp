#include "web/ScenarioConfigStore.h"

namespace tongdou {

void ScenarioConfigStore::begin() {
  resetToDefaults();
}

const ScenarioConfig& ScenarioConfigStore::current() const {
  return config_;
}

bool ScenarioConfigStore::save(const ScenarioConfig& config) {
  config_ = config;
  return true;
}

void ScenarioConfigStore::resetToDefaults() {
  config_ = {};
}

}  // namespace tongdou
