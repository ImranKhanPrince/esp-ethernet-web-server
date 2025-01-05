#pragma once
#include <esp_http_server.h>

bool status_view(char *response, size_t max_len);
const char *get_settings();
const char *set_settings(const char *data);
const char *get_json_device_func_settings();
const char *set_func_settings(const char *data);
