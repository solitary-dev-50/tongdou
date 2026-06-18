#include "web/ScenarioConfigStore.h"

#include <string.h>

namespace tongdou {
namespace {

void copyText(char* target, size_t targetLength, const char* value) {
  if (targetLength == 0) {
    return;
  }
  strncpy(target, value, targetLength - 1);
  target[targetLength - 1] = '\0';
}

}  // namespace

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
  config_.pack.formatVersion = kScenarioPackFormatVersion;
  config_.pack.defaultPersonality = PersonalityStyle::Balanced;
  copyText(config_.pack.id, sizeof(config_.pack.id), "tongdou.default.v1");
  copyText(config_.pack.name, sizeof(config_.pack.name), "Default Tong Dou");
  copyText(config_.pack.description, sizeof(config_.pack.description),
           "Built-in cheeky desk companion scenario pack.");
}

}  // namespace tongdou
