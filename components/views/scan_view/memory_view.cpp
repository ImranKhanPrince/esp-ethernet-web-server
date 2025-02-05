#include "api_json.h"
#include "tag_memory_ops.h"
// MEMORY OPERATIONS REQUIES SCAN TO happen thats why in scan view

char *view_handle_memory_command(const char *data)
{
  if (data == NULL)
  {
    return strdup("{\"error\":\"Invalid JSON FORMAT\"}\n");
  }
  cJSON *command_json = cJSON_Parse(data);
  if (command_json == NULL)
  {
    return strdup("{\"error\":\"Invalid JSON FORMAT\"}\n");
  }
  // ----Auth
  cJSON *auth_key = cJSON_GetObjectItem(command_json, "auth_key");
  if (auth_key == NULL || strcmp(auth_key->valuestring, "1234") != 0)
  {
    cJSON_Delete(command_json);
    return strdup("{\"error\":\"Invalid auth key\"}\n");
  }
  // ----Auth finish
  cJSON *command_data = cJSON_GetObjectItem(command_json, "data");
  if (command_data == NULL)
  {
    cJSON_Delete(command_json);
    return strdup("{\"error\":\"Invalid Data Format\"}\n");
  }
  const char *command_name = cJSON_GetObjectItem(command_data, "command")->valuestring;
  cJSON *command_params = cJSON_GetObjectItem(command_data, "params");
  if (command_params == NULL)
  {
    cJSON_Delete(command_json);
    return strdup("{\"error\":\"Invalid Params Format\"}\n");
  }
  if (command_name != NULL && strcmp(command_name, "READ") == 0)
  {
    char *memory_type = cJSON_GetObjectItem(command_params, "memtype")->valuestring;
    if (memory_type != NULL && strcmp(memory_type, "TID") == 0)
    {
      cJSON *epc_str = cJSON_GetObjectItem(command_params, "epc");
      if (epc_str->valuestring == NULL)
      {
        cJSON_Delete(command_json);
        return strdup("{\"error\":\"Invalid EPC Format\"}\n");
      }
      char *response = get_tid_memory(epc_str->valuestring);
      return strdup(response);
    }
    else if (memory_type != NULL && strcmp(memory_type, "USR") == 0)
    {
      cJSON *epc_str = cJSON_GetObjectItem(command_params, "epc");
      if (epc_str->valuestring == NULL)
      {
        cJSON_Delete(command_json);
        return strdup("{\"error\":\"Invalid EPC Format\"}\n");
      }
      cJSON *wnum = cJSON_GetObjectItem(command_params, "wnum");
      cJSON *windex = cJSON_GetObjectItem(command_params, "windex");
      if (wnum == NULL || windex == NULL)
      {
        cJSON_Delete(command_json);
        return strdup("{\"error\":\"invalid parameters\"}\n");
      }

      char *response = get_usr_memory(epc_str->valuestring, wnum->valueint, windex->valueint);
      cJSON_Delete(command_json);
      return strdup(response);
    }
    else
    {
      cJSON_Delete(command_json);
      return strdup("{\"error\":\"Invalid Memory type\"}\n");
    }
  }
  else if (command_name != NULL && strcmp(command_name, "WRITE") == 0)
  {
  }
  cJSON_Delete(command_json);
  return strdup("{status: \"success\"}");
}
