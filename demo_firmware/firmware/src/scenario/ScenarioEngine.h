#pragma once

#include "scenario/ScenarioContext.h"
#include "scenario/ScenarioEvent.h"
#include "scenario/ScenarioLibrary.h"
#include "scenario/ScenarioPlan.h"

namespace tongdou {

class ScenarioEngine {
 public:
  explicit ScenarioEngine(const ScenarioLibrary& library);

  ScenarioPlan select(const ScenarioEvent& event, const ScenarioContext& context) const;

 private:
  bool matches(const ScenarioRule& rule, const ScenarioEvent& event,
               const ScenarioContext& context) const;

  const ScenarioLibrary& library_;
};

}  // namespace tongdou
