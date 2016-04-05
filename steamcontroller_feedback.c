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
  featureReport.data[0]     = motor & 0xff;
  featureReport.data[1]     = onTime & 0xff;
  featureReport.data[2]     = (onTime >> 8) & 0xff;
  featureReport.data[3]     = offTime & 0xff;
  featureReport.data[4]     = (offTime >> 8) & 0xff;
  featureReport.data[5]     = count;
  featureReport.data[6]     = count >> 8;
  featureReport.data[7]     = motor >> 8;

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
  featureReport.data[0] = melodyId;
  featureReport.data[1] = melodyId >> 8;
  featureReport.data[2] = melodyId >> 16;
  featureReport.data[3] = melodyId >> 24;

  SteamController_HIDSetFeatureReport(pDevice, &featureReport);
}
