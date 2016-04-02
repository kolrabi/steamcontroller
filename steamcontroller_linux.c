#if __linux__

#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <linux/hiddev.h>
#include <linux/usbdevice_fs.h>

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <glob.h>

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define USB_VID_VALVE                               0x28de
#define USB_PID_STEAMCONTROLLER_WIRED               0x1102
#define USB_PID_STEAMCONTROLLER_WIRELESS            0x1142
#define USB_PID_STEAMCONTROLLER_DONGLE_BOOTLOADER   0x1042

#define GLOB_PATTERN_STEAMCONTROLLER_ALL_DEVICES_WIRED              "/sys/bus/hid/devices/????:28DE:1102.*/hidraw/hidraw*"
#define GLOB_PATTERN_STEAMCONTROLLER_ALL_DEVICES_WIRELESS           "/sys/bus/hid/devices/????:28DE:1142.*/hidraw/hidraw*"
#define GLOB_PATTERN_STEAMCONTROLLER_ALL_DEVICES_DONGLE_BOOTLOADER  "/sys/bus/hid/devices/????:28DE:1042.*/hidraw/hidraw*"
#define GLOB_PATTERN_STEAMCONTROLLER_DEVICE_WIRED                   "/sys/bus/hid/devices/????:28DE:1102.*/hidraw/hidraw%d"

#define STEAM_CONTROLLER_REQUEST_CLEAR_MAPPINGS             0x81
#define STEAM_CONTROLLER_REQUEST_GET_ATTRIBUTES             0x83
#define STEAM_CONTROLLER_REQUEST_UNKNOWN_85                 0x85 // ???
#define STEAM_CONTROLLER_REQUEST_SET_SETTINGS               0x87
#define STEAM_CONTROLLER_REQUEST_TRIGGER_HAPTIC_PULSE       0x8F
#define STEAM_CONTROLLER_REQUEST_START_BOOTLOADER           0x90
#define STEAM_CONTROLLER_REQUEST_SET_PRNG_ENTROPY           0x96
#define STEAM_CONTROLLER_REQUEST_TURN_OFF_CONTROLLER        0x9F
#define STEAM_CONTROLLER_REQUEST_DONGLE_GET_VERSION         0xA1
#define STEAM_CONTROLLER_REQUEST_ENABLE_PAIRING             0xAD
#define STEAM_CONTROLLER_REQUEST_GET_STRING_ATTRIBUTE       0xAE
#define STEAM_CONTROLLER_REQUEST_UNKNOWN_B1                 0xB1 // ???
#define STEAM_CONTROLLER_REQUEST_DISCONNECT_DEVICE          0xB2
#define STEAM_CONTROLLER_REQUEST_COMMIT_DEVICE              0xB3
#define STEAM_CONTROLLER_REQUEST_DONGLE_GET_WIRELESS_STATE  0xB4
#define STEAM_CONTROLLER_REQUEST_PLAY_MELODY                0xB6
#define STEAM_CONTROLLER_REQUEST_GET_CHIPID                 0xBA
#define STEAM_CONTROLLER_REQUEST_WRITE_EEPROM               0xC1 

#include "steamcontroller.h"

static inline void dumpHex(const uint8_t *data, size_t count) {
  size_t i;
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
}

typedef struct {
  uint8_t reportPage;
  uint8_t featureId;
  uint8_t dataLen;
  uint8_t data[62];
} steam_controller_feature_report_t;

/** Do an ioctl call. */
static int PerformIOCTL(int fd, unsigned long request, void *data) {
  // could insert debug output here
  return ioctl(fd, request, data);
}

/** Do an ioctl call, reset the device and retry on failure. */
static int PerformIOCTLWithReset(const steam_controller_t *pController, unsigned long request, void *data) {
  int res = PerformIOCTL(pController->fd, request, data);
  if (res >= 0)
    return res;

  SteamController_Reset(pController->deviceId);
  return PerformIOCTL(pController->fd, request, data);
}

/** 
 * Send a feature report to the device. 
 * Tries 50 times.
 * @param pController    Steam controller device object to operate on.
 * @param pReport        Feature report to send.
 * @param resetOnFailure If true, the device will be reset if a ioctl call fails.
 */
static inline bool Hid_SendFeatureReport(const steam_controller_t *pController, steam_controller_feature_report_t *pReport, bool resetOnFailure) {
  if (!pController)
    return false;

  if (pController->fd < 0)
    return false;

  if (!pReport)
    return false;

  steam_controller_feature_report_t reportOriginal = *pReport;

  for (int tries=0; tries<50; tries++) {
    *pReport = reportOriginal;

    int res;
    if (resetOnFailure) {
      res  = PerformIOCTLWithReset(pController, HIDIOCSFEATURE(sizeof(*pReport)), pReport);
    } else {
      res  = PerformIOCTL(pController->fd, HIDIOCSFEATURE(sizeof(*pReport)), pReport);
    }

    if (res >= 0)
      return true;

    if (tries < 49)
      usleep(500);
  }

  return false;
}

/** 
 * Get a specific feature report back from the device.
 * Tries 50 times, discards non relevant (non matching feature id) reports.
 * @param pController    Steam controller device object to operate on.
 * @param pReport        Feature report to send.
 */
static inline bool Hid_GetFeatureReport(const steam_controller_t *pController, steam_controller_feature_report_t *pReport) {
  if (!pController)
    return false;

  if (pController->fd < 0)
    return false;

  if (!pReport)
    return false;

  uint8_t reportPage = pReport->reportPage;
  uint8_t featureId  = pReport->featureId;

  for (int tries=0; tries<50; tries++) {
    int res = PerformIOCTL(pController->fd, HIDIOCGFEATURE(sizeof(*pReport)), pReport);
    if (res >= 0 && pReport->reportPage == reportPage && pReport->featureId == featureId) {
      return true;
    }

    if (tries < 49)
      usleep(500);
  }
  return false;
}

/**
 * Check if a controller device is a wired controller or wireless dongle.
 * Compares the USB vendor and product ID to known values.
 * @param fd          Open file descriptor of device.
 * @param isWireless  Where to store whether the device is wireless.
 * @return true if device was recognized, *isWireless will be valid.
 */
static bool SteamController_GetType(int fd, bool *isWireless) {
  struct hidraw_devinfo devInfo;
  int res = PerformIOCTL(fd, HIDIOCGRAWINFO, &devInfo);

  if (res == 0) {
    if (devInfo.vendor == USB_VID_VALVE && devInfo.product == USB_PID_STEAMCONTROLLER_WIRED) {
      *isWireless = false;
      return true;
    }

    if (devInfo.vendor == USB_VID_VALVE && devInfo.product == USB_PID_STEAMCONTROLLER_WIRELESS) {
      *isWireless = true;
      return true;
    }
  }
  return false;
}

/**
 * Enumerate all steam controllers and wireless dongles on the system.
 * @param pDeviceIds    Pointer to an array of ints that will receive valid deviceIds.
 * @param maxDevices    Maxmimal number of devices to find.
 * @return The total number of found devices.
 */
size_t SteamController_EnumControllers(int *pDeviceIds, size_t maxDevices) {

  // "HID: Vendor specific page". Report descriptor must start with these
  // bytes to be a valid steam controller device.
  static const uint8_t ReportDescriptorMagic[3] = { 
    0x06, 0x00, 0xff  
  };

  static const char * pGlobPatterns[] = {
    GLOB_PATTERN_STEAMCONTROLLER_ALL_DEVICES_WIRELESS,
    GLOB_PATTERN_STEAMCONTROLLER_ALL_DEVICES_WIRED,
    NULL
  };

  const char ** ppGlobPattern = pGlobPatterns;
  size_t deviceCount   = 0;

  while(*ppGlobPattern) {
    glob_t  globData;
    if (glob(*ppGlobPattern, 0, NULL, &globData) == 0) {     
      for (size_t i=0; i<globData.gl_pathc; i++) {
        // Get full path of hidraw device.
        char *pHidrawPath = globData.gl_pathv[i];
        if (!pHidrawPath)
          continue;

        fprintf(stderr, "globbed path %s\n", pHidrawPath);

        // Path should end with /hidraw<num>. Extract num as the deviceId.
        char *pLastDirSep = strrchr(pHidrawPath, '/');
        if (!pLastDirSep) 
          continue;

        if (strncmp(pLastDirSep, "/hidraw", 7))
          continue;

        int deviceId = strtol(pLastDirSep + 7, NULL, 10);

        // Find report descriptor path for the device.
        char reportDescriptorPath[4096];
        strncpy(reportDescriptorPath, pHidrawPath, sizeof(reportDescriptorPath));

        char *pHidRawHidRaw = strstr(reportDescriptorPath, "/hidraw/hidraw");
        if (!pHidRawHidRaw)
          continue;

        strncpy(pHidRawHidRaw, "/report_descriptor", reportDescriptorPath+4096-pHidRawHidRaw);

        // Open report descriptor and check if it begins with a vendor specific page.
        int fdReportDescriptor = open(reportDescriptorPath, O_CLOEXEC);
        if (fdReportDescriptor < 0)
          continue;

        char bufMagic[3];
        read(fdReportDescriptor, bufMagic, 3);
        close(fdReportDescriptor);

        if (memcmp(bufMagic, ReportDescriptorMagic, 3)) {
          fprintf(stderr, "Device %d: report descriptor mismatch...\n", deviceId);
          continue;
        }

        // Add to devices if there is still room left.
        if (maxDevices) {
          maxDevices--;
          *pDeviceIds = deviceId;
          pDeviceIds++;
        }
        deviceCount++;
      }
      globfree(&globData);
    }
    ppGlobPattern ++;
  }

  return deviceCount;
}

/**
 * Enumerate all wireless dongles that are in boot loader mode.
 * @param pDeviceIds    Pointer to an array of ints that will receive valid deviceIds.
 * @param maxDevices    Maxmimal number of devices to find.
 * @return The total number of found devices.
 */
size_t SteamController_EnumBootLoaders(int *pDeviceIds, size_t maxDevices) {
  size_t deviceCount   = 0;

  glob_t  globData;
  int     res = glob(GLOB_PATTERN_STEAMCONTROLLER_ALL_DEVICES_DONGLE_BOOTLOADER, 0, NULL, &globData);
  if (res != 0) 
    return 0;

  for (size_t i=0; i<globData.gl_pathc; i++) {
    char *pHidrawPath  = globData.gl_pathv[i];
    if (!pHidrawPath)
      continue;

    char *pLastDirSep      = strrchr(pHidrawPath, '/');
    if (!pLastDirSep) 
      continue;

    if (strncmp(pLastDirSep, "/hidraw", 7))
      continue;

    int deviceId = strtol(pLastDirSep+7, NULL, 10);
    if (maxDevices) {
      maxDevices--;
      *pDeviceIds = deviceId;
      pDeviceIds++;
    }
    deviceCount++;
  }

  globfree(&globData);
  return deviceCount;
}

/** 
 * Open a steam controller device.
 * @param pController   Controller object to initialize.
 * @param deviceId      Id of device to open.
 * @return true if successful.
 */
bool SteamController_Open(steam_controller_t *pController, int deviceId) {
  if (!pController)
    return false;

  // Try to open the hidraw device with the specified id.
  char devicePath[64];
  snprintf(devicePath, sizeof(devicePath), "/dev/hidraw%d", deviceId);

  pController->fd = open(devicePath, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (pController->fd < 0) {
    perror(devicePath);
    return false;
  }

  // Identify steam controller.
  if (!SteamController_GetType(pController->fd, &pController->isWireless)) {
    SteamController_Close(pController);
    return false;
  }

  pController->deviceId = deviceId;
  pController->configFlags = 0;
  return true;
}

/**
 * Close a controller device.
 * @note This only closes the device. It does not unpair, disconnect or turn off
 *       any controller.
 * @param pController   Controller object to deinitialize.
 */
void SteamController_Close(steam_controller_t *pController) {
  if (!pController)
    return;

  if (pController->fd >= 0)
    close(pController->fd);

  pController->fd = -1;
}

/** 
 * Resets a controller device.
 * The device will be reset using the USBDEVFS_RESET call on linux.
 * This is equivalent to replugging the device.
 * @note This only works for wired controllers.
 * @param deviceId  Id of the device to reset.
 */
bool SteamController_Reset(int deviceId) {
  char    devicePattern[128];
  char    temp[128];
  glob_t  globData;

  snprintf(devicePattern, sizeof(devicePattern), GLOB_PATTERN_STEAMCONTROLLER_DEVICE_WIRED, deviceId);

  if (glob(devicePattern, 0, NULL, &globData))
    return false;
  
  if (!globData.gl_pathv) {
    globfree(&globData);
    return false;  
  }

  for (size_t i=0; i<globData.gl_pathc; i++) {
    char *pDevicePath  = globData.gl_pathv[i];
    FILE *pFile;
    int   busNum, devNum;
    int   fd;

    char *pLastDirSep = strrchr(pDevicePath, '/');
    if (!pLastDirSep)
      continue;

    if (strncmp(pLastDirSep, "/hidraw", 7)) 
      continue;

    // get bus and device number of controller
    snprintf(temp, sizeof(temp), "%s/device/../../busnum", pDevicePath);
    pFile = fopen(temp, "r");
    if (!pFile) {
      fprintf(stderr, "Warning: Couldn't open %s\n", temp);
      continue;
    }
    fgets(temp, sizeof(temp), pFile);
    busNum = strtol(temp, NULL, 10);
    fclose(pFile);

    snprintf(temp, sizeof(temp), "%s/device/../../devnum", pDevicePath);
    pFile = fopen(temp, "r");
    if (!pFile) {
      fprintf(stderr, "Warning: Couldn't open %s\n", temp);
      continue;
    }
    fgets(temp, sizeof(temp), pFile);
    devNum = strtol(temp, NULL, 10);
    fclose(pFile);

    // open usb device
    snprintf(temp, sizeof(temp), "/dev/bus/usb/%.3d/%.3d", busNum, devNum);
    fd = open(temp, O_RDWR);
    if (fd < 0) {
      fprintf(stderr, "Warning: Couldn't open %s\n", temp);
      continue;
    }

    // perform reset
    fprintf(stderr, "Wired controller hung, attempting USB device reset on %s.\n", temp);
    if (PerformIOCTL(fd, USBDEVFS_RESET, NULL)) {
      fprintf(stderr, "Warning: Couldn't reset the USB device %s\n", temp);
    }
    close(fd);
  }

  globfree(&globData);
  return true;
}

/** Add a settings parameter and value to an SET_SETTINGS feature report. */
static inline void SteamController_FeatureReportAddSetting(steam_controller_feature_report_t *featureReport, uint8_t setting, uint16_t value) {
  featureReport->data[featureReport->dataLen++] = setting;
  featureReport->data[featureReport->dataLen++] = value & 0xff;
  featureReport->data[featureReport->dataLen++] = value >> 8;
}

/**
 * Do some basic initialization.
 */
static bool SteamController_Initialize(steam_controller_t *pController, unsigned configFlags, uint16_t timeoutSeconds, uint16_t brightness) {
  assert(pController);

  if (!pController)
    return false;

  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_GET_ATTRIBUTES;

  if (!Hid_SendFeatureReport(pController, &featureReport, true)) {
    fprintf(stderr, "GET_ATTRIBUTES failed for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_GET_ATTRIBUTES;

  if (!Hid_GetFeatureReport(pController, &featureReport) || featureReport.dataLen < 4) {
    fprintf(stderr, "Bad GET_ATTRIBUTES response for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_CLEAR_MAPPINGS;

  if (!Hid_SendFeatureReport(pController, &featureReport, false)) {
    fprintf(stderr, "CLEAR_MAPPINGS failed for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }

  if (!SteamController_Reconfigure(pController, configFlags, timeoutSeconds, brightness)) {
    return false;
  }

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_GET_CHIPID;

  if (!Hid_SendFeatureReport(pController, &featureReport, true)) {
    fprintf(stderr, "STEAM_CONTROLLER_REQUEST_GET_CHIPID for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_GET_CHIPID;

  if (!Hid_GetFeatureReport(pController, &featureReport)) {
    fprintf(stderr, "STEAM_CONTROLLER_REQUEST_GET_CHIPID for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }

  return true;
}

/**
 * Configure a steam controller device to be functional.
 * @param pController   Controller object.
 * @param configFlags   Flags selecting certain features.
 */
bool SteamController_Configure(steam_controller_t *pController, unsigned configFlags, uint16_t timeoutSeconds, uint8_t brightness) {
  assert(pController);
  assert(pController >= 0);

  if (!pController)
    return false;

  if (pController->fd < 0)
    return false;

  if (pController->isWireless) {
    steam_controller_feature_report_t featureReport;

    memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId   = 0;

    Hid_GetFeatureReport(pController, &featureReport);

    // send some random numbers to device for its entropy pool
    int fdRandom = open("/dev/urandom", O_CLOEXEC);
    if (fdRandom < 0) {
      fprintf(stderr, "failed to open /dev/urandom\n");
      return false;
    }

    char randomBytes[16];
    read(fdRandom, randomBytes, 16);
    close(fdRandom);

    memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId   = STEAM_CONTROLLER_REQUEST_SET_PRNG_ENTROPY;
    featureReport.dataLen     = 16;

    memcpy(featureReport.data, randomBytes, 16);

    if (!Hid_SendFeatureReport(pController, &featureReport, false)) {
      fprintf(stderr, "SET_PRNG_ENTROPY failed for controller at /dev/hidraw%d", pController->deviceId);
      return false;
    }

    // TODO: find out what this does
    memset(&featureReport, 0, sizeof(featureReport));
    featureReport.featureId   = STEAM_CONTROLLER_REQUEST_UNKNOWN_B1;
    featureReport.dataLen     = 2;

    Hid_SendFeatureReport(pController, &featureReport, false);
  }

  return SteamController_Initialize(pController, configFlags, timeoutSeconds, brightness);
}

/**
 * Select a different feature set for the controller device.
 */
bool SteamController_Reconfigure(steam_controller_t *pController, unsigned configFlags, uint16_t timeoutSeconds, uint16_t brightness) {
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_SET_SETTINGS;

  SteamController_FeatureReportAddSetting(&featureReport, 0x03, 0x2d); // 0x2d, unknown
  SteamController_FeatureReportAddSetting(&featureReport, 0x05, (configFlags & STEAM_CONTROLLER_CONFIG_FLAG_RIGHT_PAD_HAPTIC_TOUCH)     ? 1 : 0);
  SteamController_FeatureReportAddSetting(&featureReport, 0x07, (configFlags & STEAM_CONTROLLER_CONFIG_FLAG_STICK_HAPTIC)               ? 0 : 7);
  SteamController_FeatureReportAddSetting(&featureReport, 0x08, (configFlags & STEAM_CONTROLLER_CONFIG_FLAG_RIGHT_PAD_HAPTIC_TRACKBALL) ? 0 : 7);
  SteamController_FeatureReportAddSetting(&featureReport, 0x18, 0x00); // 0x00, unknown
  SteamController_FeatureReportAddSetting(&featureReport, 0x2d, brightness); // home button brightness 0-100
  SteamController_FeatureReportAddSetting(&featureReport, 0x2e, 0x00); // 0x00, unknown
  SteamController_FeatureReportAddSetting(&featureReport, 0x2f, 0x01); // 0x01, unknown
  SteamController_FeatureReportAddSetting(&featureReport, 0x30, (configFlags & 31));
  SteamController_FeatureReportAddSetting(&featureReport, 0x31, (configFlags & STEAM_CONTROLLER_CONFIG_FLAG_SEND_BATTERY_STATUS)        ? 2 : 0);
  SteamController_FeatureReportAddSetting(&featureReport, 0x32, 0x012c); // 0x012c seconds to controller shutdown

  if (!Hid_SendFeatureReport(pController, &featureReport, true)) {
    fprintf(stderr, "SET_SETTINGS failed for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }

  pController->configFlags    = configFlags;
  pController->timeoutSeconds = timeoutSeconds;
  pController->brightness     = brightness;
  return true;
}

/** 
 * Reconfigure controller with cached settings.
 */
bool SteamController_ReconfigurePrevious(steam_controller_t *pController) {
  return SteamController_Reconfigure(pController, pController->configFlags, pController->timeoutSeconds, pController->brightness);
}

/**
 * Set the brightness of the home button.
 * @param brightness The brightness of the home button in percent (0-100).
 */
bool SteamController_SetHomeButtonBrightness(steam_controller_t *pController, uint8_t brightness) {
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_SET_SETTINGS;

  SteamController_FeatureReportAddSetting(&featureReport, 0x2d, brightness); // home button brightness 0-100

  if (!Hid_SendFeatureReport(pController, &featureReport, true)) {
    fprintf(stderr, "SET_SETTINGS failed for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }

  pController->brightness     = brightness;
  return true;
}

/**
 * Store the startup and shutdown melody in the controller's EEPROM.
 */
void SteamController_SaveMelodies(const steam_controller_t *pController, uint8_t startupMelody, uint8_t shutdownMelody) {
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_WRITE_EEPROM;
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
  Hid_SendFeatureReport(pController, &featureReport, true);
}

/**
 * Set wireless dongle to accept wireless controllers.
 * @param pController   Controller object to use.
 * @param enable        If true enable pairing.
 * @param deviceType    Type of device to connect to. Only known value is 0x3c for steam wireless controller.
 *                      If set to 0, defaults to 0x3c.
 * @todo Find out what arg3 does.
 */
bool SteamController_EnablePairing(const steam_controller_t *pController, bool enable, uint8_t deviceType) {
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_ENABLE_PAIRING;
  featureReport.dataLen     = 2;
  featureReport.data[0]     = enable ? 1 : 0;
  featureReport.data[1]     = enable ? (deviceType ? deviceType : 0x3c) : 0;

  if (!Hid_SendFeatureReport(pController, &featureReport, false)) {
    fprintf(stderr, "ENABLE_PAIRING failed for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }
  return true;
}

/**
 * Accept pairing connection.
 * A pairing event will be sent when a controller connects.
 */
bool SteamController_CommitPairing(const steam_controller_t *pController, bool connect) {
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = connect ? 
                              STEAM_CONTROLLER_REQUEST_COMMIT_DEVICE :
                              STEAM_CONTROLLER_REQUEST_DISCONNECT_DEVICE;

  if (!Hid_SendFeatureReport(pController, &featureReport, true)) {
    fprintf(stderr, "COMMIT_DEVICE/DISCONNECT_DEVICE failed for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }
  return true;
}

/**
 * Get the current connection state of a wireless dongle.
 */
bool SteamController_QueryWirelessState(const steam_controller_t *pController, uint8_t *state) {
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_DONGLE_GET_WIRELESS_STATE;

  if (!Hid_SendFeatureReport(pController, &featureReport, false)) {
    fprintf(stderr, "DONGLE_GET_WIRELESS_STATE (set) failed for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_DONGLE_GET_WIRELESS_STATE;

  if (!Hid_GetFeatureReport(pController, &featureReport)) {
    fprintf(stderr, "DONGLE_GET_WIRELESS_STATE (get) failed for controller at /dev/hidraw%d\n", pController->deviceId);
    return false;
  }

  *state = featureReport.data[0];
  return true;
}

/**
 * Turn off the controller.
 */ 
bool SteamController_TurnOff(const steam_controller_t *pController) {
  if (!pController)
    return false;

  if (pController->fd < 0)
    return false;
  
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_TURN_OFF_CONTROLLER;
  featureReport.dataLen     = 4;
  memcpy(featureReport.data, "off!", 4);

  return Hid_SendFeatureReport(pController, &featureReport, true);
}

/**
 * Get firmware version of wireless dongle.
 */
uint32_t SteamController_GetDongleVersion(const steam_controller_t *pController) {
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_DONGLE_GET_VERSION;

  if (!Hid_SendFeatureReport(pController, &featureReport, true)) {
    return 0;
  }

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_DONGLE_GET_VERSION;

  if (!Hid_GetFeatureReport(pController, &featureReport))
    return 0;

  return (featureReport.data[0]<<24) | (featureReport.data[1]<<16) | (featureReport.data[2]<<8) | (featureReport.data[3]);
}

/**
 * Set's the controller device into bootloader mode for firmware upgrades.
 */
bool SteamController_ResetForBootloader(const steam_controller_t *pController) {
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId = STEAM_CONTROLLER_REQUEST_START_BOOTLOADER;
  featureReport.dataLen   = 4;
  memcpy(featureReport.data, "zot!", 4);

  return Hid_SendFeatureReport(pController, &featureReport, true);
}

/**
 * Pulse the haptic feedback of the controller using pulse width modulation.
 * 
 * @param pController Controller object to use.
 * @param motor       Actuator to use, 0 == right, 1 == left.
 * @param onTime      PWM on time in microseconds.
 * @param offTime     PWM off time in microseconds.
 * @param count       PWM cycle count.
 */
bool SteamController_TriggerHaptic(const steam_controller_t *pController, uint16_t motor, uint16_t onTime, uint16_t offTime, uint16_t count) {
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_TRIGGER_HAPTIC_PULSE;
  featureReport.dataLen     = motor > 0xFF ? 8 : 7;
  featureReport.data[0]     = motor;
  featureReport.data[1]     = onTime;
  featureReport.data[2]     = onTime >> 8;
  featureReport.data[3]     = offTime;
  featureReport.data[4]     = offTime >> 8;
  featureReport.data[5]     = count;
  featureReport.data[6]     = count >> 8;
  featureReport.data[7]     = motor >> 8;

  if (!Hid_SendFeatureReport(pController, &featureReport, true)) {
    return false;
  }
  return true;
}

/**
 * Read the next event from the device.
 * 
 * @param pController   Device to use.
 * @param pEvent        Where to store event data.
 * 
 * @return The type of the received event. If no event was received this is 0.
 */
uint8_t SteamController_ReadEvent(const steam_controller_t *pController, steam_controller_event_t *pEvent) {
  uint8_t eventData[256];
  
  int res = read(pController->fd, eventData, sizeof(eventData));
  if (res < 0) return 0;

#if 0
  //  for debug:
  fprintf(stderr, "\x1b[;H%d\n", res);
  for (int i=0;i<eventData[2]+1;i++) fprintf(stderr, "\n\n\n\n\n");

  dumpHex(eventData, res);
  fprintf(stderr, "\n");
  fflush(stderr);
#endif

  /*
    Layout of each message seems to be:

    0x0000 01       1 byte    Seems to always be 0x01. Could be an id if multiple controllers are paired with the same dongle. TODO: get second controller
    0x0001 00       1 byte    Seems to always be 0x00. Could also be the high order byte of the other byte.
    0x0002 xx       1 byte    Event type. So far the following values were observed:

                        0x01  Update. Contains axis and button data.
                        0x03  Connection update. Sent when a controller connects or disconnects (turned off/out of range).
                        0x04  Battery update. Sent occasionally to update battery status of controller. Needs to be enabled with STEAM_CONTROLLER_CONFIG_FLAG_SEND_BATTERY_STATUS
                        0x05  ??? Seems mostly random. Needs some tinkering with settings, controller disconnects after a while.

    0x0003 xx       1 byte    Unknown. Seems to always be 0x3c (the device type) for event type 1, 
                              0x01 for event type 3 and 0x0b for event type 4. 

    0x0004                    Event type specific data.
  */
  uint8_t eventType = eventData[0x02];
  switch(eventType) {

    case STEAM_CONTROLLER_EVENT_UPDATE:
      /*
        0x0004 xx xx xx xx          1 ulong   Appears to be some kind of timestamp or message counter.
        0x0008 xx xx xx             3 bytes   Button data.
        0x000b xx                   1 byte    Left trigger value.
        0x000c xx                   1 byte    Right trigger value.
        0x000d 00 00 00             3 bytes   Padding?
        0x0010 xx xx yy yy          2 sshorts Left position data. If STEAM_CONTROLLER_BUTTON_LFINGER is set 
                                              this is the position of the thumb on the left touch pad. 
                                              Otherwise it is the stick position.
        0x0014 xx xx yy yy          2 sshorts Right position data. This is the position of the thumb on the 
                                              right touch pad.
        0x0018 00 00 00 00          4 bytes   Padding?                                         
        0x001c xx xx yy yy zz zz    3 sshorts Acceleration along X,Y,Z axes.
        0x0022 xx xx yy yy zz zz    3 sshorts Angular velocity (gyro) along X,Y,Z axes.
        0x0028 xx xx yy yy zz zz    3 sshorts Orientation vector. 
      */
      pEvent->update.timeStamp          = eventData[0x04] | (eventData[0x05] << 8) | (eventData[0x06] << 16) | (eventData[0x07] << 24);
      pEvent->update.buttons            = eventData[0x08] | (eventData[0x09] << 8) | (eventData[0x0a] << 16);

      pEvent->update.leftTrigger        = eventData[0x0b];
      pEvent->update.rightTrigger       = eventData[0x0c];

      pEvent->update.leftXY.x           = eventData[0x10] | (eventData[0x11] << 8);
      pEvent->update.leftXY.y           = eventData[0x12] | (eventData[0x13] << 8);

      pEvent->update.rightXY.x          = eventData[0x14] | (eventData[0x15] << 8);
      pEvent->update.rightXY.y          = eventData[0x16] | (eventData[0x17] << 8);

      pEvent->update.acceleration.x     = eventData[0x1c] | (eventData[0x1d] << 8);
      pEvent->update.acceleration.y     = eventData[0x1e] | (eventData[0x1f] << 8);
      pEvent->update.acceleration.z     = eventData[0x20] | (eventData[0x21] << 8);

      pEvent->update.angularVelocity.x  = eventData[0x22] | (eventData[0x23] << 8);
      pEvent->update.angularVelocity.y  = eventData[0x24] | (eventData[0x25] << 8);
      pEvent->update.angularVelocity.z  = eventData[0x26] | (eventData[0x27] << 8);

      pEvent->update.orientation.x      = eventData[0x28] | (eventData[0x29] << 8);
      pEvent->update.orientation.y      = eventData[0x2a] | (eventData[0x2b] << 8);
      pEvent->update.orientation.z      = eventData[0x2c] | (eventData[0x2d] << 8);
      break;

    case STEAM_CONTROLLER_EVENT_BATTERY:
      /*
        0x0004 xx xx xx xx          1 ulong   Another timestamp or message counter, separate from update event.
        0x0008 00 00 00 00          4 bytes   Padding?                                         
        0x000c xx xx                1 ushort  Battery voltage in millivolts (both cells).
        0x000e 64 00                1 ushort  Unknown. Seems to be stuck at 0x0064 (100 in decimal).
      */
      pEvent->battery.voltage = eventData[0x0c] | (eventData[0x0d] << 8);
      break;

    case STEAM_CONTROLLER_EVENT_CONNECTION:
      /*
        0x0004 xx                   1 byte    Connection detail. 0x01 for disconnect, 0x02 for connect, 0x03 for pairing request.
        0x0005 xx xx xx             3 bytes   On disconnect: upper 3 bytes of timestamp of last received update.
      */
      pEvent->connection.details = eventData[4];
      break;

    default:
      return 0;
  }
  return eventType;
}

/**
 * Updates the state of a controller from an event.
 * Automatically disambiguates betwee left pad coordinates and stick position.
 * @param pState State to update.
 * @param pEvent Event to use.
 */
void SteamController_UpdateState(steam_controller_state_t *pState, const steam_controller_update_event_t *pEvent) { 
  if (pEvent->timeStamp == pState->timeStamp) {
    return;
  }

  pState->timeStamp       = pEvent->timeStamp;
  pState->activeButtons   = pEvent->buttons;

  pState->leftTrigger     = pEvent->leftTrigger;
  pState->rightTrigger    = pEvent->rightTrigger;

  if (pEvent->buttons & STEAM_CONTROLLER_BUTTON_LFINGER) {
    pState->leftPad = pEvent->leftXY;
  } else {
    pState->stick       = pEvent->leftXY;
    if ((pEvent->buttons & STEAM_CONTROLLER_FLAG_PAD_STICK) == 0) {
      pState->leftPad.x     = 0;
      pState->leftPad.y     = 0;
    }
  }

  pState->rightPad        = pEvent->rightXY;

  pState->acceleration    = pEvent->acceleration;
  pState->angularVelocity = pEvent->angularVelocity;
  pState->orientation     = pEvent->orientation;
}

/**
 * Play one of the predefined melodies.
 */
void SteamController_PlayMelody(const steam_controller_t *pController, uint32_t melodyId) {
  steam_controller_feature_report_t featureReport;

  memset(&featureReport, 0, sizeof(featureReport));
  featureReport.featureId   = STEAM_CONTROLLER_REQUEST_PLAY_MELODY;
  featureReport.dataLen     = 0x04;
  featureReport.data[0] = melodyId;
  featureReport.data[1] = melodyId >> 8;
  featureReport.data[2] = melodyId >> 16;
  featureReport.data[3] = melodyId >> 24;

  Hid_SendFeatureReport(pController, &featureReport, true);
}

#endif
