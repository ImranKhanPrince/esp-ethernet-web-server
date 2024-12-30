#include "api_json.h"

#include "settings.h"

#include "cJSON.h"

const char *get_settings()
{
  rfm_settings_all_t *settings = get_settings_data();
  cJSON *root_object = cJSON_CreateObject();
  if (root_object == NULL)
  {
    return NULL;
  }

  // Add data to the JSON object
  cJSON_AddNumberToObject(root_object, "device_id", settings->device_id);
  cJSON_AddStringToObject(root_object, "device_name", settings->device_name);

  char fw_version_str[10];
  snprintf(fw_version_str, sizeof(fw_version_str), "%.2f", settings->fw_version); // 2 decimal places
  cJSON_AddStringToObject(root_object, "fw_version", fw_version_str);

  char hw_version_str[10];
  snprintf(hw_version_str, sizeof(hw_version_str), "%.2f", settings->hw_version); // 2 decimal places
  cJSON_AddStringToObject(root_object, "hw_version", hw_version_str);

  cJSON_AddNumberToObject(root_object, "min_freq", settings->min_freq);
  cJSON_AddNumberToObject(root_object, "max_freq", settings->max_freq);
  cJSON_AddStringToObject(root_object, "rf_protocol", settings->rf_protocol);

  // RF Band as a single character string
  char rf_band_str[2] = {settings->rf_band, '\0'};
  cJSON_AddStringToObject(root_object, "rf_band", rf_band_str);

  cJSON_AddNumberToObject(root_object, "rf_power", settings->rf_power);
  cJSON_AddNumberToObject(root_object, "rf_scan_period", settings->rf_scan_period);
  cJSON_AddBoolToObject(root_object, "device_beep", settings->device_beep);

  // Convert JSON object to string
  char *json_string = cJSON_PrintUnformatted(root_object);
  strcat(json_string, "\n");
  cJSON_Delete(root_object); // Free JSON object

  return json_string;
}