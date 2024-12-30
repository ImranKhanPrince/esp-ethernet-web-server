#include "api_json.h"

bool status_view(char *response, size_t max_len)
{
  const char *json_response = "{\"status\": \"ok\"}";

  // TODO: how it handles delay does it keeps the connection alive or not

  if (strlen(json_response) < max_len)
  {
    strcpy(response, json_response);
    return true;
  }
  else
  {
    return false; // buffer too small TODO: maybe make chunk by chunk
  }
}