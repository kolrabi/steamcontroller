#if __linux__

#include "steamcontroller.h"
#include "common.h"

#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <linux/hiddev.h>
#include <linux/usbdevice_fs.h>
#include <unistd.h>
#include <fcntl.h>
#include <glob.h>

#define GLOB_PATTERN_STEAMCONTROLLER_ALL_DEVICES_WIRED              "/sys/bus/hid/devices/????:28DE:1102.*/hidraw/hidraw*"
#define GLOB_PATTERN_STEAMCONTROLLER_ALL_DEVICES_WIRELESS           "/sys/bus/hid/devices/????:28DE:1142.*/hidraw/hidraw*"
#define GLOB_PATTERN_STEAMCONTROLLER_ALL_DEVICES_DONGLE_BOOTLOADER  "/sys/bus/hid/devices/????:28DE:1042.*/hidraw/hidraw*"
#define GLOB_PATTERN_STEAMCONTROLLER_DEVICE_WIRED                   "/sys/bus/hid/devices/????:28DE:1102.*/hidraw/hidraw%d"

struct SteamControllerDevice {
  int     fd;
  bool    isWireless;
};

struct SteamControllerDeviceEnum {
  struct SteamControllerDeviceEnum *next;
  char *path;
};

/** 
 * Send a feature report to the device. 
 * Tries 50 times.
 * @param pController    Steam controller device object to operate on.
 * @param pReport        Feature report to send.
 * @param resetOnFailure If true, the device will be reset if a ioctl call fails.
 */
bool SteamController_HIDSetFeatureReport(const SteamControllerDevice *pDevice, SteamController_HIDFeatureReport *pReport) {
  if (!pDevice)
    return false;

  if (!pReport)
    return false;

  for (int tries=0; tries<50; tries++) {
    int res = ioctl(pDevice->fd, HIDIOCSFEATURE(sizeof(*pReport)), pReport);
    if (res >= 0)
      return true;

    if (tries < 49)
      usleep(500);
  }

  perror("HIDIOCSFEATURE");
  return false;
}

/** 
 * Get a specific feature report back from the device.
 * Tries 50 times, discards non relevant (non matching feature id) reports.
 * @param pController    Steam controller device object to operate on.
 * @param pReport        Feature report to send.
 */
bool SteamController_HIDGetFeatureReport(const SteamControllerDevice *pDevice, SteamController_HIDFeatureReport *pReport) {
  if (!pDevice)
    return false;

  if (!pReport)
    return false;

  uint8_t featureId  = pReport->featureId;

  SteamController_HIDSetFeatureReport(pDevice, pReport);

  for (int tries=0; tries<50; tries++) {
    int res = ioctl(pDevice->fd, HIDIOCGFEATURE(sizeof(*pReport)), pReport);
    if (res >= 0) {
      if (pReport->featureId == featureId) {
        return true;
      }
      continue;
    }

    if (tries < 49)
      usleep(500);
  }
  perror("HIDIOCGFEATURE");
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
  int res = ioctl(fd, HIDIOCGRAWINFO, &devInfo);

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
 */
SteamControllerDeviceEnum *SteamController_EnumControllerDevices() {

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

  SteamControllerDeviceEnum *pEnum = NULL;

  while(*ppGlobPattern) {
    glob_t  globData;
    if (glob(*ppGlobPattern, 0, NULL, &globData) == 0) {     
      for (size_t i=0; i<globData.gl_pathc; i++) {
        // Get full path of hidraw device.
        char *pHidrawPath = globData.gl_pathv[i];
        if (!pHidrawPath)
          continue;

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

        snprintf(reportDescriptorPath, sizeof(reportDescriptorPath), "/dev/hidraw%d", deviceId);

        SteamControllerDeviceEnum *pNewEnum = malloc(sizeof(SteamControllerDeviceEnum));
        pNewEnum->next = pEnum;
        pNewEnum->path = strdup(reportDescriptorPath);
        pEnum = pNewEnum;
      }
      globfree(&globData);
    }
    ppGlobPattern ++;
  }

  return pEnum;
}

SteamControllerDeviceEnum *SteamController_NextControllerDevice(SteamControllerDeviceEnum *pCurrent) {
  if (!pCurrent)
    return NULL;

  SteamControllerDeviceEnum *pNext = pCurrent->next;

  free(pCurrent->path);
  free(pCurrent);

  return pNext;
}

/** 
 * Open a steam controller device.
 * @param pController   Controller object to initialize.
 * @param deviceId      Id of device to open.
 * @return true if successful.
 */
SteamControllerDevice *SteamController_Open(const SteamControllerDeviceEnum *pEnum) {
  if (!pEnum)
    return NULL;

  if (!pEnum->path)
    return NULL;

  // Try to open the hidraw device with the specified id.
  int fd = open(pEnum->path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0) {
    perror(pEnum->path);
    return NULL;
  }

  // Identify steam controller.
  bool isWireless;
  if (!SteamController_GetType(fd, &isWireless)) {
    return NULL;
  }

  SteamControllerDevice *pDevice = malloc(sizeof(SteamControllerDevice));
  pDevice->fd = fd;
  pDevice->isWireless = isWireless;

  SteamController_Initialize(pDevice);
  return pDevice;
}

/**
 * Close a controller device.
 * @note This only closes the device. It does not unpair, disconnect or turn off
 *       any controller.
 * @param pController   Controller object to deinitialize.
 */
void SteamController_Close(SteamControllerDevice *pDevice) {
  if (!pDevice)
    return;

  close(pDevice->fd);
  free(pDevice);
}

bool SteamController_IsWirelessDongle(const SteamControllerDevice *pDevice) {
  if (!pDevice)
    return false;
  return pDevice->isWireless;
}

uint8_t SteamController_ReadRaw(const SteamControllerDevice *pDevice, uint8_t *buffer, uint8_t maxLen) {
  if (!pDevice)
    return 0;

  int res = read(pDevice->fd, buffer, maxLen);
  if (res <= 0) {
//    fprintf(stderr, "fd: %d\n", pDevice->fd);
//    perror("ReadRaw");
    return 0;
  }

  return res;
}
#endif
