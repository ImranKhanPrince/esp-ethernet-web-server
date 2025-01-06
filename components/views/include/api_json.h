#pragma once
#include <esp_http_server.h>

#include <stdbool.h>

#include "cJSON.h"

bool status_view(char *response, size_t max_len);
const char *get_settings();
const char *set_settings(const char *data);
const char *get_json_device_func_settings();
const char *set_func_settings(const char *data);

#ifdef __cplusplus
extern "C"
{
#endif
  char *handle_scan_command(const char *data);
#ifdef __cplusplus
}
#endif
