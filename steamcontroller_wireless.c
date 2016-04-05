#include "steamcontroller.h"
#include "common.h"

/**
 * Get the current connection state of a wireless dongle.
 */
bool SCAPI SteamController_QueryWirelessState(const SteamControllerDevice *pDevice, uint8_t *state) {
  SteamController_HIDFeatureReport featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_DONGLE_GET_WIRELESS_STATE;

  if (!SteamController_HIDGetFeatureReport(pDevice, &featureReport)) {
    fprintf(stderr, "DONGLE_GET_WIRELESS_STATE (get) failed for controller %p\n", pDevice);
    return false;
  }

  *state = featureReport.data[0];
  return true;
}

/**
 * Set wireless dongle to accept wireless controllers.
 * @param pController   Controller object to use.
 * @param enable        If true enable pairing.
 * @param deviceType    Type of device to connect to. Only known value is 0x3c for steam wireless controller.
 *                      If set to 0, defaults to 0x3c.
 * @todo Find out what arg3 does.
 */
bool SCAPI SteamController_EnablePairing(const SteamControllerDevice *pDevice, bool enable, uint8_t deviceType) {
  SteamController_HIDFeatureReport featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_ENABLE_PAIRING;
  featureReport.dataLen     = 2;
  featureReport.data[0]     = enable ? 1 : 0;
  featureReport.data[1]     = enable ? (deviceType ? deviceType : 0x3c) : 0;

  if (!SteamController_HIDSetFeatureReport(pDevice, &featureReport)) {
    fprintf(stderr, "ENABLE_PAIRING failed for controller %p\n", pDevice);
    return false;
  }
  return true;
}

/**
 * Accept pairing connection.
 * A pairing event will be sent when a controller connects.
 */
bool SCAPI SteamController_CommitPairing(const SteamControllerDevice *pDevice, bool connect) {
  SteamController_HIDFeatureReport featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = connect ? 
                              STEAMCONTROLLER_COMMIT_DEVICE :
                              STEAMCONTROLLER_DISCONNECT_DEVICE;

  if (!SteamController_HIDSetFeatureReport(pDevice, &featureReport)) {
    fprintf(stderr, "COMMIT_DEVICE/DISCONNECT_DEVICE failed for controller %p\n", pDevice);
    return false;
  }
  return true;
}
