#include "steamcontroller.h"

#include <stdio.h>

int main() {
  SteamControllerDeviceEnum *pEnum = SteamController_EnumControllerDevices();
  while (pEnum) {
    SteamControllerEvent event;
    SteamControllerDevice *pDevice = SteamController_Open(pEnum);
    if (pDevice) {  
       
      for(;;) {
        uint8_t res = SteamController_ReadEvent(pDevice, &event);
        if (res == STEAMCONTROLLER_EVENT_CONNECTION && event.connection.details == STEAMCONTROLLER_CONNECTION_EVENT_DISCONNECTED) {
          fprintf(stderr, "Device %p is not connected (anymore), trying next one...\n", pDevice);
          break;
        }

        if (res == STEAMCONTROLLER_EVENT_CONNECTION && event.connection.details == STEAMCONTROLLER_CONNECTION_EVENT_CONNECTED) {
          fprintf(stderr, "Device %p is connected, configuring...\n", pDevice);
          SteamController_Configure(pDevice, 0);
        }

        if (res == 1) {
          // just print the value of the left touch pad / stick position
          fprintf(stderr, "% 6hd % 6hd\n", event.update.leftXY.x, event.update.leftXY.y);  
        }
      }

      SteamController_Close(pDevice);
    }
    pEnum = SteamController_NextControllerDevice(pEnum);
  }
  return 0;
}
