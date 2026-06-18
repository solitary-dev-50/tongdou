#pragma once

#include "web/ScenarioConfigModel.h"

namespace tongdou {

class ScenarioConfigStore {
 public:
  void begin();
  const ScenarioConfig& current() const;
  bool save(const ScenarioConfig& config);
  void resetToDefaults();

 private:
  ScenarioConfig config_;
};

}  // namespace tongdou
