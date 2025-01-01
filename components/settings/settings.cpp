#include "settings.h"

#include "driver.h"

// THIS FILE WILL NOT RETURN ANY JSON INSTED IT WILL RETURN STRUCTS AND ARRAYS
// Create a UART Mutex that controls the access of the UHF module UART otherwise parallel processing 2 route will cause issue
// use nvs to keep all the settigns data. keep nvs applications in another component just call the helpers from here.

static rfm_settings_all_t settings_ = {
    .device_id = 1,
    .device_name = "J4221UI",
    .fw_version = 0.1,
    .hw_version = 0.1,
    .min_freq = 100,
    .max_freq = 1000,
    .rf_protocol = "ISO 18000-6C",
    .rf_band = 'U',
    .rf_power = 20,         // dBm
    .rf_scan_period = 1000, // ms
    .device_beep = true};

rfm_settings_all_t *get_settings_data()
{
  // These functions Gets called one at a time which is handled by the http_server component. so no worry of concurrency.
  // However if wanted we can use Tasks then it will be concurrent.

  // TODO: Create a device tree struct where you can find the status of each devices

  ReaderInfo ri;
  GetSettings(&ri);

  settings_.min_freq = ri.MinFreq;
  settings_.max_freq = ri.MaxFreq;
  settings_.rf_band = ri.Band;
  settings_.rf_power = ri.Power;
  settings_.rf_scan_period = ri.ScanTime * 100;
  settings_.device_beep = ri.BeepOn;

  printsettings(&ri);
  return &settings_;
}

SET_SETTINGS_STATUS set_settings_data(const rfm_settings_saveable_t *settings)
{

  if (settings->rf_band != 'U' && settings->rf_band != 'E' && settings->rf_band != 'C' && settings->rf_band != 'K')
  {
    return SET_SETTINGS_INVALID_BAND;
  }
  if (settings->rf_power < 0 || settings->rf_power > 30)
  {
    return SET_SETTINGS_INVALID_POWER;
  }
  if (settings->rf_scan_period < 300 || settings->rf_scan_period > 25500)
  {
    return SET_SETTINGS_INVALID_SCAN_PERIOD;
  }
  if (settings->device_beep != true && settings->device_beep != false)
  {
    return SET_SETTINGS_INVALID_BEEP;
  }

  ReaderInfo ri;
  ri.Band = settings->rf_band;
  ri.Power = settings->rf_power;
  ri.ScanTime = settings->rf_scan_period / 100;
  ri.BeepOn = settings->device_beep;
  ri.BaudRate = 115200;

  // TODO: USE Mutex to lock the UART
  if (SetSettings(&ri))
  {
    return SET_SETTINGS_SUCCESS;
  }
  else
  {
    return SET_SETTINGS_FAIL;
  }
}