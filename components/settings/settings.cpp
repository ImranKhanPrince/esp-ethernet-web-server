#include "settings.h"

#include "driver.h"

// THIS FILE WILL NOT RETURN ANY JSON INSTED IT WILL RETURN STRUCTS AND ARRAYS

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
  // TODO: Create a device tree struct where you can find the status of each devices

  ReaderInfo ri;
  GetSettings(&ri);
  printf("LOG: get_settings_data is getting called\n");
  fflush(stdout);

  settings_.min_freq = ri.MinFreq;
  settings_.max_freq = ri.MaxFreq;
  settings_.rf_band = ri.Band;
  settings_.rf_power = ri.Power;
  settings_.rf_scan_period = ri.ScanTime * 100;
  settings_.device_beep = ri.BeepOn;

  printsettings(&ri);
  return &settings_;
}