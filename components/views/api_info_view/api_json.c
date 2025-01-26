#include "api_json.h"

bool status_view(char *response, size_t max_len)
{
  const char *json_response = "{\"status\": \"ok\"}";

  if (strlen(json_response) < max_len)
  {
    strcpy(response, json_response);
    return true;
  }
  else
  {
    return false;
  }
}