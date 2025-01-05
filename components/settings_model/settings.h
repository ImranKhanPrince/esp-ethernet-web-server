#pragma once

#include <stdbool.h>

/*
device_id - int
device name - string
fw_version - int
hw_version - int
min_freq - int
max_freq - int
rf_protocol - string
rf_band - char
rf_power - int
rf_scan_period - int
device_beep - bool

*/

// TODO: Make this hw status and other one will be function status
/*
//
scan_mode - string (cont, trigger, etc)
data_output - array[sd, link{ip, port, encryption_key}]

*/
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

// TODO: Similar struct for function_status(ip, encryption_key, etc)

typedef enum
{
  NO_TRIGGER,
  IR_SENSOR,
  MOTION_SENSOR,
  IR_MOTION_SENSOR
} TRIGGER;

typedef enum
{
  SCAN_CONTINUOUS,
  SCAN_TRIGGER,
  SCAN_OFF,
  SCAN_TRIGGER_COMBINED
} SCAN_MODES;

typedef struct
{
  SCAN_MODES scan_mode;     // cont scan or trigger scan
  int scan_interval;        // if cont scan then interval
  char data_output_loc[50]; // none, ip port, bluetooth, //TODO: make this a different struct that has ip, port, encryption key(different key for each ip). if ip is error then show it in display
  TRIGGER trigger;
} device_func_status_t; // rf module settings

typedef struct
{
  char rf_band;            // RF band (char)
  int rf_power;            // RF power
  int rf_scan_period;      // RF scan period
  bool device_beep;        // Device beep (boolean)
} rfm_settings_saveable_t; // rf module settings

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
  SET_DEVICE_SETTINGS_FAIL, // Generic error
  SET_DEVICE_SETTING_INVALID_TRIGGER,
  SET_DEVICE_SETTING_INVALID_SCAN_MODE,
  SET_DEVICE_SETTING_INVALID_DATA_OUTPUT_LOC,
  SET_DEVICE_SETTING_INVALID_SCAN_INTERVAL
} SET_DEVICE_SETTING_STATUS;

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

  rfm_settings_all_t *get_device_settings();
  SET_SETTINGS_STATUS set_device_settings(const rfm_settings_saveable_t *settings);
  device_func_status_t *get_device_func_settings();
  SET_DEVICE_SETTING_STATUS
  set_device_func_settings(const device_func_status_t *settings);
#ifdef __cplusplus
}
#endif //__cplusplus

// THIS IS THE MODEL TO HANDLE The business logic and database
