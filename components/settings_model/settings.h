#pragma once

#include <stdbool.h>
#include "nvs_flash.h"

//+1 for null terminator
#define SIZE_OF_AUTH_KEY 4 + 1
#define SIZE_OF_SERIAL_NUMBER 10 + 1
#define SIZE_OF_PASSWORD 10 + 1

typedef enum
{
  NO_TRIGGER,
  IR_SENSOR,
  MOTION_SENSOR,
  IR_MOTION_SENSOR
} TRIGGER;
// TODO: LATER: Timed trigger or timed continuous scan from 8AM to 5pm such and such from json array of struct to store the periods

typedef enum
{
  SCAN_ONCE,
  SCAN_CONTINUOUS,
  SCAN_OFF,
} SCAN_MODES;

typedef enum
{
  SET_SETTINGS_SUCCESS,
  SET_SETTINGS_FAIL, // Generic error
  SET_SETTINGS_INVALID_JSON,
  SET_SETTINGS_INVALID_BAND,
  SET_SETTINGS_INVALID_POWER,
  SET_SETTINGS_INVALID_SCAN_PERIOD,
  SET_SETTINGS_INVALID_BEEP
} SET_SETTINGS_STATUS;

typedef enum
{
  SET_DEVICE_SETTINGS_SUCCESS,
  SET_DEVICE_SETTINGS_FAIL, // Generic error or NVS failed
  SET_DEVICE_SETTING_INVALID_TRIGGER,
  SET_DEVICE_SETTING_INVALID_SCAN_MODE,
  SET_DEVICE_SETTING_INVALID_DATA_OUTPUT_LOC,
  SET_DEVICE_SETTING_INVALID_SCAN_INTERVAL
} SET_DEVICE_SETTING_STATUS;

typedef struct
{
  int device_id;        // Device ID
  char device_name[50]; // Device name (string)
  float fw_version;     // Firmware version
  float hw_version;     // Hardware version
  int min_freq;         // Minimum frequency
  int max_freq;         // Maximum frequency
  char rf_protocol[20]; // RF protocol (string)
  char rf_band;         // RF band (char)
  int rf_power;         // RF power
  int rf_scan_period;   // RF scan period
  bool device_beep;     // Device beep (boolean)
} rfm_settings_all_t;   // rf module settings

typedef struct
{
  int scan_interval;     // if cont scan then interval
  char *data_output_loc; // need to use strdup and free // none, ip port, bluetooth, //TODO: make this a different struct that has ip, port, encryption key(different key for each ip). if ip is error then show it in display
  TRIGGER trigger;
  char auth_key[SIZE_OF_AUTH_KEY];
} device_func_status_t; // rf module settings
// TODO: IMPORTANT: store encryption key here too and use password to set this settings.
// data_output - array[sd, link{ip, port, encryption_key}]

typedef struct
{
  char rf_band;            // RF band (char)
  int rf_power;            // RF power
  int rf_scan_period;      // RF scan period
  bool device_beep;        // Device beep (boolean)
} rfm_settings_saveable_t; // rf module settings

typedef struct
{
  SCAN_MODES scan_mode;
  bool filter;
  int offset;
  char value[24];
} scan_info_t;

typedef struct
{
  char serial_num[10 + 1];
  char default_pass[10 + 1];
  char current_pass[10 + 1];
} credentials_t;

// TODO: LATER: use getter and setter for nvs and variable sync

extern device_func_status_t functionality_status_;
extern rfm_settings_all_t settings_;
extern nvs_handle_t func_settings_handle;
extern scan_info_t scan_info_;
extern credentials_t credentials_;

bool nvs_init();
bool get_nvs_func_settings(device_func_status_t *func_settings);
bool set_nvs_func_settings(device_func_status_t *func_settings);
void print_device_func_settings(device_func_status_t *func_settings);
bool nvs_save_scan_mode();
bool nvs_load_scan_mode();

uint32_t load_increment_store_restart_counter_till_last_flash(void);

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

  rfm_settings_all_t *get_device_settings();
  SET_SETTINGS_STATUS set_device_settings(const rfm_settings_saveable_t *settings);
  device_func_status_t *get_device_func_settings();
  SET_DEVICE_SETTING_STATUS
  set_device_func_settings(const device_func_status_t *settings);

  void store_serial_number_and_default_password(const char *serial_number, const char *cur_password, const char *def_password);
  bool load_serial_number_and_default_password(char *serial_number, size_t serial_number_size,
                                               char *cur_password, size_t cur_password_size,
                                               char *def_password, size_t def_pass_size);
#ifdef __cplusplus
}
#endif //__cplusplus

// THIS IS THE MODEL TO HANDLE The business logic and database
