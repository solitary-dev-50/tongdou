#include <Arduino.h>

#include "app/App.h"

tongdou::App app;

void setup() {
  app.begin();
}

void loop() {
  app.update();
}
