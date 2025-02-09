#include "api_json.h"
#include "tag_memory_ops.h"
#include "settings.h"
// MEMORY OPERATIONS REQUIES SCAN TO happen thats why in scan view
#define EPC_STRING_LENGTH 24 + 1

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
  if (auth_key == NULL || strcmp(auth_key->valuestring, functionality_status_.auth_key) != 0)
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
    cJSON *memtype = cJSON_GetObjectItem(command_params, "memtype");
    if (memtype == NULL)
    {
      cJSON_Delete(command_json);
      return strdup("{\"error\":\"Invalid Memory Type\"}\n");
    }
    if (strcmp(memtype->valuestring, "EPC") == 0)
    {
      cJSON *epc_item = cJSON_GetObjectItem(command_params, "epc");
      cJSON *new_epc_item = cJSON_GetObjectItem(command_params, "new_epc");
      if (epc_item == NULL || epc_item->valuestring == NULL ||
          new_epc_item == NULL || new_epc_item->valuestring == NULL)
      {
        cJSON_Delete(command_json);
        return strdup("{\"error\":\"Invalid EPC Format\"}\n");
      }
      char old_epc[EPC_STRING_LENGTH];
      char new_epc[EPC_STRING_LENGTH];
      strncpy(old_epc, epc_item->valuestring, EPC_STRING_LENGTH - 1);
      strncpy(new_epc, new_epc_item->valuestring, EPC_STRING_LENGTH - 1);
      old_epc[EPC_STRING_LENGTH - 1] = '\0'; // Ensure null-termination
      new_epc[EPC_STRING_LENGTH - 1] = '\0'; // Ensure null-termination
      if (strlen(epc_item->valuestring) != 24 || strlen(new_epc_item->valuestring) != 24)
      {
        printf("Sizeof epc = %d\n", strlen(epc_item->valuestring));
        cJSON_Delete(command_json);
        return strdup("{\"error\":\"Invalid EPC Format\"}\n");
      }
      MEM_WRITE_STAUTUS status = change_epc(old_epc, new_epc);
      switch (status)
      {
      case MEM_WRITE_FAILED:
        printf("EPC changed successfully\n");
        cJSON_Delete(command_json);
        return strdup("{\"status\":\"success\"}\n");
      case MEM_WRITE_SUCCESSFUL:
        cJSON_Delete(command_json);
        return strdup("{\"error\":\"Failed to change EPC\"}\n");
      case TAG_NOT_FOUND:
        cJSON_Delete(command_json);
        return strdup("{\"error\":\"Tag not found\"}\n");
        break;
      }
    }
    else if (strcmp(memtype->valuestring, "USR") == 0)
    {
      cJSON *epc_item = cJSON_GetObjectItem(command_params, "epc");
      cJSON *data_item = cJSON_GetObjectItem(command_params, "data");
      cJSON *windex_item = cJSON_GetObjectItem(command_params, "windex");
      cJSON *wsize_item = cJSON_GetObjectItem(command_params, "wsize");
      if (epc_item == NULL || data_item == NULL || windex_item == NULL || wsize_item == NULL)
      {
        cJSON_Delete(command_json);
        return strdup("{\"error\":\"Invalid Parameters\"}\n");
      }

      char epc[EPC_STRING_LENGTH];
      strncpy(epc, epc_item->valuestring, EPC_STRING_LENGTH - 1);
      epc[EPC_STRING_LENGTH - 1] = '\0';
      int windex = windex_item->valueint;
      int wsize = wsize_item->valueint;

      size_t mem_size = sizeof(data_item->valuestring);
      char mem_data[mem_size];
      strncpy(mem_data, data_item->valuestring, mem_size);

      MEM_WRITE_STAUTUS status = change_user_mem(epc, mem_data, wsize, windex);
      switch (status)
      {
      case MEM_WRITE_FAILED:
        printf("Memory changed successfully\n");
        cJSON_Delete(command_json);
        return strdup("{\"status\":\"success\"}\n");
      case MEM_WRITE_SUCCESSFUL:
        cJSON_Delete(command_json);
        return strdup("{\"error\":\"Failed to change EPC\"}\n");
      case TAG_NOT_FOUND:
        cJSON_Delete(command_json);
        return strdup("{\"error\":\"Tag not found\"}\n");
      default:
        printf("Unknown Error\n");
      }
    }
    else
    {
      cJSON_Delete(command_json);
      return strdup("{\"error\":\"Invalid Memory Type\"}\n");
    }
    cJSON_Delete(command_json);
    return strdup("{\"error\":\"Invalid Command\"}\n");
  }

  printf("command: %s\n", command_name);
  cJSON_Delete(command_json);
  return strdup("{\"error\":\"Invalid Command\"}\n");
}
