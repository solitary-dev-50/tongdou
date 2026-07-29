#pragma once

#include "app/App.h"

namespace tongdou {

enum class BootMode {
  Normal,
};

class SystemBootstrap {
 public:
  void boot();
  void run();
  BootMode mode() const;

 private:
  App app_;
  BootMode mode_ = BootMode::Normal;
};

}  // namespace tongdou
