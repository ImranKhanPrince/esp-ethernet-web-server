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

// TODO: Similar struct for function_status(ip, encryption_key, etc)

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

  rfm_settings_all_t *get_settings_data();
  SET_SETTINGS_STATUS set_settings_data(const rfm_settings_saveable_t *settings);

#ifdef __cplusplus
}
#endif //__cplusplus

// THIS IS THE MODEL TO HANDLE The business logic and database
