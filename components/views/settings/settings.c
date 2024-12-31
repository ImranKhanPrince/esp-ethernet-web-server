#include "api_json.h"

#include "settings.h"

#include "cJSON.h"

// TODO: Cahnge the name of this file into settings_view.c

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

char *settings_to_json(const rfm_settings_saveable_t *settings)
{
  if (settings == NULL)
  {
    return NULL;
  }

  // Create a JSON object
  cJSON *root_object = cJSON_CreateObject();
  if (root_object == NULL)
  {
    return NULL;
  }

  // Add the settings to the JSON object
  cJSON_AddStringToObject(root_object, "rf_band", (char[]){settings->rf_band, '\0'});
  cJSON_AddNumberToObject(root_object, "rf_power", settings->rf_power);
  cJSON_AddNumberToObject(root_object, "rf_scan_period", settings->rf_scan_period);
  cJSON_AddBoolToObject(root_object, "device_beep", settings->device_beep);

  // Convert JSON object to string
  char *json_string = cJSON_PrintUnformatted(root_object);
  cJSON_Delete(root_object); // Free JSON object

  return json_string;
}
const char *set_settings(const char *data)
{
  printf("LOG: Data: %s\n", data);
  cJSON *root = cJSON_Parse(data);
  char *data_json = cJSON_Print(root);

  printf("LOG: Data_json: %s\n", data_json);

  if (root == NULL)
  {
    return NULL;
  }

  // Extract the "data" object from the input JSON
  cJSON *data_object = cJSON_GetObjectItem(root, "data");
  // TODO: HERE LIES THE PROBLEM print and verify req -> json ->> stripped json ->> new json
  if (data_object == NULL)
  {
    cJSON_Delete(root);
    return NULL;
  }

  rfm_settings_saveable_t setting_s;

  cJSON *rf_band = cJSON_GetObjectItem(data_object, "rf_band");
  if (cJSON_IsString(rf_band) && (rf_band->valuestring != NULL))
  {
    setting_s.rf_band = rf_band->valuestring[0];
  }

  cJSON *rf_power = cJSON_GetObjectItem(data_object, "rf_power");
  if (cJSON_IsNumber(rf_power))
  {
    setting_s.rf_power = rf_power->valueint;
  }

  cJSON *rf_scan_period = cJSON_GetObjectItem(data_object, "rf_scan_period");
  if (cJSON_IsNumber(rf_scan_period))
  {
    setting_s.rf_scan_period = rf_scan_period->valueint;
  }

  cJSON *device_beep = cJSON_GetObjectItem(data_object, "device_beep");
  if (cJSON_IsBool(device_beep))
  {
    setting_s.device_beep = cJSON_IsTrue(device_beep);
  }

  // char *json_string = settings_to_json(&setting_s);
  // strcat(json_string, "\n");
  cJSON_Delete(root);

  //  process the data into a struct then pass that to model/settings/peripheral to write the settings
  SET_SETTINGS_STATUS status = set_settings_data(&setting_s);
  if (status != SET_SETTINGS_SUCCESS)
  {
    return "Failed to update settings";
  }
  return "Settings Updated Successfully";
}
