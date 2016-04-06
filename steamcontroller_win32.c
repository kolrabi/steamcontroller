#if _WIN32

#include "steamcontroller.h"
#include "common.h"

#include <windows.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <guiddef.h>

#include <stdio.h>

struct SteamControllerDevice {
  HANDLE      devHandle;
  HANDLE      reportEvent;
  OVERLAPPED  overlapped;
  bool    isWireless;
};

struct SteamControllerDeviceEnum {
  struct SteamControllerDeviceEnum *next;
  SP_DEVICE_INTERFACE_DETAIL_DATA *pDevIntfDetailData;
  HIDD_ATTRIBUTES hidAttribs;
};

SCAPI SteamControllerDeviceEnum * SteamController_EnumControllerDevices() {
  GUID hidGuid;
  HidD_GetHidGuid(&hidGuid);

  HDEVINFO  devInfo;
  devInfo = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_INTERFACEDEVICE | DIGCF_PRESENT);

  SP_DEVICE_INTERFACE_DATA devIntfData  = { .cbSize = sizeof(SP_DEVICE_INTERFACE_DATA) };
  DWORD                    devIndex     = 0;

  SteamControllerDeviceEnum *pEnum = NULL;

  while(SetupDiEnumDeviceInterfaces(devInfo, NULL, &hidGuid, devIndex++, &devIntfData)) {
    DWORD reqSize;

    SetupDiGetDeviceInterfaceDetail(devInfo, &devIntfData, NULL, 0, &reqSize, NULL);

    SP_DEVICE_INTERFACE_DETAIL_DATA *pDevIntfDetailData;
    pDevIntfDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*)malloc(reqSize);
    pDevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    if (!SetupDiGetDeviceInterfaceDetail(devInfo, &devIntfData, pDevIntfDetailData, reqSize, &reqSize, NULL)) {
      fprintf(stderr, "SetupDiGetDeviceInterfaceDetail failed. Last error: %08lx\n", GetLastError());
      free(pDevIntfDetailData);
      continue;
    }

    HANDLE devFile;
    devFile = CreateFile(
      pDevIntfDetailData->DevicePath, 
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      0,
      OPEN_EXISTING,
      FILE_FLAG_OVERLAPPED,
      0
    );
    if (devFile == INVALID_HANDLE_VALUE) {
      free(pDevIntfDetailData);
      continue;
    }

    HIDD_ATTRIBUTES hidAttribs = { .Size = sizeof(HIDD_ATTRIBUTES) };
    HidD_GetAttributes(devFile, &hidAttribs);
    CloseHandle(devFile);

    if (hidAttribs.VendorID != USB_VID_VALVE) {
      free(pDevIntfDetailData);
      continue;
    }

    if (hidAttribs.ProductID != USB_PID_STEAMCONTROLLER_WIRED && hidAttribs.ProductID != USB_PID_STEAMCONTROLLER_WIRELESS) {
      free(pDevIntfDetailData);
      continue;
    }

    SteamControllerDeviceEnum *pNewEnum = malloc(sizeof(SteamControllerDeviceEnum));
    pNewEnum->next = pEnum;
    pNewEnum->pDevIntfDetailData = pDevIntfDetailData;
    pNewEnum->hidAttribs = hidAttribs;
    pEnum = pNewEnum;
  }

  SetupDiDestroyDeviceInfoList(devInfo);

  return pEnum;
}

SCAPI SteamControllerDeviceEnum * SteamController_NextControllerDevice(SteamControllerDeviceEnum *pCurrent) {
  if (!pCurrent)
    return NULL;

  SteamControllerDeviceEnum *pNext = pCurrent->next;

  free(pCurrent->pDevIntfDetailData);
  free(pCurrent);

  return pNext;
}

SCAPI SteamControllerDevice * SteamController_Open(const SteamControllerDeviceEnum *pEnum) {
  if (!pEnum)  
    return NULL;

  SteamControllerDevice *pDevice = malloc(sizeof(SteamControllerDevice));
  pDevice->isWireless = pEnum->hidAttribs.ProductID == USB_PID_STEAMCONTROLLER_WIRELESS;
  pDevice->devHandle  = CreateFile(
    pEnum->pDevIntfDetailData->DevicePath, 
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    0,
    OPEN_EXISTING,
    0,
    NULL
  );

  pDevice->reportEvent = CreateEvent(NULL, true, false, NULL);
  pDevice->overlapped.hEvent = pDevice->reportEvent;
  pDevice->overlapped.Offset = 0;
  pDevice->overlapped.OffsetHigh = 0;

  SteamController_Initialize(pDevice);
  return pDevice;
}

SCAPI void SteamController_Close(SteamControllerDevice *pDevice) {
  if (!pDevice)
    return;

  CloseHandle(pDevice->reportEvent);
  CloseHandle(pDevice->devHandle);
  free(pDevice);
}

bool SteamController_HIDSetFeatureReport(const SteamControllerDevice *pDevice, SteamController_HIDFeatureReport *pReport) {
  if (!pDevice || !pReport || !pDevice->devHandle)
    return false;

  fprintf(stderr, "SteamController_HIDSetFeatureReport %02x\n", pReport->featureId);

  for (int i=0; i<50; i++) {
    bool ok = HidD_SetFeature(pDevice->devHandle, pReport, sizeof(SteamController_HIDFeatureReport));
    if (ok)
      return true;

    fprintf(stderr, "HidD_SetFeature failed. Last error: %08lx\n", GetLastError());
    Sleep(1);
  }

  return false;
}

bool SteamController_HIDGetFeatureReport(const SteamControllerDevice *pDevice, SteamController_HIDFeatureReport *pReport) {
  if (!pDevice || !pReport || !pDevice->devHandle)
    return false;

  uint8_t featureId   = pReport->featureId;

  SteamController_HIDSetFeatureReport(pDevice, pReport);

  fprintf(stderr, "SteamController_HIDGetFeatureReport %02x\n", pReport->featureId);

  for (int i=0; i<50; i++) {
    bool ok = HidD_GetFeature(pDevice->devHandle, pReport, sizeof(SteamController_HIDFeatureReport));
    if (ok) {
      if (featureId == pReport->featureId)
        return true;
      continue;
    }

    fprintf(stderr, "HidD_SetFeature failed. Last error: %08lx\n", GetLastError());
    Sleep(1);
  }

  return false;
}

bool SCAPI SteamController_IsWirelessDongle(const SteamControllerDevice *pDevice) {
  if (!pDevice)
    return false;
  return pDevice->isWireless;
}

uint8_t SteamController_ReadRaw(const SteamControllerDevice *pDevice, uint8_t *buffer, uint8_t maxLen) {
  if (!pDevice)
    return 0;

  DWORD bytesRead = 0;
  ReadFile(pDevice->devHandle, buffer, maxLen, &bytesRead, (OVERLAPPED*)&pDevice->overlapped);
  WaitForSingleObject(pDevice->reportEvent, 0);

  return bytesRead & 0xff;
}

#endif
