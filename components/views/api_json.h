#pragma once
#include <esp_http_server.h>

#include <stdbool.h>

#include "cJSON.h"
// TODO: IMPORTANT: Find all the CJSON related memory leaks using -ai

bool status_view(char *response, size_t max_len);
char *get_settings();
char *set_settings(const char *data);
char *get_json_device_func_settings();
char *set_func_settings(const char *data);

#ifdef __cplusplus
extern "C"
{
#endif
  char *handle_scan_command(const char *data);
#ifdef __cplusplus
}
#endif
