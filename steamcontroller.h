#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#if _WIN32
#ifdef STEAMCONTROLLER_BUILDING_LIBRARY
  #define SCAPI __declspec(dllexport)
#else
  #define SCAPI __declspec(dllimport)
#endif
#else
  #define SCAPI 
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SteamControllerDevice      SteamControllerDevice;
typedef struct SteamControllerDeviceEnum  SteamControllerDeviceEnum;

/** A coordinate pair, representing a horizontal and vertical axis. */
typedef struct { int16_t x, y; }          SteamControllerAxisPair;

/** 3 coordinates representing a vector in three dimensional space relative to the controller. */
typedef struct { int16_t x, y, z; }       SteamControllerVector;

#define STEAMCONTROLLER_BUTTON_RT          (1<<0)        /**< Right trigger fully pressed. */
#define STEAMCONTROLLER_BUTTON_LT          (1<<1)        /**< Left trigger fully pressed. */
#define STEAMCONTROLLER_BUTTON_RS          (1<<2)        /**< Right shoulder button pressed. */
#define STEAMCONTROLLER_BUTTON_LS          (1<<3)        /**< Left shoulder button pressed. */
#define STEAMCONTROLLER_BUTTON_Y           (1<<4)        /**< Y button. */
#define STEAMCONTROLLER_BUTTON_B           (1<<5)        /**< B button. */
#define STEAMCONTROLLER_BUTTON_X           (1<<6)        /**< X button. */
#define STEAMCONTROLLER_BUTTON_A           (1<<7)        /**< A button. */

#define STEAMCONTROLLER_BUTTON_DPAD_UP     (0x01 << 8)   /**< Left pad pressed with thumb in the upper quarter. */
#define STEAMCONTROLLER_BUTTON_DPAD_RIGHT  (0x02 << 8)   /**< Left pad pressed with thumb in the right quarter. */
#define STEAMCONTROLLER_BUTTON_DPAD_LEFT   (0x04 << 8)   /**< Left pad pressed with thumb in the left quarter. */
#define STEAMCONTROLLER_BUTTON_DPAD_DOWN   (0x08 << 8)   /**< Left pad pressed with thumb in the bottom quarter. */
#define STEAMCONTROLLER_BUTTON_PREV        (0x10 << 8)   /**< Left arrow button. */
#define STEAMCONTROLLER_BUTTON_HOME        (0x20 << 8)   /**< Steam logo button. */
#define STEAMCONTROLLER_BUTTON_NEXT        (0x40 << 8)   /**< Right arrow button. */
#define STEAMCONTROLLER_BUTTON_LG          (0x80 << 8)   /**< Left grip button. */

#define STEAMCONTROLLER_BUTTON_RG          (0x01 << 16)  /**< Right grip button. */
#define STEAMCONTROLLER_BUTTON_STICK       (0x02 << 16)  /**< Stick is pressed down. */
#define STEAMCONTROLLER_BUTTON_RPAD        (0x04 << 16)  /**< Right pad pressed. */
#define STEAMCONTROLLER_BUTTON_LFINGER     (0x08 << 16)  /**< If set, a finger is touching left touch pad. */
#define STEAMCONTROLLER_BUTTON_RFINGER     (0x10 << 16)  /**< If set, a finger is touching right touch pad. */

#define STEAMCONTROLLER_FLAG_PAD_STICK     (0x80 << 16)  /**< If set, STEAM_CONTROLLER_BUTTON_LFINGER to determines whether leftXY is 
                                                               pad position or stick position. */

/**
 * Current state of a controller including buttons and axes.
 */
typedef struct {
  uint32_t                  timeStamp;      /**< Timestamp of latest update. */

  uint32_t                  activeButtons;  /**< Actively pressed buttons and flags. */

  uint8_t                   leftTrigger;    /**< Left analog trigger value. */
  uint8_t                   rightTrigger;   /**< Right analog trigger value. */

  SteamControllerAxisPair   rightPad;       /**< Position of thumb on the right touch pad, or 0,0 if untouched. */
  SteamControllerAxisPair   leftPad;        /**< Position of thumb on the left touch pad, or 0,0 if untouched. */

  SteamControllerAxisPair   stick;          /**< Stick position. */

  /** 
   * Contains some kind of orientation vector? 
   * When rotating the controller around one axis 360 degrees, the value for that
   * axis becomes negative. One further rotation and it becomes positive againg.
   * This is probably the imaginary parts of a unit quaternion representing the
   * controller orientation in space.
   * @todo Figure this out.
   */
  SteamControllerVector     orientation;

  /**
   * Current acceleration of the controller.
   * Positive X is toward the right, positive Y is toward the front (where the usb port is),
   * positive Z is up (the direction the stick is pointing). 16384 seems to be around 1g.
   */
  SteamControllerVector     acceleration;

  /**
   * Data from the gyroscopes. Axes seem to be the same as for acceleration. TODO: figure out scale and units
   */
  SteamControllerVector     angularVelocity;

  /**
   * Voltage of the battery (both cells) in millivolt.
   */ 
  uint16_t                  batteryVoltage;
  bool                      isConnected;
  bool                      hasPairingRequest;
} SteamControllerState;

// ----------------------------------------------------------------------------------------------
// Events

#define STEAMCONTROLLER_EVENT_UPDATE       (1)
typedef struct {
  uint8_t                   eventType;

  uint32_t                  timeStamp;
  uint32_t                  buttons;

  uint8_t                   leftTrigger;
  uint8_t                   rightTrigger;

  SteamControllerAxisPair   leftXY;
  SteamControllerAxisPair   rightXY;

  SteamControllerVector     orientation;
  SteamControllerVector     acceleration;
  SteamControllerVector     angularVelocity;
} SteamControllerUpdateEvent;

#define STEAMCONTROLLER_EVENT_CONNECTION   (3)
#define STEAMCONTROLLER_CONNECTION_EVENT_DISCONNECTED        1
#define STEAMCONTROLLER_CONNECTION_EVENT_CONNECTED           2
#define STEAMCONTROLLER_CONNECTION_EVENT_PAIRING_REQUESTED   3
typedef struct {
  uint8_t                   eventType;
  uint8_t                   details;
} SteamControllerConnectionEvent;

#define STEAMCONTROLLER_EVENT_BATTERY      (4)
typedef struct {
  uint8_t                   eventType;
  uint16_t                  voltage;    /**< Battery voltage in millivolts */
} SteamControllerBatteryEvent;

typedef union {
  uint8_t                         eventType;
  SteamControllerUpdateEvent      update;
  SteamControllerBatteryEvent     battery;
  SteamControllerConnectionEvent  connection;
} SteamControllerEvent;

// ----------------------------------------------------------------------------------------------
// Controller device enumeration

SCAPI SteamControllerDeviceEnum * SteamController_EnumControllerDevices();
SCAPI SteamControllerDeviceEnum * SteamController_NextControllerDevice(SteamControllerDeviceEnum *pCurrent);

// ----------------------------------------------------------------------------------------------
// Controller initialization

SCAPI SteamControllerDevice * SteamController_Open(const SteamControllerDeviceEnum *pEnum);
SCAPI void                    SteamController_Close(SteamControllerDevice *pDevice);
SCAPI bool                    SteamController_IsWirelessDongle(const SteamControllerDevice *pDevice);
SCAPI bool                    SteamController_TurnOff(const SteamControllerDevice *pDevice);

// ----------------------------------------------------------------------------------------------
// Wireless dongle control

#define   STEAMCONTROLLER_WIRELESS_STATE_NOT_CONNECTED 1   /**< No controller is paired and connected with the dongle. */
#define   STEAMCONTROLLER_WIRELESS_STATE_CONNECTED     2   /**< A controller is connected to the dongle. */

bool      SCAPI SteamController_QueryWirelessState(const SteamControllerDevice *pDevice, uint8_t *state);

bool      SCAPI SteamController_EnablePairing(const SteamControllerDevice *pDevice, bool enable, uint8_t deviceType);
bool      SCAPI SteamController_CommitPairing(const SteamControllerDevice *pDevice, bool connect);

// ----------------------------------------------------------------------------------------------
// Controller configuration

#define   STEAMCONTROLLER_CONFIG_HORIZONTAL_ACCELERATION_AS_STICK   1     /**< Replaces left horizontal axes with X acceleration. */
#define   STEAMCONTROLLER_CONFIG_VERTICAL_ACCELERATION_AS_STICK     2     /**< Replaces left vertical axes with Y acceleration. */
#define   STEAMCONTROLLER_CONFIG_SEND_ORIENTATION                   4     /**< Sends the controller orientation. */
#define   STEAMCONTROLLER_CONFIG_SEND_ACCELERATION                  8     /**< Sends the controller acceleration vector. */
#define   STEAMCONTROLLER_CONFIG_SEND_GYRO                          16    /**< Sends the angular velocity of the controller. */
#define   STEAMCONTROLLER_CONFIG_SEND_BATTERY_STATUS                32    /**< Sends battery status updates. */
#define   STEAMCONTROLLER_CONFIG_RIGHT_PAD_HAPTIC_TOUCH             64    /**< Makes right pad tick on touch. */
#define   STEAMCONTROLLER_CONFIG_RIGHT_PAD_HAPTIC_TRACKBALL         128   /**< Makes right pad tick like in trackball mode. */
#define   STEAMCONTROLLER_CONFIG_STICK_HAPTIC                       256   /**< Makes stick tick on movement. */

#define   STEAMCONTROLLER_DEFAULT_FLAGS                           0     /**< Only axes and button states in updates, no automatic haptic feedback. */
#define   STEAMCONTROLLER_DEFAULT_TIMEOUT                         300   /**< Turn of after 5 minutes of inactivity. */
#define   STEAMCONTROLLER_DEFAULT_BRIGHTNESS                      100   /**< Full home button brightness. */

bool      SCAPI SteamController_Configure(const SteamControllerDevice *pDevice, unsigned configFlags);
bool      SCAPI SteamController_SetHomeButtonBrightness(const SteamControllerDevice *pDevice, uint8_t brightness);
bool      SCAPI SteamController_SetTimeOut(const SteamControllerDevice *pDevice, uint16_t timeout);

// ----------------------------------------------------------------------------------------------
// State

uint8_t   SCAPI SteamController_ReadEvent(const SteamControllerDevice *pDevice, SteamControllerEvent *pEvent);
void      SCAPI SteamController_UpdateState(SteamControllerState *pState, const SteamControllerEvent *pEvent);

// ----------------------------------------------------------------------------------------------
// Feedback

bool      SCAPI SteamController_TriggerHaptic(const SteamControllerDevice *pDevice, uint16_t motor, uint16_t onTime, uint16_t offTime, uint16_t count);
void      SCAPI SteamController_PlayMelody(const SteamControllerDevice *pDevice, uint32_t melody);

#ifdef __cplusplus
} // extern "C"
#endif
