#include "settings.h"
#include "global_status.h"

#define LOG_TAG "Settings Helper"
void print_device_func_settings(device_func_status_t *func_settings)
{
  LOGI(LOG_TAG, "{\n\t\"data_output_loc\":\"%s\", \n\t\"scan_interval\": %d, \n\t\"trigger\": %d\n}",
       func_settings->data_output_loc,
       func_settings->scan_interval,
       func_settings->trigger);
};