/*
  
*/

#include "mongoose_glue.h"
#include <EEPROM.h>
#include <Streaming.h>
#include "HardwareSerial.h"
#include "mongoose_init.h"

const char* inoVersion = "AiO-NG-v6 " __DATE__ " " __TIME__;

// globally available, working settings struct
//  - read/write from/to this struct
//  - call glue_get_inputs(&input_vars); before making changes
//  - call glue_set_inputs(&input_vars); after making changes


const uint16_t EE_IDENT = 2417; // change to force EE update
const uint16_t eeAddr = 0;

#include "LEDS.h"
LEDS LEDs = LEDS(1000, 255, 64, 127);   // 1000ms RGB update, 255/64/127 RGB brightness balance levels for v5.0a
#include "inputs.h"
#include "esp32.h"
#include "misc.h"

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
  serial_init();

  uint16_t eeIdentRead;
  EEPROM.get(eeAddr + 0, eeIdentRead);  // get eeprom ident, for checking if eeprom needs resetting
  uint16_t varSize = 0;
  EEPROM.get(eeAddr + 2, varSize);      // get size of struct, if Mongoose wizard has changed struct, reset to new defaults

  struct inputs input_vars;
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

  struct misc misc_vars;
  glue_get_misc(&misc_vars);
  strcpy(misc_vars.fversion, inoVersion);
  glue_set_misc(&misc_vars);

  initializeInputs();
  LEDs.init();
  LEDs.set(LED_ID::PWR_ETH, PWR_ETH_STATE::PWR_ON);
  LEDs.set(LED_ID::PWR_ETH, PWR_ETH_STATE::ETH_READY);
  LEDs.set(LED_ID::STEER, STEER_STATE::WAS_ERROR);

  Serial.print("F_CPU: "); Serial.print(F_CPU/1e6);  Serial.println(" MHz."); 
  //Serial.print("ADC_F_BUS: "); Serial.print(ADC_F_BUS/1e6); Serial.println(" MHz.");  // needs ADC library?
}

void loop() {
  mongoose_poll();
  LEDs.updateLoop();
  esp32DataCheck(); // check for ESP32 serial data
  USB_Data_Check();
  checkInputsTimer();
  checkMiscTimer();
  uartDataChecks();


  static uint64_t timer;
  if (mg_timer_expired(&timer, 500, mg_millis())) {        // Every 500ms
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // blink an LED
  }

}
