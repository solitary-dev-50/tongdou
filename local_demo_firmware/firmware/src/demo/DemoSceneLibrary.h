#pragma once

#include "demo/DemoSceneTypes.h"

namespace tongdou {

class DemoSceneLibrary {
 public:
  DemoScene scene(DemoSceneId id) const;
  DemoScene scene(DemoSceneId id, uint8_t variant) const;
};

}  // namespace tongdou
