#include <Arduino.h>

#include "system/SystemBootstrap.h"

tongdou::SystemBootstrap bootstrap;

void setup() {
  bootstrap.boot();
}

void loop() {
  bootstrap.run();
}
