#pragma once

#include "web/ScenarioConfigOptions.h"
#include "web/ScenarioConfigStore.h"
#include "web/WebApiResult.h"

namespace tongdou {

class ScenarioConfigApi {
 public:
  explicit ScenarioConfigApi(ScenarioConfigStore& store);

  void begin();
  void update();

  const ScenarioConfig& getConfig() const;
  WebApiResult putConfig(const ScenarioConfigPatch& patch);
  WebApiResult resetConfig();
  WebApiResult importConfig(const ScenarioConfig& config);
  const ScenarioConfig& exportConfig() const;
  const ScenarioConfigOptions& options() const;

 private:
  ScenarioConfigStore& store_;
  ScenarioConfigOptions options_;
};

}  // namespace tongdou
