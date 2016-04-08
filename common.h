#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if _MSC_VER
#define inline __inline
#endif

void Debug_DumpHex(const void *pData, size_t count);

#define USB_VID_VALVE                               0x28de
#define USB_PID_STEAMCONTROLLER_WIRED               0x1102
#define USB_PID_STEAMCONTROLLER_WIRELESS            0x1142
#define USB_PID_STEAMCONTROLLER_DONGLE_BOOTLOADER   0x1042

#define STEAMCONTROLLER_CLEAR_MAPPINGS             0x81 // 1000 0001
#define STEAMCONTROLLER_GET_ATTRIBUTES             0x83 // 1000 0011
#define STEAMCONTROLLER_UNKNOWN_85                 0x85 // 1000 0101
#define STEAMCONTROLLER_SET_SETTINGS               0x87 // 1000 0111 // seems to reset config
#define STEAMCONTROLLER_UNKNOWN_8E                 0x8E // 1000 1110
#define STEAMCONTROLLER_TRIGGER_HAPTIC_PULSE       0x8F // 1000 1111
#define STEAMCONTROLLER_START_BOOTLOADER           0x90 // 1001 0000
#define STEAMCONTROLLER_SET_PRNG_ENTROPY           0x96 // 1001 0110
#define STEAMCONTROLLER_TURN_OFF_CONTROLLER        0x9F // 1001 1111
#define STEAMCONTROLLER_DONGLE_GET_VERSION         0xA1 // 1010 0001
#define STEAMCONTROLLER_ENABLE_PAIRING             0xAD // 1010 1101
#define STEAMCONTROLLER_GET_STRING_ATTRIBUTE       0xAE // 1010 1110
#define STEAMCONTROLLER_UNKNOWN_B1                 0xB1 // 1011 0001
#define STEAMCONTROLLER_DISCONNECT_DEVICE          0xB2 // 1011 0010
#define STEAMCONTROLLER_COMMIT_DEVICE              0xB3 // 1011 0011
#define STEAMCONTROLLER_DONGLE_GET_WIRELESS_STATE  0xB4 // 1011 0100
#define STEAMCONTROLLER_PLAY_MELODY                0xB6 // 1011 0110
#define STEAMCONTROLLER_GET_CHIPID                 0xBA // 1011 1010
#define STEAMCONTROLLER_WRITE_EEPROM               0xC1 // 1100 0001

typedef struct {
  uint8_t reportPage;
  uint8_t featureId;
  uint8_t dataLen;
  uint8_t data[62];
} SteamController_HIDFeatureReport;

bool SteamController_HIDSetFeatureReport(const SteamControllerDevice *pDevice, SteamController_HIDFeatureReport *pReport);
bool SteamController_HIDGetFeatureReport(const SteamControllerDevice *pDevice, SteamController_HIDFeatureReport *pReport);

bool    SteamController_Initialize(const SteamControllerDevice *pDevice);
uint8_t SteamController_ReadRaw(const SteamControllerDevice *pDevice, uint8_t *buffer, uint8_t maxLen);


static inline uint8_t LowByte(uint16_t value)   { return value & 0xff; }
static inline uint8_t HighByte(uint16_t value)  { return (value >> 8) & 0xff; }

static inline void StoreU16(uint8_t *pDestination, uint16_t value) {
  pDestination[0] = (value >> 0) & 0xff;
  pDestination[2] = (value >> 8) & 0xff;
}

static inline void StoreU32(uint8_t *pDestination, uint32_t value) {
  pDestination[0] = (value >>  0) & 0xff;
  pDestination[1] = (value >>  8) & 0xff;
  pDestination[2] = (value >> 16) & 0xff;
  pDestination[3] = (value >> 24) & 0xff;
}
