#include "steamcontroller.h"
#include "common.h"

void Debug_DumpHex(const void *pData, size_t count) {
  const uint8_t *data = (const uint8_t *)pData;
  const uint8_t *line = data;

  for (size_t i=0; i<count; i++) {
    fprintf(stderr, "%02x ", data[i]);
    if ((i % 0x10) == 0xF) {
      fprintf(stderr, " ");
      for (size_t j=0; j<16; j++) {
        if (line[j]>31 && line[j]<0x80)
          fprintf(stderr, "%c", line[j]);
        else
          fprintf(stderr, ".");
      }
      fprintf(stderr, "\n");
      line = data+i+1;
    }
  }
  fprintf(stderr, "\n");
}

bool SteamController_Initialize(const SteamControllerDevice *pDevice) {
  assert(pDevice);

  if (!pDevice)
    return false;

  SteamController_HIDFeatureReport featureReport;

  if (SteamController_IsWirelessDongle(pDevice)) {
    memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId   = 0;

    SteamController_HIDGetFeatureReport(pDevice, &featureReport);

    memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId   = STEAMCONTROLLER_SET_PRNG_ENTROPY;
    featureReport.dataLen     = 16;
    for (int i=0; i<16; i++) featureReport.data[i] = rand(); // FIXME

    if (!SteamController_HIDSetFeatureReport(pDevice, &featureReport)) {
      fprintf(stderr, "SET_PRNG_ENTROPY failed for controller %p\n", pDevice);
      return false;
    }

    memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId   = STEAMCONTROLLER_UNKNOWN_B1;
    featureReport.dataLen     = 2;
    SteamController_HIDSetFeatureReport(pDevice, &featureReport);

    memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId   = STEAMCONTROLLER_DONGLE_GET_WIRELESS_STATE;
    SteamController_HIDGetFeatureReport(pDevice, &featureReport);

    Debug_DumpHex(&featureReport, sizeof(featureReport));
  }

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_GET_ATTRIBUTES;
  featureReport.dataLen     = 4;

  if (!SteamController_HIDGetFeatureReport(pDevice, &featureReport)) {
    fprintf(stderr, "Failed to get GET_ATTRIBUTES response for controller %p\n", pDevice);
    return false;
  } 

  if (featureReport.dataLen < 4) {
    fprintf(stderr, "Bad GET_ATTRIBUTES response for controller %p\n", pDevice);
    // return false;
  }

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_CLEAR_MAPPINGS;

  if (!SteamController_HIDSetFeatureReport(pDevice, &featureReport)) {
    fprintf(stderr, "CLEAR_MAPPINGS failed for controller %p\n", pDevice);
    return false;
  }

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_GET_CHIPID;

  if (!SteamController_HIDGetFeatureReport(pDevice, &featureReport)) {
    fprintf(stderr, "GET_CHIPID for controller %p\n", pDevice);
    return false;
  }

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_DONGLE_GET_VERSION;

  if (!SteamController_HIDGetFeatureReport(pDevice, &featureReport)) {
    fprintf(stderr, "GET_CHIPID for controller %p\n", pDevice);
    return false;
  }

  return true;
}

/** Add a settings parameter and value to an SET_SETTINGS feature report. */
static inline void SteamController_FeatureReportAddSetting(SteamController_HIDFeatureReport *featureReport, uint8_t setting, uint16_t value) {
  featureReport->data[featureReport->dataLen++] = setting;
  featureReport->data[featureReport->dataLen++] = value & 0xff;
  featureReport->data[featureReport->dataLen++] = value >> 8;
}

bool SCAPI SteamController_Configure(const SteamControllerDevice *pDevice, unsigned configFlags) {
  SteamController_HIDFeatureReport featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_SET_SETTINGS;

  SteamController_FeatureReportAddSetting(&featureReport, 0x03, 0x2d); // 0x2d, unknown
  SteamController_FeatureReportAddSetting(&featureReport, 0x05, (configFlags & STEAMCONTROLLER_CONFIG_RIGHT_PAD_HAPTIC_TOUCH)     ? 1 : 0);
  SteamController_FeatureReportAddSetting(&featureReport, 0x07, (configFlags & STEAMCONTROLLER_CONFIG_STICK_HAPTIC)               ? 0 : 7);
  SteamController_FeatureReportAddSetting(&featureReport, 0x08, (configFlags & STEAMCONTROLLER_CONFIG_RIGHT_PAD_HAPTIC_TRACKBALL) ? 0 : 7);
  SteamController_FeatureReportAddSetting(&featureReport, 0x18, 0x00); // 0x00, unknown
  SteamController_FeatureReportAddSetting(&featureReport, 0x2d, 100); // home button brightness 0-100
  SteamController_FeatureReportAddSetting(&featureReport, 0x2e, 0x00); // 0x00, unknown
  SteamController_FeatureReportAddSetting(&featureReport, 0x2f, 0x01); // 0x01, unknown
  SteamController_FeatureReportAddSetting(&featureReport, 0x30, (configFlags & 31));
  SteamController_FeatureReportAddSetting(&featureReport, 0x31, (configFlags & STEAMCONTROLLER_CONFIG_SEND_BATTERY_STATUS)        ? 2 : 0);
  SteamController_FeatureReportAddSetting(&featureReport, 0x32, 300); // 0x012c seconds to controller shutdown

  if (!SteamController_HIDSetFeatureReport(pDevice, &featureReport)) {
    fprintf(stderr, "SET_SETTINGS failed for controller %p\n", pDevice);
    return false;
  }

  return true;
}

/**
 * Set the brightness of the home button.
 * @param brightness The brightness of the home button in percent (0-100).
 */
bool SCAPI SteamController_SetHomeButtonBrightness(const SteamControllerDevice *pDevice, uint8_t brightness) {
  SteamController_HIDFeatureReport featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_SET_SETTINGS;

  SteamController_FeatureReportAddSetting(&featureReport, 0x2d, brightness); 

  if (!SteamController_HIDSetFeatureReport(pDevice, &featureReport)) {
    fprintf(stderr, "SET_SETTINGS failed for controller %p\n", pDevice);
    return false;
  }

  return true;
}

bool SCAPI SteamController_SetTimeOut(const SteamControllerDevice *pDevice, uint16_t timeout) {
  SteamController_HIDFeatureReport featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_SET_SETTINGS;

  SteamController_FeatureReportAddSetting(&featureReport, 0x32, timeout); 

  if (!SteamController_HIDSetFeatureReport(pDevice, &featureReport)) {
    fprintf(stderr, "SET_SETTINGS failed for controller %p\n", pDevice);
    return false;
  }

  return true;
}

/**
 * Turn off the controller.
 */ 
bool SCAPI SteamController_TurnOff(const SteamControllerDevice *pDevice) {
  if (!pDevice)
    return false;

  SteamController_HIDFeatureReport featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_TURN_OFF_CONTROLLER;
  featureReport.dataLen     = 4;
  memcpy(featureReport.data, "off!", 4);

  return SteamController_HIDSetFeatureReport(pDevice, &featureReport);
}

/**
 * Store the startup and shutdown melody in the controller's EEPROM.
 */
void SCAPI SteamController_SaveMelodies(const SteamControllerDevice *pDevice, uint8_t startupMelody, uint8_t shutdownMelody) {
  SteamController_HIDFeatureReport featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_WRITE_EEPROM;
  featureReport.dataLen     = 0x10;
  featureReport.data[0]  = startupMelody;
  featureReport.data[1]  = shutdownMelody;
  featureReport.data[2]  = 0xff;
  featureReport.data[3]  = 0xff;
  featureReport.data[4]  = 0x03; // 0x03
  featureReport.data[5]  = 0x09; // 0x09
  featureReport.data[6]  = 0x05; // 0x05
  featureReport.data[7]  = 0xff;
  featureReport.data[8]  = 0xff;
  featureReport.data[9]  = 0xff;
  featureReport.data[10] = 0xff;
  featureReport.data[11] = 0xff;
  featureReport.data[12] = 0xff;
  featureReport.data[13] = 0xff;
  featureReport.data[14] = 0xff;
  featureReport.data[15] = 0xff;
  SteamController_HIDSetFeatureReport(pDevice, &featureReport);
}
