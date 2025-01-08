#include "api_json.h"

#include "settings.h"

// TODO: Cahnge the name of this file into settings_view.c

const char *get_settings()
{
  rfm_settings_all_t *settings = get_device_settings();
  if (settings == NULL)
  {
    return strdup("{\"error\":\"Failed to get device settings\"}\n");
  }

  cJSON *root_object = cJSON_CreateObject();
  if (root_object == NULL)
  {
    return strdup("{\"error\":\"Failed to create JSON object\"}\n");
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
  cJSON_Delete(root_object); // Free JSON object

  if (json_string == NULL)
  {
    return strdup("{\"error\":\"Failed to print JSON\"}\n");
  }

  return strdup(json_string);
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

/* Example JSON data:
{
    "auth_key": "1234",
    "data": {
    "rf_band": "U",
    "rf_power": 26,
    "rf_scan_period": 300,
    "device_beep": true
    }
}
*/
const char *set_settings(const char *data)
{
  printf("LOG: Data: %s\n", data);
  cJSON *root = cJSON_Parse(data);

  if (root == NULL)
  {
    printf("LOG: Invalid JSON FORMAT\n");
    return NULL;
  }
  cJSON *auth_key = cJSON_GetObjectItem(root, "auth_key");
  if (auth_key == NULL || auth_key->valuestring == NULL)
  {
    cJSON_Delete(root);
    return "{\"error\":\"Invalid JSON FORMAT\"}\n";
  }

  // verify the auth key
  if (strcmp(auth_key->valuestring, "1234") != 0)
  {
    cJSON_Delete(root);
    return "{\"error\":\"Invalid Auth Key\"}\n";
  }

  char *data_json = cJSON_Print(root);
  printf("LOG: Data_json: %s\n", data_json);

  // Extract the "data" object from the input JSON
  cJSON *data_object = cJSON_GetObjectItem(root, "data");

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
  SET_SETTINGS_STATUS status = set_device_settings(&setting_s);
  if (status != SET_SETTINGS_SUCCESS)
  {
    cJSON *error_object = cJSON_CreateObject();

    if (status == SET_SETTINGS_INVALID_BAND)
    {
      cJSON_AddStringToObject(error_object, "error", "Invalid RF Band");
      char *error_json = cJSON_PrintUnformatted(error_object);
      // strcat(error_json, "\n");
      cJSON_Delete(error_object);
      return error_json;
    }
    else if (status == SET_SETTINGS_INVALID_POWER)
    {
      return "Invalid RF Power";
    }
    else if (status == SET_SETTINGS_INVALID_SCAN_PERIOD)
    {
      return "Invalid RF Scan Period";
    }
    else if (status == SET_SETTINGS_INVALID_BEEP)
    {
      return "Invalid Device Beep";
    }
    else if (status == SET_SETTINGS_FAIL)
    {
      return "Failed! Try again";
    }
    else
    {
      return "Unknown Error";
    }
  }

  return "{\"success\":\"Settings set successfully.\"}\n";
}

const char *get_json_device_func_settings()
{
  cJSON *root_object = cJSON_CreateObject();

  device_func_status_t *device_func_settings = get_device_func_settings();

  cJSON_AddNumberToObject(root_object, "scan_interval", device_func_settings->scan_interval);
  cJSON_AddStringToObject(root_object, "data_output_loc", device_func_settings->data_output_loc);
  cJSON_AddNumberToObject(root_object, "trigger", device_func_settings->trigger);

  char *json_string = cJSON_PrintUnformatted(root_object);
  strcat(json_string, "\n");
  cJSON_Delete(root_object); // Free JSON object

  return json_string;
}

const char *set_func_settings(const char *data)
{
  cJSON *root_object = cJSON_Parse(data);
  if (root_object == NULL)
  {
    return NULL;
  }
  device_func_status_t device_function_stat;

  cJSON *scan_interval = cJSON_GetObjectItem(root_object, "scan_interval");
  if (cJSON_IsNumber(scan_interval))
  {
    device_function_stat.scan_interval = scan_interval->valueint;
  }
  else
  {
    return "{\"error\":\"Invalid scan_interval format\"}\n";
  }

  cJSON *data_output_loc = cJSON_GetObjectItem(root_object, "data_output_loc");
  if (cJSON_IsString(data_output_loc) && (data_output_loc->valuestring != NULL))
  {
    // TODO: check the size of the string and return error if it is too long
    strncpy(device_function_stat.data_output_loc, data_output_loc->valuestring, sizeof(device_function_stat.data_output_loc) - 1);
    device_function_stat.data_output_loc[sizeof(device_function_stat.data_output_loc) - 1] = '\0'; // Ensure null-termination
  }
  else
  {
    return "{\"error\":\"Invalid data_output_loc format\"}\n";
  }

  cJSON *trigger = cJSON_GetObjectItem(root_object, "trigger");
  if (cJSON_IsNumber(trigger))
  {
    device_function_stat.trigger = trigger->valueint;
  }
  else
  {
    return "{\"error\":\"Invalid trigger format\"}\n";
  }
  cJSON_Delete(root_object);
  set_device_func_settings(&device_function_stat); // call to model

  printf("LOG: Data: %s\n", data);
  return "{\"success\":\"Settings set successfully.\"}\n";
}