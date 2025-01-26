#include "settings.h"
#include "string.h"

nvs_handle_t func_settings_handle = NULL;

bool nvs_init()
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err != ESP_OK)
  {
    return false;
  }
  return true;
}

bool get_nvs_func_settings(device_func_status_t *func_settings)
{
  bool ret_code = true;

  esp_err_t err = nvs_open("func-setting", NVS_READONLY, &func_settings_handle);
  if (err != ESP_OK)
  {
    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
  }
  else
  {
    printf("Opened Nvs for READ\n");
  }
  // READ func_settings->data_output_loc
  size_t required_size;
  esp_err_t ret = nvs_get_str(func_settings_handle, "msg-link", NULL, &required_size);
  if (ret != ESP_OK)
  {
    printf("Error getting size: %s\n", esp_err_to_name(ret));
    ret_code = false;
  }
  else
  {
    char *str_value = (char *)malloc(required_size);
    ret = nvs_get_str(func_settings_handle, "msg-link", str_value, &required_size);
    free(func_settings->data_output_loc);
    func_settings->data_output_loc = NULL;
    func_settings->data_output_loc = strdup(str_value);
    printf("got the nvs value..\n");
    free(str_value);
    str_value = NULL;
  }

  // READ func_settings->scan_interval(uint16_t)
  uint16_t scan_interval = 0; // ms
  ret = nvs_get_u16(func_settings_handle, "scn-interval", &scan_interval);
  if (ret != ESP_OK)
  {
    printf("Failed to read scan_interval from nvs: %s\n", esp_err_to_name(err));
    ret_code = false;
  }
  else
  {
    func_settings->scan_interval = scan_interval;
  }

  // READ func_settings->trigger(uint8_t)
  uint8_t trigger = 99;
  ret = nvs_get_u8(func_settings_handle, "trigger", &trigger);
  if (ret != ESP_OK || trigger == 99)
  {
    printf("Failed to read scan trigger from nvs: %s\n", esp_err_to_name(ret));
    ret_code = false;
  }
  else
  {
    func_settings->trigger = (TRIGGER)(trigger);
  }

  // printf("%s\n", func_settings->data_output_loc);

  nvs_close(func_settings_handle);
  func_settings_handle = NULL;
  return ret_code;
}

bool set_nvs_func_settings(device_func_status_t *func_settings)
{
  esp_err_t ret;

  esp_err_t err = nvs_open("func-setting", NVS_READWRITE, &func_settings_handle);
  if (err != ESP_OK)
  {
    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    return false;
  }
  else
  {
    printf("Opened NVS for write\n");
  }
  // SET func_settings->data_output_loc
  err = nvs_set_str(func_settings_handle, "msg-link", func_settings->data_output_loc);
  if (err != ESP_OK)
  {
    printf("Error saving data_out_loc: %s\n", esp_err_to_name(err));
    return false;
  }
  // SET func_settings->scan_interval - int
  err = nvs_set_u16(func_settings_handle, "scn-interval", func_settings->scan_interval);
  if (err != ESP_OK)
  {
    printf("Error saving scan_interval %s", esp_err_to_name(err));
    return false;
  }

  // SET func_settings->trigger - ENUM(int)
  err = nvs_set_u8(func_settings_handle, "trigger", func_settings->trigger);
  if (err != ESP_OK)
  {
    printf("Error saving trigger %s\n", esp_err_to_name(err));
    return false;
  }

  // nvs commit
  err = nvs_commit(func_settings_handle);
  if (err != ESP_OK)
  {
    printf("failed to commit settings nvs.\n");
    return true;
  }
  // nvs close
  nvs_close(func_settings_handle);
  func_settings_handle = NULL;
  // return
  return true;
}