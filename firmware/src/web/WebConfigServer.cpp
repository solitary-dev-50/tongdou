#include "web/WebConfigServer.h"

namespace tongdou {

WebConfigServer::WebConfigServer(ScenarioConfigApi& scenarioApi)
    : scenarioApi_(scenarioApi) {}

void WebConfigServer::begin() {
  scenarioApi_.begin();
}

void WebConfigServer::update() {
  scenarioApi_.update();
}

}  // namespace tongdou
