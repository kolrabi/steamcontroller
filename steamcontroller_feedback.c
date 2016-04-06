#include "steamcontroller.h"
#include "common.h"

/**
 * Pulse the haptic feedback of the controller using pulse width modulation.
 * 
 * @param pDevice     Controller object to use.
 * @param motor       Actuator to use, 0 == right, 1 == left.
 * @param onTime      PWM on time in microseconds.
 * @param offTime     PWM off time in microseconds.
 * @param count       PWM cycle count.
 */
bool SCAPI SteamController_TriggerHaptic(const SteamControllerDevice *pDevice, uint16_t motor, uint16_t onTime, uint16_t offTime, uint16_t count) {
  SteamController_HIDFeatureReport featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_TRIGGER_HAPTIC_PULSE;
  featureReport.dataLen     = motor > 0xff ? 8 : 7;
  featureReport.data[0]     = LowByte(motor);
  StoreU16(featureReport.data + 1, onTime);
  StoreU16(featureReport.data + 3, offTime);
  StoreU16(featureReport.data + 5, count);

  if (featureReport.dataLen > 7)
    featureReport.data[7]     = HighByte(motor);

  return SteamController_HIDSetFeatureReport(pDevice, &featureReport);
}

/**
 * Play one of the predefined melodies.
 */
void SCAPI SteamController_PlayMelody(const SteamControllerDevice *pDevice, uint32_t melodyId) {
  SteamController_HIDFeatureReport featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAMCONTROLLER_PLAY_MELODY;
  featureReport.dataLen     = 0x04;
  StoreU32(featureReport.data, melodyId);

  SteamController_HIDSetFeatureReport(pDevice, &featureReport);
}
