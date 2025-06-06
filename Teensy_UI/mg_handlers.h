#include <string.h>
#include "avr/pgmspace.h"

void checkUartDevice(char *_baud, int *_state) {
  if (!strcmp(_baud, "DISABLED"))
    *_state = 4;
  else if (*_state == 4)
    *_state = 3;
}

void GUI_maintenance()
{
  struct comms comms_temp;
  glue_get_comms(&comms_temp);
  checkUartDevice(comms_temp.gps1Baud, &comms_temp.gps1State);
  checkUartDevice(comms_temp.gps2Baud, &comms_temp.gps2State);
  //checkUartDevice(comms_temp.imuBaud, &comms_temp.imuState);
  checkUartDevice(comms_temp.rs232Baud, &comms_temp.rs232State);
  checkUartDevice(comms_temp.keyaBaud, &comms_temp.keyaState);
  checkUartDevice(comms_temp.rtkBaud, &comms_temp.rtkState);
  checkUartDevice(comms_temp.esp32Baud, &comms_temp.esp32State);
  glue_set_comms(&comms_temp);

  glue_update_state();
}



// ******************* Websocket callback functions *************************

static void ws_50(struct mg_connection *c) {    //  || s_misc.update
  //s_misc.update = false;
  struct inputs inputs_temp;
  glue_get_inputs(&inputs_temp);
  mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("steerState"), inputs_temp.steerState);
  mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("workState"), inputs_temp.workState);
  mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("workInput"), inputs_temp.workInput);
  mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("workThres"), inputs_temp.workThres);
  mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("kickoutState"), inputs_temp.kickoutState);
  mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("kickoutStateHist"), inputs_temp.kickoutStateHist);
}

// Update help text in Kickout panel according to option selected in dropdown
const char* ko_help[6] = {
  "Unknown option/error.",
  "Set AOG to Count/Pressure/Current Sensor according to your setup",
  "Set AOG to \"Count Sensor\". Connect both signals, one to each Kickout Input (Analog & Digital)",
  "Set AOG to \"Pressure Sensor\". Connect to Kickout Analog (Fastest response time)",
  "Set AOG to \"Pressure Sensor\". *Need more instructions*",
  "Set AOG to \"Pressure Sensor\". *Need more instructions*"
};

// Update help text in Outputs-Section/Machine panel according to option selected in dropdown
const char* outputs_help[4] = {
  "Unknown option/error.",
  "(6) HIGH or LOW outputs",
  "(3) pairs of Bidirectional DC Motor outputs",
  "(3) pairs of Three Wire Ball Valve outputs"
};

static void ws_200(struct mg_connection *c) {    // || oldKickoutMode != inputs_temp.kickoutModeStr[0]
  //static uint8_t oldKickoutMode = 0;
  struct inputs inputs_temp;
  glue_get_inputs(&inputs_temp);
  // check first char of dropdown's selected option to match displayed help text
  uint8_t koSelection = inputs_temp.kickoutModeStr[0] - '0';
  const char* ko_help_selected = ko_help[koSelection];
  //MG_INFO((inputs_temp.kickoutModeStr));
  mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %m}", MG_ESC("kickout_dropdown_help"), MG_ESC((ko_help_selected)));
  //oldKickoutMode = inputs_temp.kickoutModeStr[0];


  // if no change, skip sending WS update?
  // or restructure, https://discord.com/channels/1194962781745709128/1194962781745709131/1353767399526109304
  struct outputs outputs_temp;
  glue_get_outputs(&outputs_temp);
  uint8_t outSelection = outputs_temp.outputsModeStr[0] - '0';
  const char* outputs_help_selected = outputs_help[outSelection];
  mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %m}", MG_ESC("outputs_dropdown_help"), MG_ESC((outputs_help_selected)));
}


// *********** API Endpoint callback functions *********************

static uint64_t timeout_reboot;  // Time when reboot ends
bool check_reboot(void) {
  return timeout_reboot > mg_now(); // Return true if reboot is in progress
}
void start_reboot(void) {
  timeout_reboot = mg_now() + 1000; // time here doesn't matter because Teensy forgets all when rebooting
  MG_INFO(("rebooting"));
  SCB_AIRCR = 0x05FA0004; // Teensy reboot
}

//static uint64_t timeout_dec_work_thres;  // Time when dec_work_thres ends
bool check_dec_work_thres(void) {
  //return timeout_dec_work_thres > mg_now(); // Return true if dec_work_thres is in progress
  return false;
}
void start_dec_work_thres(void) {
  //timeout_dec_work_thres = mg_now() + 100; // Start dec_work_thres, finish after 0.1 second
  struct inputs inputs_temp;
  glue_get_inputs(&inputs_temp);
  if(inputs_temp.workThres > 10) inputs_temp.workThres -= 1;
  glue_set_inputs(&inputs_temp);
  glue_update_state();
}

//static uint64_t timeout_inc_work_thres;  // Time when inc_work_thres ends
bool check_inc_work_thres(void) {
  //return timeout_inc_work_thres > mg_now(); // Return true if inc_work_thres is in progress
  return false;
}
void start_inc_work_thres(void) {
  //timeout_inc_work_thres = mg_now() + 100; // Start inc_work_thres, finish after 0.1 second
  struct inputs inputs_temp;
  glue_get_inputs(&inputs_temp);
  if(inputs_temp.workThres < 90) inputs_temp.workThres += 1;
  glue_set_inputs(&inputs_temp);
  glue_update_state();
}

//static uint64_t timeout_set_work_thres;  // Time when set_work_thres ends
bool check_set_work_thres(void) {
  //return timeout_set_work_thres > mg_now(); // Return true if set_work_thres is in progress
  return false;
}
void start_set_work_thres(void) {
  //timeout_set_work_thres = mg_now() + 500; // Start set_work_thres, finish after 0.5 second
  struct inputs inputs_temp;
  glue_get_inputs(&inputs_temp);
  inputs_temp.workThres = constrain(inputs_temp.workInput, 10, 90);
  glue_set_inputs(&inputs_temp);
  glue_update_state();
}

//static uint64_t timeout_set_work_digital;  // Time when set_work_digital ends
bool check_set_work_digital(void) {
  //return timeout_set_work_digital > mg_now(); // Return true if set_work_digital is in progress
  return false;
}
void start_set_work_digital(void) {
  //timeout_set_work_digital = mg_now() + 500; // Start set_work_digital, finish after 0.5 second
  struct inputs inputs_temp;
  glue_get_inputs(&inputs_temp);
  inputs_temp.workThres = 50;
  memcpy(inputs_temp.workHystStr, "18", 2);
  inputs_temp.workHystVal = atoi(inputs_temp.workHystStr);
  inputs_temp.workInvert = true;
  glue_set_inputs(&inputs_temp);
  glue_update_state();
}

