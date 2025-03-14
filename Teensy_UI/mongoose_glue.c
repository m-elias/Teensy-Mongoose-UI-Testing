#include <stdint.h>
// SPDX-FileCopyrightText: 2024 Cesanta Software Limited
// SPDX-License-Identifier: GPL-2.0-only or commercial
// Generated by Mongoose Wizard, https://mongoose.ws/wizard/

// Include your device-specific headers, and edit functions below
// #include "hal.h"

#include "mongoose_glue.h"
#include "Arduino.h"

// Dropdown item lists
const char* pwmFreqList = "[\"122 hz - Baraki Valve\",\"490 hz\",\"1000 hz - Danfoss\",\"3921 hz\",\"9210 hz\",\"39210 hz - Motor\"]";
const char* kickoutModeList = "[\"1 - AOG Setting (default)\",\"2 - Quadrature Encoder\",\"3 - JD Variable Duty Encoder\",\"4 - WAS-PWM Ratio\",\"5 - Encoder Speed\"]";
const char* ko_help[6] = {
    "Unknown option/error.",
    "Set AOG to Count/Pressure/Current Sensor according to your setup",
    "Set AOG to \"Count Sensor\". Connect both signals, one to each Kickout Input (Analog & Digital)",
    "Set AOG to \"Pressure Sensor\". Connect to Kickout Analog (Fastest response time)",
    "Set AOG to \"Pressure Sensor\". *Need more instructions*",
    "Set AOG to \"Pressure Sensor\". *Need more instructions*"
};

// added these forward declarations so that btn functions can access API endpoint structs
static struct comms s_comms;
static struct inputs s_inputs;
static struct outputs s_outputs;
static struct misc s_misc;

static void ws_timer_50(void *arg) {    //  || s_misc.update
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  struct mg_connection *c;
  //uint64_t now = mg_millis();

  for (c = mgr->conns; c != NULL; c = c->next) {
    if (c->is_websocket) {
      if (c->send.len > 2048) continue;  // Prevent stale connections to grow infinitely

      s_misc.update = false;
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("steerState"), s_inputs.steerState);
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("workState"), s_inputs.workState);
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("workInput"), s_inputs.workInput);
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("workThres"), s_inputs.workThres);
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("kickoutState"), s_inputs.kickoutState);
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %d}", MG_ESC("kickoutStateHist"), s_inputs.kickoutStateHist);
    }
  }
}

// Update help text in Kickout panel according to option selected in dropdown
static void ws_timer_200(void *arg) {    // || oldKickoutMode != s_inputs.kickoutModeStr[0]
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  struct mg_connection *c;
  //uint64_t now = mg_millis();
  //static uint8_t oldKickoutMode = 0;

  for (c = mgr->conns; c != NULL; c = c->next) {
    if (c->is_websocket) {
      if (c->send.len > 2048) continue;  // Prevent stale connections to grow infinitely

      // check first char of dropdown's selected option to match displayed help text
      uint8_t koSelection = s_inputs.kickoutModeStr[0] - '0';
      const char* ko_help_selected = ko_help[koSelection];

      //MG_INFO((s_inputs.kickoutModeStr));
      mg_ws_printf(c, WEBSOCKET_OP_TEXT, "{%m: %m}", MG_ESC("kickout_dropdown_help"), MG_ESC((ko_help_selected)));
      //oldKickoutMode = s_inputs.kickoutModeStr[0];
    }
  }
}

void glue_init(void) {
  MG_INFO(("Starting websocket timers"));

  mg_timer_add(&g_mgr, 50,        // Call every 50 milliseconds
               MG_TIMER_REPEAT,   // Periodically
               ws_timer_50,   // This function
               &g_mgr             // And pass this parameter to the function
  );

  mg_timer_add(&g_mgr, 200,      // Call every 2000 milliseconds
               MG_TIMER_REPEAT,   // Periodically
               ws_timer_200,       // This function
               &g_mgr             // And pass this parameter to the function
  );

  s_comms.gps2State = 3;    // set as 3 - "Undetected" at boot, UI Wizard defaults to '4' to show all UI elements while designing layout
  //s_comms.ntripState = 3;
  MG_DEBUG(("Custom init done"));
}

static uint64_t s_action_timeout_reboot;  // Time when reboot ends
bool glue_check_reboot(void) {
  return s_action_timeout_reboot > mg_now(); // Return true if reboot is in progress
}
void glue_start_reboot(void) {
  s_action_timeout_reboot = mg_now() + 4000; // This timer doesn't do anything because the Teensy reboots and forgets about the timer
  MG_INFO(("rebooting"));
  SCB_AIRCR = 0x05FA0004; // Teensy reboot
}

static uint64_t s_action_timeout_dec_work_thres;  // Time when dec_work_thres ends
bool glue_check_dec_work_thres(void) {
  return s_action_timeout_dec_work_thres > mg_now(); // Return true if dec_work_thres is in progress
}
void glue_start_dec_work_thres(void) {
  s_action_timeout_dec_work_thres = mg_now() + 100; // Start dec_work_thres, finish after 1 second
  if(s_inputs.workThres > 10) s_inputs.workThres -= 1;
  glue_update_state();
}

static uint64_t s_action_timeout_inc_work_thres;  // Time when inc_work_thres ends
bool glue_check_inc_work_thres(void) {
  return s_action_timeout_inc_work_thres > mg_now(); // Return true if inc_work_thres is in progress
}
void glue_start_inc_work_thres(void) {
  s_action_timeout_inc_work_thres = mg_now() + 100; // Start inc_work_thres, finish after 1 second
  if(s_inputs.workThres < 90) s_inputs.workThres += 1;
  glue_update_state();
}

static uint64_t s_action_timeout_set_work_thres;  // Time when set_work_thres ends
bool glue_check_set_work_thres(void) {
  return s_action_timeout_set_work_thres > mg_now(); // Return true if set_work_thres is in progress
}
void glue_start_set_work_thres(void) {
  s_action_timeout_set_work_thres = mg_now() + 250; // Start set_work_thres, finish after 250ms
  //s_inputs.workThres = min(max(s_inputs.workInput, 10), 90);
  s_inputs.workThres = constrain(s_inputs.workInput, 10, 90);
  glue_update_state();
}

static uint64_t s_action_timeout_set_work_digital;  // Time when set_work_digital ends
bool glue_check_set_work_digital(void) {
  return s_action_timeout_set_work_digital > mg_now(); // Return true if set_work_digital is in progress
}
void glue_start_set_work_digital(void) {
  s_action_timeout_set_work_digital = mg_now() + 250; // Start set_work_digital, finish after 250ms
  s_inputs.workThres = 50;
  memcpy(s_inputs.workHystStr, "18", 2);
  s_inputs.workInvert = true;
  glue_update_state();
}

void *glue_ota_begin_firmware_update(char *file_name, size_t total_size) {
  bool ok = mg_ota_begin(total_size);
  MG_DEBUG(("%s size %lu, ok: %d", file_name, total_size, ok));
  return ok ? (void *) 1 : NULL;
}
bool glue_ota_end_firmware_update(void *context) {
  mg_timer_add(&g_mgr, 500, 0, (void (*)(void *)) mg_ota_end, context);
  return true;
}
bool glue_ota_write_firmware_update(void *context, void *buf, size_t len) {
  MG_DEBUG(("ctx: %p %p/%lu", context, buf, len));
  return mg_ota_write(buf, len);
}

static struct comms s_comms = {3, 3, "115200", "38400", "AgOpenGPS", 3, 3, "921600", "921600", false, "60ms - F9P", 3, 3, "460800", "0d 0h 0m 0s", 12, "**SSID**", "**PW**", true};
void glue_get_comms(struct comms *data) {
  *data = s_comms;  // Sync with your device
}
void glue_set_comms(struct comms *data) {
  s_comms = *data; // Sync with your device
}

static struct inputs s_inputs = {0, 3, 50, true, 50, "18", 18, 3, 2, "1 - AOG Setting (default)"};
void glue_get_inputs(struct inputs *data) {
  *data = s_inputs;  // Sync with your device
}
void glue_set_inputs(struct inputs *data) {
  s_inputs = *data; // Sync with your device
}

void glue_reply_inputsKickoutModeList(struct mg_connection *c, struct mg_http_message *hm) {
  const char *headers = "Cache-Control: no-cache\r\n" "Content-Type: application/json\r\n";
  const char *value = kickoutModeList;
  (void) hm;
  mg_http_reply(c, 200, headers, "%s\n", value);
}
static struct outputs s_outputs = {"shoutld change"};
void glue_get_outputs(struct outputs *data) {
  *data = s_outputs;  // Sync with your device
}
void glue_set_outputs(struct outputs *data) {
  s_outputs = *data; // Sync with your device
}

void glue_reply_outputsPwmFreqList(struct mg_connection *c, struct mg_http_message *hm) {
  const char *headers = "Cache-Control: no-cache\r\n" "Content-Type: application/json\r\n";
  const char *value = pwmFreqList;
  (void) hm;
  mg_http_reply(c, 200, headers, "%s\n", value);
}
static struct misc s_misc = {75, false, "AiO NG v6"};
void glue_get_misc(struct misc *data) {
  *data = s_misc;  // Sync with your device
}
void glue_set_misc(struct misc *data) {
  s_misc = *data; // Sync with your device
}
