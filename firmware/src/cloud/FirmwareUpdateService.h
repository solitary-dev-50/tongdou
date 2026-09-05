#pragma once

#include <Arduino.h>

#include "system/ServerEndpointStore.h"

namespace tongdou {

struct FirmwareUpdateOffer {
  String version;
  String url;
  String sha256;
  uint32_t size = 0;
};

class FirmwareUpdateService {
 public:
  explicit FirmwareUpdateService(ServerEndpointStore& serverEndpointStore);

  void begin();
  void update();
  bool acceptOffer(const FirmwareUpdateOffer& offer);
  bool offerPending() const;
  bool installPendingAndRestart();
  String lastError() const;

 private:
  static constexpr unsigned long kBootConfirmationDelayMs = 10000;

  bool validateOffer(const FirmwareUpdateOffer& offer,
                     FirmwareUpdateOffer& normalizedOffer);
  bool savePendingOffer(const FirmwareUpdateOffer& offer);
  bool storedOfferPending() const;
  void clearStoredOffer();
  void confirmRunningFirmware();

  ServerEndpointStore& serverEndpointStore_;
  FirmwareUpdateOffer pendingOffer_;
  String lastError_;
  unsigned long confirmAtMs_ = 0;
  bool confirmPending_ = false;
  bool offerPending_ = false;
};

}  // namespace tongdou
