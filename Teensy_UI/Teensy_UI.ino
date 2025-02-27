#include "mongoose_glue.h"
#include <EEPROM.h>
#include <Streaming.h>
#include "mongoose_init.h"

const char* inoVersion = "AiO-NG-v6 Web GUI - " __DATE__ " " __TIME__;

// globally available, working settings struct
//  - read/write from/to this struct
//  - call glue_get_inputs(&input_vars); before making changes
//  - call glue_set_inputs(&input_vars); after making changes
struct inputs input_vars;
struct misc misc_vars;

const uint16_t EE_IDENT = 2417; // change to force EE update
const uint16_t eeAddr = 0;

#include "inputs.h"

#include "LEDS.h"
LEDS LEDs = LEDS(1000, 255, 64, 127);   // 1000ms RGB update, 255/64/127 RGB brightness balance levels for v5.0a

#define SerialESP32 Serial2
const int32_t baudESP32 = 460800;
uint8_t ESP32rxbuffer[256];   // don't know what size is needed
uint8_t ESP32txbuffer[256];   // don't know what size is needed
uint32_t esp32Runtime;
elapsedMillis esp32ElapsedUpdateTime;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  //while (!Serial) delay(50);
  Serial.print("\r\n\n\n*********************\r\nStarting setup...\r\n");
  Serial.println(inoVersion);

  //MG_ERROR(("Hello1"));
  //MG_INFO(("Hello2"));
  //MG_DEBUG(("Hello3"));   // doesn't print in setup() with default debug settings
  //MG_VERBOSE(("Hello4"));  // doesn't print in setup() with default debug settings

  ethernet_init();
  mongoose_init();
  //mg_set_ip();  // causes Teensy to hang

  uint16_t eeIdentRead;
  EEPROM.get(eeAddr + 0, eeIdentRead);  // get eeprom ident, for checking if eeprom needs resetting
  uint16_t varSize = 0;
  EEPROM.get(eeAddr + 2, varSize);      // get size of struct, if Mongoose wizard has changed struct, reset to new defaults

  // if EE Ident OR the struct size does not match then overwrite with new defaults
  if (eeIdentRead != EE_IDENT || varSize != sizeof(input_vars)) {
    glue_get_inputs(&input_vars);
    EEPROM.put(eeAddr + 0, EE_IDENT);   // EEPROM.put() uses EEPROM.update() which only writes to EEPROM if the value !=
    EEPROM.put(eeAddr + 2, sizeof(input_vars));
    EEPROM.put(eeAddr + 4, input_vars);
    Serial.print("\r\n** EEPROM reset to new defaults **\r\n");
  } else {
    EEPROM.get(eeAddr + 4, input_vars);  // read the Settings saved in EEPROM to the global Settings struct
    Serial.print("Loaded settings from EEPROM\r\n");
    glue_set_inputs(&input_vars);
  }

  glue_get_misc(&misc_vars);
  strcpy(misc_vars.fversion, inoVersion);
  glue_set_misc(&misc_vars);

  initializeInputs();
  LEDs.init();
  LEDs.set(LED_ID::PWR_ETH, PWR_ETH_STATE::PWR_ON);
  LEDs.set(LED_ID::PWR_ETH, PWR_ETH_STATE::ETH_READY);

  SerialESP32.begin(baudESP32);
  SerialESP32.addMemoryForRead(ESP32rxbuffer, sizeof(ESP32rxbuffer));
  SerialESP32.addMemoryForWrite(ESP32txbuffer, sizeof(ESP32txbuffer));

}

void loop() {
  mongoose_poll();
  LEDs.updateLoop();


  // check for ESP32 serial data
  if (SerialESP32.available())
  {
    static uint8_t incomingBytes[50];
    static uint8_t incomingIndex;
    incomingBytes[incomingIndex] = SerialESP32.read();
    incomingIndex++;
    //Serial.print("\r\nindex: "); Serial.print(incomingIndex);
    //Serial.print(" ");
    //for (byte i = 0; i < incomingIndex; i++) {
      //Serial.print(incomingBytes[i]);
      //Serial.print(" ");
    //}

    if (incomingBytes[incomingIndex - 2] == 13 && incomingBytes[incomingIndex - 1] == 10)
    {
      if (incomingBytes[0] == 128 && incomingBytes[1] == 129)
      {
        /*for (byte i = 0; i < incomingIndex; i++) {
          Serial.print(incomingBytes[i]);
          Serial.print(" ");
        }
        Serial.println();*/

        // Process the special ESP32->Teensy PGNs
        if (incomingBytes[2] == 90 && incomingBytes[3] == 90) { // helloFromESP32 PGN
          union {   // both variables in the union share the same memory space
            byte array[4];
            uint32_t millis;
          } runtime;
          runtime.array[0] = incomingBytes[5];
          runtime.array[1] = incomingBytes[6];
          runtime.array[2] = incomingBytes[7];
          runtime.array[3] = incomingBytes[8];
          esp32Runtime = runtime.millis;
          uint8_t esp32NumClients = incomingBytes[9];
          uint16_t esp32m2mPort = (incomingBytes[11] << 8) + incomingBytes[10];
          //Serial.printf("M2M Port: %i\r\n", esp32m2mPort);

          struct comms comms_vars;
          glue_get_comms(&comms_vars);
          comms_vars.esp32Detected = 1;

          esp32Runtime /= 1000;
          
          uint8_t days = esp32Runtime / 86400;
          esp32Runtime %= 86400;
          
          uint8_t hrs = esp32Runtime / 3600;
          esp32Runtime %= 3600;
          
          uint8_t mins = esp32Runtime / 60;
          esp32Runtime %= 60;
          
          uint8_t secs = esp32Runtime;
          
          char esp32RuntimeStr[20];
          sprintf(esp32RuntimeStr, "%id %ih %im %is", days, hrs, mins, secs);
          //Serial.printf("ESP32 Runtime: %s\r\n", esp32RuntimeStr);
          strcpy(comms_vars.esp32Runtime, esp32RuntimeStr);
          
          comms_vars.esp32NumClients = esp32NumClients;
          glue_set_comms(&comms_vars);
          esp32ElapsedUpdateTime = 0;
        }
        
        else if (incomingBytes[2] == 91 && incomingBytes[3] == 91) { // ESP32 WiFi Creds PGN
          //Serial.println("WiFi Creds PGN");
          struct comms comms_vars;
          glue_get_comms(&comms_vars);

          char str[24];
          memset(str,'\0', sizeof(str));  //init to all 0, might not be necessary
          for (uint8_t i = 0; i < 24 && incomingBytes[i+5] != 0; i++) {
            str[i] = incomingBytes[i+5];  // only copy non 0 chars, 0 denotes end of char strings
          }
          //Serial.println(str);
          memcpy(comms_vars.esp32SSID, str, 24);    // copy to UI string

          memset(str,'\0', sizeof(str));  // zero out and repeat for password
          for (uint8_t i = 0; i < 24 && incomingBytes[i+5+24] != 0; i++) {
            str[i] = incomingBytes[i+5+24];
          }
          if (str[0] == 0) memcpy(str, "**blank**\0", 10);  // detect blank/empty password and indicate accordingly
          //Serial.println(str);
          memcpy(comms_vars.esp32PW, str, 24);

          glue_set_comms(&comms_vars);


        // all other properly formed PGNs forwarded to AgIO
        } else {

          // Modules--Wifi:9999-->ESP32--serial-->Teensy--ethernet:9999-->AgIO
          //UDP.SendUdpByte(incomingBytes, incomingIndex - 2, UDP.broadcastIP, UDP.portAgIO_9999);

          //pass data to USB for debug
          //Serial.print("\r\nE32-s->T41-e:9999->AgIO ");
          //for (byte i = 0; i < incomingIndex - 2; i++) {
            //Serial.print(incomingBytes[i]);
            //Serial.print(" ");
          //}
          //Serial.print((String)" (" + SerialESP32->available() + ")");
        }

      // unformed/corrupt PGN detected
      } else {
        Serial.print("\r\n\nCR/LF detected but [0]/[1] bytes != 128/129\r\n");
        for (byte i = 0; i < incomingIndex; i++) {
          Serial.print(incomingBytes[i]);
          Serial.print(" ");
        }
        Serial.println();
      }
      incomingIndex = 0;
    }
  }



  static elapsedMillis miscTimer = 0;
  if (miscTimer > 499) { // 2hz
    miscTimer -= 500;   // try to maintain steady 2hz
    glue_get_misc(&misc_vars); // pull-sync from UI

    LEDs.setBrightness(int(float(misc_vars.rgbBrightness) / 2.55));

    if (esp32ElapsedUpdateTime > 15000) {
      struct comms comms_vars;
      glue_get_comms(&comms_vars);
      if (comms_vars.esp32Detected == 1) comms_vars.esp32Detected = 2;
      glue_set_comms(&comms_vars);
    }
  }


  static elapsedMillis inputsTimer = 0;
  if (inputsTimer > 99) { // 10hz
    inputsTimer -= 100;   // try to maintain steady 10hz
    glue_get_inputs(&input_vars); // pull-sync from UI

    // **** read analog WORK input ****
    float read = float(analogRead(WAS_A_PIN)) / 10.24; // convert to 0-100 percent (/1024*100)
    
    if (read < 3.0 || read > 97.0) {  // detect disconnected sensor
      input_vars.workEnabled = 0;
    } else {
      input_vars.workEnabled = 1;
    }
    //Serial.printf("Steer Enabled: %i\r\n", input_vars.steerEnabled);

    if (input_vars.workInvert) read = 100 - read;  // option to invert input polarity

    input_vars.workHystVal = atoi(input_vars.workHystStr);
    static bool workState = 0;
    if (read < max(input_vars.workThres - input_vars.workHystVal, 10)) {  // limit to >10% (0.33V)
      workState = LOW;
    }
    if (read > min(input_vars.workThres + input_vars.workHystVal, 90)) { // limit to <90% (2.97V)
      workState = HIGH;
    }

    input_vars.workState = workState;
    //input_vars.work_state = (!workState != !input_vars.work_invert);  // 2nd option to invert, XOR the work state logic with invert setting
    
    input_vars.workInput = max(min(round(read), 100), 0);  // limit betwwen 0 - 100

    // **** read digital STEER input ****
    input_vars.steerState = !digitalRead(STEER_PIN);
    

    // **** read digital KICKOUT input ****
    static elapsedMillis kickoutTransitionTimer = 0;
    if (input_vars.kickoutState == digitalRead(KICKOUT_D_PIN)){ // detect change
      kickoutTransitionTimer = 0;
      //memcpy(input_vars.kickoutStateColor, "#FFA500", 7); // set temporary orange color
      input_vars.kickoutStateHist = 2;  // set temp orange color
      input_vars.kickoutState = !input_vars.kickoutState;
    }

    if (kickoutTransitionTimer > 2000) {
      input_vars.kickoutStateHist = input_vars.kickoutState;
      /*if (input_vars.kickoutState == HIGH) {
        memcpy(input_vars.kickoutStateColor, "#22c55e", 7);
      } else {
        memcpy(input_vars.kickoutStateColor, "#ef4444", 7);
      }*/
    }

    glue_set_inputs(&input_vars); // push-sync to UI
    glue_update_state();  // triggers UI update, max 1hz regardless of Wizard setting?
  }



  static uint64_t timer;
  if (mg_timer_expired(&timer, 500, mg_millis())) {        // Every 500ms
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // blink an LED
  }




  if (Serial.available()) {
    uint8_t sRead = Serial.read();

    if (sRead == 's') {   // toggle Steer input UI elements visible/hidden
      glue_get_inputs(&input_vars); // pull-sync from UI
      input_vars.steerEnabled = !input_vars.steerEnabled;
      glue_set_inputs(&input_vars); // push-sync to UI
    }

    if (sRead == 'k') {   // toggle Kickout input UI elements visible/hidden
      glue_get_inputs(&input_vars); // pull-sync from UI
      input_vars.kickoutEnabled = !input_vars.kickoutEnabled;
      glue_set_inputs(&input_vars); // push-sync to UI
    }

    if (sRead == 'w') {   // toggle Work input UI elements visible/hidden
      glue_get_inputs(&input_vars); // pull-sync from UI
      input_vars.workEnabled = !input_vars.workEnabled;
      glue_set_inputs(&input_vars); // push-sync to UI
    }

    if (sRead == 'i') {
      glue_get_inputs(&input_vars); // pull-sync from UI
      input_vars.workInvert = !input_vars.workInvert;
      glue_set_inputs(&input_vars); // push-sync to UI
    }

    if (sRead == 'h') {
      glue_get_inputs(&input_vars); // pull-sync from UI
      memcpy(input_vars.workHystStr, "6", 2);
      glue_set_inputs(&input_vars); // push-sync to UI
    }

    if (sRead == 'd') {
      glue_start_set_work_digital();
    }
  }
}
