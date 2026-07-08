#pragma once

#include "demo/DemoSceneTypes.h"

namespace tongdou {

class DemoSceneLibrary {
 public:
  DemoScene scene(DemoSceneId id) const;
};

}  // namespace tongdou
