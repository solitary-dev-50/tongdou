#include "web/ScenarioConfigApi.h"

namespace tongdou {

ScenarioConfigApi::ScenarioConfigApi(ScenarioConfigStore& store) : store_(store) {}

void ScenarioConfigApi::begin() {
  (void)store_;
}

void ScenarioConfigApi::update() {}

const ScenarioConfig& ScenarioConfigApi::getConfig() const {
  return store_.current();
}

WebApiResult ScenarioConfigApi::putConfig(const ScenarioConfigPatch& patch) {
  return {store_.save(patch.config) ? WebApiStatus::Ok : WebApiStatus::StorageError};
}

WebApiResult ScenarioConfigApi::resetConfig() {
  store_.resetToDefaults();
  return {};
}

WebApiResult ScenarioConfigApi::importConfig(const ScenarioConfig& config) {
  return {store_.save(config) ? WebApiStatus::Ok : WebApiStatus::StorageError};
}

const ScenarioConfig& ScenarioConfigApi::exportConfig() const {
  return store_.current();
}

const ScenarioConfigOptions& ScenarioConfigApi::options() const {
  return options_;
}

}  // namespace tongdou
