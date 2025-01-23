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
  ESP_ERROR_CHECK(err); // TODO: handle error
  return true;
}

bool get_nvs_func_settings(device_func_status_t *func_settings)
{
  esp_err_t err = nvs_open("func-setting", NVS_READWRITE, &func_settings_handle);
  if (err != ESP_OK)
  {
    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
  }
  else
  {
    printf("Opened Nvs for READ\n");
  }
  // READ
  size_t required_size;
  esp_err_t ret = nvs_get_str(func_settings_handle, "msg-link", NULL, &required_size);
  if (ret != ESP_OK)
  {
    printf("Error getting size: %s\n", esp_err_to_name(ret));
  }
  else
  {
    char *str_value = (char *)malloc(required_size);
    ret = nvs_get_str(func_settings_handle, "msg-link", str_value, &required_size);
    func_settings->data_output_loc = strdup(str_value);
    printf("got the nvs value..\n");
    free(str_value);
    str_value = NULL;
  }
  printf("%s\n", func_settings->data_output_loc);

  nvs_close(func_settings_handle);
  func_settings_handle = NULL;
  return true;
}

bool set_nvs_func_settings(device_func_status_t *func_settings)
{
  esp_err_t ret;

  esp_err_t err = nvs_open("func-setting", NVS_READWRITE, &func_settings_handle);
  if (err != ESP_OK)
  {
    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    // TODO: return
  }
  else
  {
    printf("Opened Nvs for write\n");
  }

  err = nvs_set_str(func_settings_handle, "msg-link", func_settings->data_output_loc);
  if (err != ESP_OK)
  {
    printf("Error setting string: %s\n", esp_err_to_name(err));
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