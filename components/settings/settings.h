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

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

  rfm_settings_all_t *get_settings_data();

#ifdef __cplusplus
}
#endif //__cplusplus

// THIS IS THE MODEL TO HANDLE The business logic and database
