#include "settings.h"
#include "string.h"
#include "global_status.h"

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

void store_serial_number_and_default_password(const char *serial_number, const char *cur_password, const char *def_password)
{
  nvs_handle_t nvs_handle;
  esp_err_t err;

  // Initialize NVS
  err = nvs_flash_init();
  if (err != ESP_OK)
  {
    LOGI("", "Error initializing NVS: %s\n", esp_err_to_name(err));
    return;
  }

  // Open NVS handle
  err = nvs_open("default_pass", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    LOGI("", "Error opening NVS handle: %s\n", esp_err_to_name(err));
    return;
  }

  // Write serial number
  err = nvs_set_str(nvs_handle, "serial_number", serial_number);
  if (err != ESP_OK)
  {
    LOGI("", "Error writing serial number: %s\n", esp_err_to_name(err));
    nvs_close(nvs_handle);
    return;
  }

  // Write current password
  err = nvs_set_str(nvs_handle, "cur_password", cur_password);
  if (err != ESP_OK)
  {
    LOGI("", "Error writing current password: %s\n", esp_err_to_name(err));
    nvs_close(nvs_handle);
    return;
  }

  // Write default password
  err = nvs_set_str(nvs_handle, "def_password", def_password);
  if (err != ESP_OK)
  {
    LOGI("", "Error writing default password: %s\n", esp_err_to_name(err));
    nvs_close(nvs_handle);
    return;
  }

  // Commit written value
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    LOGI("", "Error committing NVS: %s\n", esp_err_to_name(err));
  }

  // Close NVS handle
  nvs_close(nvs_handle);
}

bool load_serial_number_and_default_password(char *serial_number, size_t serial_number_size,
                                             char *cur_password, size_t cur_password_size,
                                             char *def_password, size_t def_pass_size)
{
  nvs_handle_t nvs_handle;
  esp_err_t err;

  // Initialize NVS
  err = nvs_flash_init();
  if (err != ESP_OK)
  {
    LOGI("", "Error initializing NVS: %s\n", esp_err_to_name(err));
    return false;
  }

  // Open NVS handle
  err = nvs_open("default_pass", NVS_READONLY, &nvs_handle);
  if (err != ESP_OK)
  {
    LOGI("", "Error opening NVS handle: %s\n", esp_err_to_name(err));
    return false;
  }

  // Read serial number
  size_t required_size = serial_number_size;
  err = nvs_get_str(nvs_handle, "serial_number", serial_number, &required_size);
  if (err != ESP_OK)
  {
    LOGI("", "Error reading serial number: %s\n", esp_err_to_name(err));
    nvs_close(nvs_handle);
    return false;
  }

  // Read current password
  required_size = cur_password_size;
  err = nvs_get_str(nvs_handle, "cur_password", cur_password, &required_size);
  if (err != ESP_OK)
  {
    LOGI("", "Error reading current password: %s\n", esp_err_to_name(err));
    nvs_close(nvs_handle);
    return false;
  }

  // Read default password
  required_size = def_pass_size;
  err = nvs_get_str(nvs_handle, "def_password", def_password, &required_size);
  if (err != ESP_OK)
  {
    LOGI("", "Error reading default password: %s\n", esp_err_to_name(err));
    nvs_close(nvs_handle);
    return false;
  }

  // Close NVS handle
  nvs_close(nvs_handle);
  return true;
}

bool get_nvs_func_settings(device_func_status_t *func_settings)
{
  bool ret_code = true;
  esp_err_t err = nvs_open("func-setting", NVS_READONLY, &func_settings_handle);
  if (err != ESP_OK)
  {
    LOGI("", "Error (%s) opening func_settings NVS handle!\n", esp_err_to_name(err));
    return false;
  }
  else
  {
    LOGI("", "Opened Nvs for READ\n");
  }

  // READ func_settings->data_output_loc
  size_t required_size;
  esp_err_t ret = nvs_get_str(func_settings_handle, "msg-link", NULL, &required_size);
  if (ret != ESP_OK)
  {
    LOGI("", "Error getting size: %s\n", esp_err_to_name(ret));
    ret_code = false;
  }
  else
  {
    char *str_value = (char *)malloc(required_size);
    ret = nvs_get_str(func_settings_handle, "msg-link", str_value, &required_size);
    free(func_settings->data_output_loc);
    func_settings->data_output_loc = NULL;
    func_settings->data_output_loc = strdup(str_value);
    LOGI("", "got the nvs value..\n");
    free(str_value);
    str_value = NULL;
  }

  // READ func_settings->scan_interval(uint16_t)
  uint16_t scan_interval = 0; // ms
  ret = nvs_get_u16(func_settings_handle, "scn-interval", &scan_interval);
  if (ret != ESP_OK)
  {
    LOGI("", "Failed to read scan_interval from nvs: %s\n", esp_err_to_name(err));
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
    LOGI("", "Failed to read scan trigger from nvs: %s\n", esp_err_to_name(ret));
    ret_code = false;
  }
  else
  {
    func_settings->trigger = (TRIGGER)(trigger);
  }

  size_t auth_key_size = 0;

  nvs_get_str(func_settings_handle, "auth_key", functionality_status_.auth_key, &auth_key_size);
  if (ret != ESP_OK || auth_key_size <= 0)
  {
    LOGI("", "failed to read auth_key form nvs:");
    ret_code = false;
  }

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
    LOGI("", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    return false;
  }
  else
  {
    LOGI("", "Opened NVS for write\n");
  }

  // SET func_settings->data_output_loc
  err = nvs_set_str(func_settings_handle, "msg-link", func_settings->data_output_loc);
  if (err != ESP_OK)
  {
    LOGI("", "Error saving data_out_loc: %s\n", esp_err_to_name(err));
    nvs_close(func_settings_handle);
    return false;
  }

  // SET func_settings->scan_interval - int
  err = nvs_set_u16(func_settings_handle, "scn-interval", func_settings->scan_interval);
  if (err != ESP_OK)
  {
    LOGI("", "Error saving scan_interval: %s\n", esp_err_to_name(err));
    nvs_close(func_settings_handle);
    return false;
  }

  // SET func_settings->trigger - ENUM(int)
  err = nvs_set_u8(func_settings_handle, "trigger", func_settings->trigger);
  if (err != ESP_OK)
  {
    LOGI("", "Error saving trigger: %s\n", esp_err_to_name(err));
    nvs_close(func_settings_handle);
    return false;
  }
  err = nvs_set_str(func_settings_handle, "auth_key", func_settings->auth_key);
  if (err != ESP_OK)
  {
    LOGI("", "Error saving the auth_key\n");
    nvs_close(func_settings_handle);
  }

  // nvs commit
  err = nvs_commit(func_settings_handle);
  if (err != ESP_OK)
  {
    LOGI("", "Failed to commit settings to NVS: %s\n", esp_err_to_name(err));
    nvs_close(func_settings_handle);
    return false;
  }

  // nvs close
  nvs_close(func_settings_handle);
  func_settings_handle = NULL;
  return true;
}

bool nvs_save_scan_mode()
{
  if (func_settings_handle != NULL)
  {
    LOGI("", "Nvs handle is busy in another task\n");
    return false;
  }

  esp_err_t err = nvs_open("func_settings", NVS_READWRITE, &func_settings_handle);
  if (err != ESP_OK)
  {
    LOGI("", "Error opening NVS handle: %s\n", esp_err_to_name(err));
    return false;
  }

  err = nvs_set_blob(func_settings_handle, "scan_info", (const void *)&scan_info_, sizeof(scan_info_));
  if (err != ESP_OK)
  {
    LOGI("", "Failed to write scan_info in NVS: %s\n", esp_err_to_name(err));
    nvs_close(func_settings_handle);
    func_settings_handle = NULL;
    return false;
  }

  err = nvs_commit(func_settings_handle);
  if (err != ESP_OK)
  {
    LOGI("", "Failed to commit in NVS: %s\n", esp_err_to_name(err));
    nvs_close(func_settings_handle);
    func_settings_handle = NULL;
    return false;
  }

  nvs_close(func_settings_handle);
  func_settings_handle = NULL;
  return true;
}

bool nvs_load_scan_mode()
{
  if (func_settings_handle != NULL)
  {
    LOGI("", "Nvs handle is busy in another task\n");
    return false;
  }

  esp_err_t err = nvs_open("func_settings", NVS_READONLY, &func_settings_handle);
  if (err != ESP_OK)
  {
    LOGI("", "Failed to open NVS: %s\n", esp_err_to_name(err));
    return false;
  }

  size_t required_size = sizeof(scan_info_);
  err = nvs_get_blob(func_settings_handle, "scan_info", &scan_info_, &required_size);
  if (err != ESP_OK)
  {
    LOGI("", "Failed to read scan_info from NVS: %s\n", esp_err_to_name(err));
    nvs_close(func_settings_handle);
    func_settings_handle = NULL;
    return false;
  }

  nvs_close(func_settings_handle);
  func_settings_handle = NULL;
  return true;
}

uint32_t load_increment_store_restart_counter_till_last_flash(void)
{
  // After flashing it will always be 0 but when monitor starts the value has already been increased once as monitor triggers a restart but first restart has already happend
  nvs_handle_t nvs_handle;
  esp_err_t err;
  uint32_t counter = 0;

  // Initialize NVS
  err = nvs_flash_init();
  if (err != ESP_OK)
  {
    LOGI("", "Error initializing NVS: %s\n", esp_err_to_name(err));
    return -1;
  }
  LOGI("", "NVS initialized successfully\n");

  // Open NVS handle
  err = nvs_open("default_pass", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    LOGI("", "Error opening NVS handle: %s\n", esp_err_to_name(err));
    return -1;
  }
  LOGI("", "NVS handle opened successfully\n");

  // Read the counter
  size_t required_size = sizeof(counter);
  err = nvs_get_u32(nvs_handle, "restart_c", &counter);
  if (err == ESP_ERR_NVS_NOT_FOUND)
  {
    // Counter not found, initialize it to 0
    counter = 0;
    LOGI("", "Counter not found, initializing to 0\n");
  }
  else if (err != ESP_OK)
  {
    LOGI("", "Error reading counter: %s\n", esp_err_to_name(err));
    nvs_close(nvs_handle);
    return -1;
  }
  else
  {
    LOGI("", "Counter found, current value: %lu\n", counter);
  }

  // Increment the counter
  counter++;
  LOGI("", "Incremented counter value: %lu\n", counter);

  // Write the counter back to NVS
  err = nvs_set_u32(nvs_handle, "restart_c", counter);
  if (err != ESP_OK)
  {
    LOGI("", "Error writing counter: %s\n", esp_err_to_name(err));
    nvs_close(nvs_handle);
    return -1;
  }
  LOGI("", "Counter written to NVS successfully\n");

  // Commit written value
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    LOGI("", "Error committing NVS: %s\n", esp_err_to_name(err));
    nvs_close(nvs_handle);
    return -1;
  }
  LOGI("", "NVS committed successfully\n");

  // Close NVS handle
  nvs_close(nvs_handle);
  LOGI("", "NVS handle closed successfully\n");

  return counter;
}