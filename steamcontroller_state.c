#include "steamcontroller.h"
#include "common.h"

/**
 * Read the next event from the device.
 * 
 * @param pController   Device to use.
 * @param pEvent        Where to store event data.
 * 
 * @return The type of the received event. If no event was received this is 0.
 */
uint8_t SCAPI SteamController_ReadEvent(const SteamControllerDevice *pDevice, SteamControllerEvent *pEvent) {
  if (!pDevice)
    return 0;

  if (!pEvent)
    return 0;

  uint8_t eventDataBuf[65];
  uint16_t len = SteamController_ReadRaw(pDevice, eventDataBuf, 65);

  if (!len)
    return 0;

  uint8_t *eventData = eventDataBuf;
  if (!*eventData) {
    eventData++;
    len--;
  }

  /*
    Layout of each message seems to be:

    0x0000 01       1 byte    Seems to always be 0x01. Could be an id if multiple controllers are paired with the same dongle. TODO: get second controller
    0x0001 00       1 byte    Seems to always be 0x00. Could also be the high order byte of the other byte.
    0x0002 xx       1 byte    Event type. So far the following values were observed:

                        0x01  Update. Contains axis and button data.
                        0x03  Connection update. Sent when a controller connects or disconnects (turned off/out of range).
                        0x04  Battery update. Sent occasionally to update battery status of controller. Needs to be enabled with STEAMCONTROLLER_CONFIG_FLAG_SEND_BATTERY_STATUS
                        0x05  ??? Seems mostly random. Needs some tinkering with settings, controller disconnects after a while.

    0x0003 xx       1 byte    Unknown. Seems to always be 0x3c (the device type) for event type 1, 
                              0x01 for event type 3 and 0x0b for event type 4. 

    0x0004                    Event type specific data.
  */
  uint8_t eventType = eventData[0x02];
  pEvent->eventType = eventType;
  switch(eventType) {

    case STEAMCONTROLLER_EVENT_UPDATE:
      /*
        0x0004 xx xx xx xx          1 ulong   Appears to be some kind of timestamp or message counter.
        0x0008 xx xx xx             3 bytes   Button data.
        0x000b xx                   1 byte    Left trigger value.
        0x000c xx                   1 byte    Right trigger value.
        0x000d 00 00 00             3 bytes   Padding?
        0x0010 xx xx yy yy          2 sshorts Left position data. If STEAMCONTROLLER_BUTTON_LFINGER is set 
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

    case STEAMCONTROLLER_EVENT_BATTERY:
      /*
        0x0004 xx xx xx xx          1 ulong   Another timestamp or message counter, separate from update event.
        0x0008 00 00 00 00          4 bytes   Padding?                                         
        0x000c xx xx                1 ushort  Battery voltage in millivolts (both cells).
        0x000e 64 00                1 ushort  Unknown. Seems to be stuck at 0x0064 (100 in decimal).
      */
      pEvent->battery.voltage = eventData[0x0c] | (eventData[0x0d] << 8);
      break;

    case STEAMCONTROLLER_EVENT_CONNECTION:
      /*
        0x0004 xx                   1 byte    Connection detail. 0x01 for disconnect, 0x02 for connect, 0x03 for pairing request.
        0x0005 xx xx xx             3 bytes   On disconnect: upper 3 bytes of timestamp of last received update.
      */
      pEvent->connection.details = eventData[4];
      break;

    default:
      fprintf(stderr, "Received unknown event type %02x:\n", eventType);
      Debug_DumpHex(eventData, len);

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
void SCAPI SteamController_UpdateState(SteamControllerState *pState, const SteamControllerEvent *pEvent) { 
  switch(pEvent->eventType) {
    case STEAMCONTROLLER_EVENT_UPDATE:
      pState->timeStamp       = pEvent->update.timeStamp;
      pState->activeButtons   = pEvent->update.buttons;

      pState->leftTrigger     = pEvent->update.leftTrigger;
      pState->rightTrigger    = pEvent->update.rightTrigger;

      if (pEvent->update.buttons & STEAMCONTROLLER_BUTTON_LFINGER) {
        pState->leftPad = pEvent->update.leftXY;
      } else {
        pState->stick       = pEvent->update.leftXY;
        if ((pEvent->update.buttons & STEAMCONTROLLER_FLAG_PAD_STICK) == 0) {
          pState->leftPad.x     = 0;
          pState->leftPad.y     = 0;
        }
      }

      pState->rightPad        = pEvent->update.rightXY;

      pState->acceleration    = pEvent->update.acceleration;
      pState->angularVelocity = pEvent->update.angularVelocity;
      pState->orientation     = pEvent->update.orientation;
      break;

    case STEAMCONTROLLER_EVENT_CONNECTION:
      if (pEvent->connection.details == STEAMCONTROLLER_CONNECTION_EVENT_DISCONNECTED) {
        pState->isConnected = false;
        pState->hasPairingRequest = false;
      } else if (pEvent->connection.details == STEAMCONTROLLER_CONNECTION_EVENT_CONNECTED) {
        pState->isConnected = true;
        pState->hasPairingRequest = false;
      } else if (pEvent->connection.details == STEAMCONTROLLER_CONNECTION_EVENT_PAIRING_REQUESTED) {
        pState->hasPairingRequest =  true;
      } else {
        fprintf(stderr, "Unknown detail id for connection event: %02x\n", pEvent->connection.details);
      }
      break;

    case STEAMCONTROLLER_EVENT_BATTERY:
      pState->batteryVoltage = pEvent->battery.voltage;
      break;

    default:
      fprintf(stderr, "Unknown event type: %02x\n", pEvent->eventType);
      break;
  }
}


