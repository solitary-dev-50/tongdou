#include "system/SystemBootstrap.h"

namespace tongdou {

void SystemBootstrap::boot() {
  app_.begin();
  mode_ = BootMode::Normal;
}

void SystemBootstrap::run() {
  app_.update();
}

BootMode SystemBootstrap::mode() const {
  return mode_;
}

}  // namespace tongdou
