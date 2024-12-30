#pragma once
#include <esp_http_server.h>

bool status_view(char *response, size_t max_len);
const char *get_settings();
