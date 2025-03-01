// SPDX-FileCopyrightText: 2024 Cesanta Software Limited
// SPDX-License-Identifier: GPL-2.0-only or commercial
// Generated by Mongoose Wizard, https://mongoose.ws/wizard/

#ifndef MONGOOSE_GLUE_H
#define MONGOOSE_GLUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mongoose.h"

#define WIZARD_ENABLE_HTTP 1
#define WIZARD_ENABLE_HTTPS 0
#define WIZARD_ENABLE_HTTP_UI 1
#define WIZARD_ENABLE_HTTP_UI_LOGIN 0

#define WIZARD_ENABLE_WEBSOCKET 1
#define WIZARD_WEBSOCKET_TIMER_MS 50

#define WIZARD_ENABLE_MQTT 0
#define WIZARD_MQTT_URL ""

#define WIZARD_ENABLE_SNTP 0  // Enable time sync.
#define WIZARD_SNTP_TYPE 0    // 0: default Google, 1: DHCP, 2: custom
#define WIZARD_SNTP_URL "udp://time.google.com:123"  // Custom SNTP server URL
#define WIZARD_SNTP_INTERVAL_SECONDS 3600            // Frequency of SNTP syncs

#define WIZARD_DNS_TYPE 0  // 0: default Google, 1: DHCP, 2: custom
#define WIZARD_DNS_URL "udp://8.8.8.8:53"  // Custom DNS server URL
#define WIZARD_CAPTIVE_PORTAL 0

#define WIZARD_ENABLE_MODBUS 0
#define WIZARD_MODBUS_PORT 502

#ifndef WIZARD_REBOOT_TIMEOUT_MS
#define WIZARD_REBOOT_TIMEOUT_MS 500
#endif

void mongoose_init(void);    // Initialise Mongoose
void mongoose_poll(void);    // Poll Mongoose
extern struct mg_mgr g_mgr;  // Mongoose event manager
void glue_init(void);        // Called at the end of mongoose_init()

#define run_mongoose() \
  do {                 \
    mongoose_init();   \
    for (;;) {         \
      mongoose_poll(); \
    }                  \
  } while (0)

#if WIZARD_ENABLE_MQTT
void glue_lock_init(void);  // Initialise global Mongoose mutex
void glue_lock(void);       // Lock global Mongoose mutex
void glue_unlock(void);     // Unlock global Mongoose mutex
#else
#define glue_lock_init()
#define glue_lock()
#define glue_unlock()
#endif

// Increment device change state counter - trigger UI refresh
void glue_update_state(void);

// Firmware Glue


void glue_websocket_on_timer(struct mg_connection *c);

bool glue_check_reboot(void);
void glue_start_reboot(void);

bool glue_check_dec_work_thres(void);
void glue_start_dec_work_thres(void);

bool glue_check_inc_work_thres(void);
void glue_start_inc_work_thres(void);

bool glue_check_set_work_thres(void);
void glue_start_set_work_thres(void);

bool glue_check_set_work_digital(void);
void glue_start_set_work_digital(void);

void *glue_ota_begin_firmware_update(char *file_name, size_t total_size);
bool glue_ota_end_firmware_update(void *context);
bool glue_ota_write_firmware_update(void *context, void *buf, size_t len);

struct comms {
  char gpsSync[15];
  bool gpsPass;
  int esp32Detected;
  char esp32Runtime[20];
  int esp32NumClients;
  char esp32SSID[24];
  char esp32PW[24];
  bool esp32BridgeEnabled;
};
void glue_get_comms(struct comms *);
void glue_set_comms(struct comms *);

struct inputs {
  int steerEnabled;
  bool steerState;
  int workEnabled;
  bool workState;
  int workInput;
  bool workInvert;
  int workThres;
  char workHystStr[3];
  int workHystVal;
  int kickoutEnabled;
  bool kickoutState;
  int kickoutStateHist;
  char kickoutModeStr[30];
};
void glue_get_inputs(struct inputs *);
void glue_set_inputs(struct inputs *);

void glue_reply_inputsKickoutModeList(struct mg_connection *, struct mg_http_message *);
struct outputs {
  char drv8701PwmFreq[20];
};
void glue_get_outputs(struct outputs *);
void glue_set_outputs(struct outputs *);

void glue_reply_outputsPwmFreqList(struct mg_connection *, struct mg_http_message *);
struct misc {
  int rgbBrightness;
  bool update;
  char fversion[50];
};
void glue_get_misc(struct misc *);
void glue_set_misc(struct misc *);


#ifdef __cplusplus
}
#endif
#endif  // MONGOOSE_GLUE_H
