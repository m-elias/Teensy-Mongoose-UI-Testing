#include "core_pins.h"
#include <stdint.h>
#include "elapsedMillis.h"
void checkMiscTimer()
{
  static elapsedMillis miscTimer = 0;
  if (miscTimer > 499) { // 2hz
    miscTimer = 0;
    struct misc misc_temp;
    glue_get_misc(&misc_temp); // pull-sync from UI

    LEDs.setBrightness(int(float(misc_temp.rgbBrightness) / 2.55));

    struct comms comms_temp;
    glue_get_comms(&comms_temp);
    if (esp32ElapsedUpdateTime > 10000 && comms_temp.esp32State < 3) {
      if (esp32ElapsedUpdateTime < 20000) {
        comms_temp.esp32State = 2;  // set as "Timed out"
      } else {
        comms_temp.esp32State = 0;  // set as "Disconnected"
      }
    }
    glue_set_comms(&comms_temp);
    glue_update_state();
  }
}

void USB_Data_Check()
{
  if (Serial.available()) {
    /*uint8_t sRead = */Serial.read();

  }
}

#define SerialIMU   Serial4 // IMU BNO-085 in RVC serial mode
#define SerialRTK   Serial3 // RTK radio
#define SerialGPS1  Serial5 // GPS1 UART (Right F9P, or UM982)
#define SerialGPS2  Serial8 // GPS2 UART (Left F9P)
#define SerialRS232 Serial7 // RS232 UART
#define SerialESP32 Serial2 // ESP32 UART (for ESP32 WiFi Bridge)

const int32_t baudGPS1 = 921600;
const int32_t baudGPS2 = 921600;
const int32_t baudRTK = 115200; // most are using Xbee radios with default of 115200
const int32_t baudRS232 = 38400;

void serial_init()
{
  SerialGPS1.begin(baudGPS1);
  SerialGPS2.begin(baudGPS2);
  SerialRTK.begin(baudRTK);
  SerialRS232.begin(baudRS232);
  SerialIMU.begin(115200);

  SerialESP32.begin(baudESP32);
  SerialESP32.addMemoryForRead(ESP32rxbuffer, sizeof(ESP32rxbuffer));
  SerialESP32.addMemoryForWrite(ESP32txbuffer, sizeof(ESP32txbuffer));

  SerialGPS1.clear();
  SerialGPS2.clear();
  SerialRTK.clear();
  SerialRS232.clear();
  SerialIMU.clear();
  SerialESP32.clear();

  struct comms comms_temp;
  glue_get_comms(&comms_temp);
  //comms_temp.gps2State = 3;    // set as 3 - "Undetected" at boot, UI Wizard defaults to '4' to show all UI elements while designing layout
  glue_set_comms(&comms_temp);
}

void uartDataChecks()
{
  struct comms comms_temp;
  glue_get_comms(&comms_temp);

  static elapsedMillis gps1ValidDataTimer = 0;
  static elapsedMillis gps1MissedDataTimer = 0;
  if (SerialGPS1.available()) {
    comms_temp.gps1State = 1;
    char r = SerialGPS1.read();
    //Serial.print(r);
    if (r == '$') gps1ValidDataTimer = 0;
  }
  if (gps1ValidDataTimer > 150 && gps1ValidDataTimer < 1100 && comms_temp.gps1State < 3) {
    comms_temp.gps1State = 2;
    gps1MissedDataTimer = 0;
  }
  if (gps1ValidDataTimer > 1100 && comms_temp.gps1State < 3) {
    comms_temp.gps1State = 0;
  }


  static elapsedMillis rtkValidDataTimer = 0;
  static elapsedMillis rtkMissedDataTimer = 0;
  if (SerialRTK.available()) {
    if (comms_temp.rtkState != 2 || rtkMissedDataTimer > 3000){
      comms_temp.rtkState = 1;
      rtkMissedDataTimer = 0;
    }
    SerialRTK.read();
    rtkValidDataTimer = 0;
  }
  if (rtkValidDataTimer > 2500 && rtkValidDataTimer < 11000 && comms_temp.rtkState < 3) {
    comms_temp.rtkState = 2;
    rtkMissedDataTimer = 0;
  }
  if (rtkValidDataTimer > 11000 && comms_temp.rtkState < 3) {
    if (comms_temp.rtkState != 2 || rtkMissedDataTimer > 3000) comms_temp.rtkState = 0;
  }


  static uint32_t imuValidDataTimer = 0;
  static uint32_t imuMissedDataTimer = 0;
  static int prevImuState = -1;
  if (prevImuState != comms_temp.imuState) {
    Serial.printf("%i: %i>%i\r\n", millis(), prevImuState, comms_temp.imuState);
    prevImuState = comms_temp.imuState;
  }
  if (!strcmp(comms_temp.imuBaud, "DISABLED")) { // IMU input is disabled by user or system
    if (comms_temp.imuState != 4) {
      comms_temp.imuState = 4;
      SerialIMU.end();
      SerialIMU.clear();
      Serial.print("IMU stop: ");
      Serial.println(comms_temp.imuState);
    }
  } else {  // IMU is not disabled
    if (comms_temp.imuState == 4) { // if IMU was previously disabled
      SerialIMU.clear();
      SerialIMU.begin(atoi(comms_temp.imuBaud));
      comms_temp.imuState = 3;
      Serial.print("IMU begin: ");
      Serial.println(atoi(comms_temp.imuBaud));
    }
    if (SerialIMU.available() && comms_temp.imuState < 4) {
      if (comms_temp.imuState != 2 || millis() - imuMissedDataTimer > 5000) {
        comms_temp.imuState = 1;
        imuMissedDataTimer = millis();
      }
      SerialIMU.read();
      /*Serial.print(millis()); Serial.print(": ");
      Serial.print(millis() - imuValidDataTimer); Serial.print(" <> "); Serial.println(millis() - imuMissedDataTimer);*/
      imuValidDataTimer = millis();
    }
    if (millis() - imuValidDataTimer > 15 && millis() - imuValidDataTimer < 110 && comms_temp.imuState < 3) {
      comms_temp.imuState = 2;
      imuMissedDataTimer = millis();
    }
    if (millis() - imuValidDataTimer > 110 && comms_temp.imuState < 3) {
      if (comms_temp.imuState != 2 || millis() - imuMissedDataTimer > 5000) comms_temp.imuState = 0;
    }
  }
  

  glue_set_comms(&comms_temp);
  glue_update_state();
}