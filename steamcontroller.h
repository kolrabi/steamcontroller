#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * A single controller device. Could be a wired controller or 
 * a wireless dongle. 
 * @todo: Figure out multiple controllers on single dongle.
 */
typedef struct {
  int       deviceId;               /**< HID device id. */
  int       fd;                     /**< Open file descriptor of device. */
  unsigned  configFlags;            /**< Initial configuration. */
  uint16_t  timeoutSeconds;         /**< Time without use before controller shuts down. */
  uint8_t   brightness;             /**< Home button brightness. */
  bool      isWireless;             /**< If true, the device is a wireless dongle. */
} steam_controller_t;

/**
 * A coordinate pair, representing a horizontal and vertical axis.
 */
typedef struct { int16_t x, y; }    steam_controller_axis_pair_t;

/**
 * 3 coordinates representing a vector in three dimensional space relative to the controller.
 */
typedef struct { int16_t x, y, z; } steam_controller_vector_t;

#define STEAM_CONTROLLER_BUTTON_RT          (1<<0)        /**< Right trigger fully pressed. */
#define STEAM_CONTROLLER_BUTTON_LT          (1<<1)        /**< Left trigger fully pressed. */
#define STEAM_CONTROLLER_BUTTON_RS          (1<<2)        /**< Right shoulder button pressed. */
#define STEAM_CONTROLLER_BUTTON_LS          (1<<3)        /**< Left shoulder button pressed. */
#define STEAM_CONTROLLER_BUTTON_Y           (1<<4)        /**< Y button. */
#define STEAM_CONTROLLER_BUTTON_B           (1<<5)        /**< B button. */
#define STEAM_CONTROLLER_BUTTON_X           (1<<6)        /**< X button. */
#define STEAM_CONTROLLER_BUTTON_A           (1<<7)        /**< A button. */

#define STEAM_CONTROLLER_BUTTON_DPAD_UP     (0x01 << 8)   /**< Left pad pressed with thumb in the upper quarter. */
#define STEAM_CONTROLLER_BUTTON_DPAD_RIGHT  (0x02 << 8)   /**< Left pad pressed with thumb in the right quarter. */
#define STEAM_CONTROLLER_BUTTON_DPAD_LEFT   (0x04 << 8)   /**< Left pad pressed with thumb in the left quarter. */
#define STEAM_CONTROLLER_BUTTON_DPAD_DOWN   (0x08 << 8)   /**< Left pad pressed with thumb in the bottom quarter. */
#define STEAM_CONTROLLER_BUTTON_PREV        (0x10 << 8)   /**< Left arrow button. */
#define STEAM_CONTROLLER_BUTTON_HOME        (0x20 << 8)   /**< Steam logo button. */
#define STEAM_CONTROLLER_BUTTON_NEXT        (0x40 << 8)   /**< Right arrow button. */
#define STEAM_CONTROLLER_BUTTON_LG          (0x80 << 8)   /**< Left grip button. */

#define STEAM_CONTROLLER_BUTTON_RG          (0x01 << 16)  /**< Right grip button. */
#define STEAM_CONTROLLER_BUTTON_STICK       (0x02 << 16)  /**< Stick is pressed down. */
#define STEAM_CONTROLLER_BUTTON_RPAD        (0x04 << 16)  /**< Right pad pressed. */
#define STEAM_CONTROLLER_BUTTON_LFINGER     (0x08 << 16)  /**< If set, a finger is touching left touch pad. */
#define STEAM_CONTROLLER_BUTTON_RFINGER     (0x10 << 16)  /**< If set, a finger is touching right touch pad. */

#define STEAM_CONTROLLER_FLAG_PAD_STICK     (0x80 << 16)  /**< If set, STEAM_CONTROLLER_BUTTON_LFINGER to determines whether leftXY is 
                                                               pad position or stick position. */

/**
 * Current state of a controller including buttons and axes.
 */
typedef struct {
  uint32_t                      timeStamp;      /**< Timestamp of latest update. */

  uint32_t                      activeButtons;  /**< Actively pressed buttons and flags. */

  uint8_t                       leftTrigger;    /**< Left analog trigger value. */
  uint8_t                       rightTrigger;   /**< Right analog trigger value. */

  steam_controller_axis_pair_t  rightPad;       /**< Position of thumb on the right touch pad, or 0,0 if untouched. */
  steam_controller_axis_pair_t  leftPad;        /**< Position of thumb on the left touch pad, or 0,0 if untouched. */

  steam_controller_axis_pair_t  stick;          /**< Stick position. */

  /** 
   * Contains some kind of orientation vector? 
   * When rotating the controller around one axis 360 degrees, the value for that
   * axis becomes negative. One further rotation and it becomes positive againg.
   * This is probably the imaginary parts of a unit quaternion representing the
   * controller orientation in space.
   * @todo Figure this out.
   */
  steam_controller_vector_t     orientation;

  /**
   * Current acceleration of the controller.
   * Positive X is toward the right, positive Y is toward the front (where the usb port is),
   * positive Z is up (the direction the stick is pointing). 16384 seems to be around 1g.
   */
  steam_controller_vector_t     acceleration;

  /**
   * Data from the gyroscopes. Axes seem to be the same as for acceleration. TODO: figure out scale and units
   */
  steam_controller_vector_t     angularVelocity;

  /**
   * Voltage of the battery (both cells) in millivolt.
   */ 
  uint16_t                      batteryVoltage;
} steam_controller_state_t;

// ----------------------------------------------------------------------------------------------
// Events

#define STEAM_CONTROLLER_EVENT_UPDATE       (1)
typedef struct {
  uint32_t                      timeStamp;
  uint32_t                      buttons;

  uint8_t                       leftTrigger;
  uint8_t                       rightTrigger;

  steam_controller_axis_pair_t  leftXY;
  steam_controller_axis_pair_t  rightXY;

  steam_controller_vector_t     orientation;
  steam_controller_vector_t     acceleration;
  steam_controller_vector_t     angularVelocity;
} steam_controller_update_event_t;

#define STEAM_CONTROLLER_EVENT_CONNECTION   (3)
#define STEAM_CONTROLLER_CONNECTION_EVENT_DISCONNECTED        1
#define STEAM_CONTROLLER_CONNECTION_EVENT_CONNECTED           2
#define STEAM_CONTROLLER_CONNECTION_EVENT_PAIRING_REQUESTED   3
typedef struct {
  uint8_t details;
} steam_controller_connection_event_t;

#define STEAM_CONTROLLER_EVENT_BATTERY      (4)
typedef struct {
  uint16_t                      voltage;    /**< Battery voltage in millivolts */
} steam_controller_battery_event_t;

typedef union {
  steam_controller_update_event_t     update;
  steam_controller_battery_event_t    battery;
  steam_controller_connection_event_t connection;
} steam_controller_event_t;

// ----------------------------------------------------------------------------------------------
// Controller device enumeration

size_t    SteamController_EnumControllers(int *pDeviceIds, size_t nMaxDevices);

// ----------------------------------------------------------------------------------------------
// Controller initialization

bool      SteamController_Reset(int nDeviceId);
bool      SteamController_Open(steam_controller_t *pController, int nDeviceId);
void      SteamController_Close(steam_controller_t *pController);
bool      SteamController_TurnOff(const steam_controller_t *pController);

// ----------------------------------------------------------------------------------------------
// Wireless dongle control

#define   STEAM_CONTROLLER_WIRELESS_STATE_NOT_CONNECTED 1   /**< No controller is paired and connected with the dongle. */
#define   STEAM_CONTROLLER_WIRELESS_STATE_CONNECTED     2   /**< (At least?) one controller is connected with the dongle. */

bool      SteamController_QueryWirelessState(const steam_controller_t *pController, uint8_t *state);

bool      SteamController_EnablePairing(const steam_controller_t *pController, bool enable, uint8_t deviceType);
bool      SteamController_CommitPairing(const steam_controller_t *pController, bool connect);

uint32_t  SteamController_GetDongleVersion(const steam_controller_t *pController);

// ----------------------------------------------------------------------------------------------
// Controller configuration

#define   STEAM_CONTROLLER_CONFIG_FLAG_HORIZONTAL_ACCELERATION_AS_STICK   1     /**< Replaces left horizontal axes with X acceleration. */
#define   STEAM_CONTROLLER_CONFIG_FLAG_VERTICAL_ACCELERATION_AS_STICK     2     /**< Replaces left vertical axes with Y acceleration. */
#define   STEAM_CONTROLLER_CONFIG_FLAG_SEND_ORIENTATION                   4     /**< Sends the controller orientation. */
#define   STEAM_CONTROLLER_CONFIG_FLAG_SEND_ACCELERATION                  8     /**< Sends the controller acceleration vector. */
#define   STEAM_CONTROLLER_CONFIG_FLAG_SEND_GYRO                          16    /**< Sends the angular velocity of the controller. */
#define   STEAM_CONTROLLER_CONFIG_FLAG_SEND_BATTERY_STATUS                32    /**< Sends battery status updates. */
#define   STEAM_CONTROLLER_CONFIG_FLAG_RIGHT_PAD_HAPTIC_TOUCH             64    /**< Makes right pad tick on touch. */
#define   STEAM_CONTROLLER_CONFIG_FLAG_RIGHT_PAD_HAPTIC_TRACKBALL         128   /**< Makes right pad tick like in trackball mode. */
#define   STEAM_CONTROLLER_CONFIG_FLAG_STICK_HAPTIC                       256   /**< Makes stick tick on movement. */

#define   STEAM_CONTROLLER_CONFIG_DEFAULT_FLAGS                           0     /**< Only axes and button states in updates, no automatic haptic feedback. */
#define   STEAM_CONTROLLER_CONFIG_DEFAULT_TIMEOUT                         300   /**< Turn of after 5 minutes of inactivity. */
#define   STEAM_CONTROLLER_CONFIG_DEFAULT_BRIGHTNESS                      100   /**< Full home button brightness. */

bool      SteamController_Configure(steam_controller_t *pController, unsigned configFlags, uint16_t timeoutSeconds, uint8_t brightness);
bool      SteamController_Reconfigure(steam_controller_t *pController, unsigned configFlags, uint16_t timeoutSeconds, uint16_t brightness);
bool      SteamController_ReconfigurePrevious(steam_controller_t *pController);
bool      SteamController_SetHomeButtonBrightness(steam_controller_t *pController, uint8_t brightness);

// ----------------------------------------------------------------------------------------------
// Operations

uint8_t SteamController_ReadEvent(const steam_controller_t *pController, steam_controller_event_t *pEvent);
void    SteamController_UpdateState(steam_controller_state_t *pState, const steam_controller_update_event_t *pEvent);
void    SteamController_PlayMelody(const steam_controller_t *pController, uint32_t melody);

// ----------------------------------------------------------------------------------------------
// Haptic feedback

bool    SteamController_TriggerHaptic(const steam_controller_t *pController, uint16_t motor, uint16_t onTime, uint16_t offTime, uint16_t count);

// ----------------------------------------------------------------------------------------------
// Firmware boot loader

// TODO:    These are useless without function to upload firmware
// WARNING: Until this is figured out it is not safe to use these functions and if you set your
//          wireless dongle into bootloader mode you have to start steam to reset it. 
// bool    SteamController_ResetForBootloader(const steam_controller_t *pController);
// size_t  SteamController_EnumBootLoaders(int *pDeviceIds, size_t nMaxDevices);
