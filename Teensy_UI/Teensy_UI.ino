#include "mongoose_glue.h"
#include <EEPROM.h>
#include <Streaming.h>

// Networking variables
struct NetConfigStruct
{
  //static const uint8_t defaultIP[5] = {192, 168, 5, 126};
  uint8_t currentIP[5] = {192, 168, 5, 126};
  uint8_t gatewayIP[5] = {192, 168, 5, 1};
  uint8_t broadcastIP[5] = {192, 168, 5, 255};
};
NetConfigStruct const defaultNet;
NetConfigStruct netConfig = defaultNet;

#include "mongoose_init.h"

// globally available, working settings struct
//  - read/write from/to this struct
//  - call glue_get_settings(&settings_global); before making changes
//  - call glue_set_settings(&settings_global); after making changes
struct settings settings_global = {
  192,    // int bd_ip1
  168,    // int bd_ip2
  5,      // int bd_ip3
  126,    // int bd_ip4
  1,      // int gps_type
  false,  // bool gps_pass
  false,  // bool steer_state
  false,  // bool work_state
  0,      // int work_input
  50,     // int work_thres
  5,      // int work_hyst
  false,  // bool work_invert
  2,      // ui refresh rate
  "AiO GUI v5.12"  // char fversion[40]
};

const uint8_t WORK_HYST_LOOKUP[6] = {1,2,3,5,8,12};
const uint8_t UI_UPDATE_RATE_LOOKUP[4] = {1,2,4,8};

const uint16_t EE_IDENT = 2405;

#define WAS_PIN         A15     // WAS input
#define STEER_PIN         2     // STEER input
#define WORK_PIN        A17     // WORK input
#define KICKOUT_D_PIN     3     // REMOTE input
#define CURRENT_PIN     A13     // CURRENT sense input from on board DRV8701
#define KICKOUT_A_PIN   A12     // PRESSURE input

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(WORK_PIN, INPUT_DISABLE);
  Serial.begin(115200);
  //while (!Serial) delay(50);
  Serial.print("\r\n\n\n*********************\r\nStarting setup...\r\n");

  //MG_ERROR(("Hello1"));
  //MG_INFO(("Hello2"));
  //MG_DEBUG(("Hello3"));   // doesn't print in setup() with default debug settings
  //MG_VERBOSE(("Hello4"));  // doesn't print in setup() with default debug settings

  ethernet_init();
  mongoose_init();
  //mg_set_ip();  // causes Teensy to hang

  uint16_t eeIdentRead = EE_IDENT;
  uint16_t eeAddr = 0;
  EEPROM.get(eeAddr + 0, eeIdentRead);

  if (eeIdentRead != EE_IDENT) {  // if value in eeprom does not match, overwrite with defaults
    EEPROM.put(eeAddr + 0, EE_IDENT);
    EEPROM.put(eeAddr + 2, settings_global);
    Serial.print("\r\n- ** EEPROM version mismatch, reset to defaults! **\r\n");
  } else {
    EEPROM.get(eeAddr + 2, settings_global);  // read the Settings saved in EEPROM to the global Settings struct
    Serial.print("\r\n- loaded settings from EEPROM\r\n");
  }
  glue_set_settings(&settings_global);

  long num = random();
  Serial.println((num));
  Serial.println(u_int(num));
  //Serial.println(byte(num));
  //Serial.println(u_long(0xFFFFFFFF));
  Serial.println(float(num) / 0xFFFFFFFF, 3);

}

/*
  data->steer_state = !data->steer_state;
  data->work_input = random() / 42949673; // 0xFFFFFFFF ("long" random() full scale)
  if (data->work_input > data->work_thres + data->work_hyst) data->work_state = true;
  if (data->work_input < data->work_thres - data->work_hyst) data->work_state = false;
*/


void loop() {
  mongoose_poll();

  static elapsedMillis workTimer = 0;
  if (workTimer > 200) {
    workTimer = 0;
    float read = float(analogRead(WAS_PIN)) / 10.24; // convert to 0-100 percent (/1024*100)

    glue_get_settings(&settings_global);

    if (read < settings_global.work_thres - WORK_HYST_LOOKUP[settings_global.work_hyst]) {
      settings_global.work_state = LOW;
      if (settings_global.work_invert) settings_global.work_state = !settings_global.work_state;
    }
    if (read > settings_global.work_thres + WORK_HYST_LOOKUP[settings_global.work_hyst]) {
      settings_global.work_state = HIGH;
      if (settings_global.work_invert) settings_global.work_state = !settings_global.work_state;
    }
    settings_global.work_input = round(read);
    glue_set_settings(&settings_global);

    

    static uint8_t updateState = 0;
    updateState++;
    if (updateState >= UI_UPDATE_RATE_LOOKUP[settings_global.ui_refresh_rate]) {
      glue_update_state();
      Serial << "\r\n" << millis() << " ui update (" << UI_UPDATE_RATE_LOOKUP[settings_global.ui_refresh_rate] << ")";
      updateState = 0;
    }

    /*static elapsedMillis glueUpdateStateTimer = 0;
    if (glueUpdateStateTimer > 999) {
      glueUpdateStateTimer = 0;
      glue_update_state();
    }*/

    /*Serial << "\r\n" << read << " " << settings_global.work_thres << " " << WORK_HYST_LOOKUP[settings_global.work_hyst] << " "
      << settings_global.work_input << " " << settings_global.work_state << " " << settings_global.work_invert;

    Serial.printf("{%s: %u}", "work_input", settings_global.work_input);
    Serial.printf("{%s: %u}", "work_state", settings_global.work_state);
    */
  }


  static uint64_t timer;
  if (mg_timer_expired(&timer, 500, mg_millis())) {        // Every 500ms
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // blink an LED
  }

}
