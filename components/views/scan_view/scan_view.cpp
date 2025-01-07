#include "api_json.h"

#include "scan_task.h"

#include "common.h"
#include "helper_func.h"

// ===============STATIC SIGNATURES=======================
static std::string tohex(const std::string &uid);

static char *handle_stop_cont_scan();
static char *handle_start_cont_scan();
static char *handle_single_scan();

// {
//     "auth_key": "1234",
//     "data":{
//     "command": 0, // scan once(0), start const scan(1), stop cont scan(2),
//     "params": {
//         "filter": true,
//         "offset": 0,//int
//         "data": "02"//hex string
//     }
//    }
// }
char *handle_scan_command(const char *data)
{
  printf("LOG: handle_scan_command called\n");
  if (data == NULL)
  {
    return strdup("{\"error\":\"Invalid JSON FORMAT\"}\n");
  }

  cJSON *root_object = cJSON_Parse(data);
  if (root_object == NULL)
  {
    cJSON_Delete(root_object);
    return strdup("{\"error\":\"Invalid JSON FORMAT\"}\n");
  }
  // --------Auth
  cJSON *auth_key = cJSON_GetObjectItem(root_object, "auth_key");

  if (auth_key == NULL || strcmp(auth_key->valuestring, "1234") != 0)
  {
    cJSON_Delete(root_object);
    cJSON_Delete(auth_key);
    return strdup("{\"error\":\"Invalid Auth Key\"}\n");
  }
  // cJSON_Delete(auth_key); // TODO: causes crahs know why
  // ---- Auth finish

  cJSON *data_object = cJSON_GetObjectItem(root_object, "data");
  if (data_object == NULL)
  {
    cJSON_Delete(root_object);
    return strdup("{\"error\":\"data format invalid\"}\n");
  }
  cJSON *scan_mode_obj = cJSON_GetObjectItem(data_object, "command");
  if (scan_mode_obj == NULL)
  {
    cJSON_Delete(data_object);
    return strdup("{\"error\":\"Invalid command Number\"}\n");
  }
  int scan_mode = cJSON_GetNumberValue(scan_mode_obj);

  char *response;
  if (scan_mode == SCAN_ONCE)
  {
    const char *scan_response = handle_single_scan();
    if (scan_response == NULL)
    {
      return strdup("{\"error\":\"Failed to handle scan\"}\n");
    }
    printf("LOG: response: %s\n", scan_response);
    response = strdup(scan_response);
    if (response == NULL)
    {
      return strdup("{\"error\":\"Failed to allocate response\"}\n");
    }
  }
  else if (scan_mode == SCAN_CONTINUOUS)
  {
    const char *response_json = handle_start_cont_scan();
    response = strdup(response_json);
  }
  else if (scan_mode == SCAN_OFF)
  {
    const char *response_json = handle_stop_cont_scan();
    response = strdup(response_json);
    // stop cont scan
  }
  else
  {
    response = strdup("{\"error\":\"Wrong Command\"}\n");
  }

  return response;
}

static std::string tohex(const std::string &uid)
{
  std::string z = "";
  char p[3] = "00";
  for (int i = 0; i < (int)uid.size(); i++)
  {
    snprintf(p, sizeof(p), "%02X", uid[i] & 0xFF);
    z += p;
  }
  return z;
}

char *handle_single_scan()
{
  std::vector<ScanResult> scanResults = single_scan();

  cJSON *scan_array = cJSON_CreateArray();
  if (scan_array == NULL)
  {
    cJSON_Delete(scan_array);
    return strdup("{\"error\":\"Failed to create JSON array\"}\n");
  }

  for (size_t i = 0; i < scanResults.size(); ++i)
  {
    cJSON *scan_object = cJSON_CreateObject();
    if (scan_object == NULL)
    {
      cJSON_Delete(scan_array);
      return strdup("{\"error\":\"Failed to create JSON object\"}\n");
    }

    cJSON_AddNumberToObject(scan_object, "ant", scanResults[i].ant);
    cJSON_AddStringToObject(scan_object, "RSSI", signedCharToString(scanResults[i].RSSI));
    cJSON_AddNumberToObject(scan_object, "count", scanResults[i].count);
    cJSON_AddNumberToObject(scan_object, "epclen", scanResults[i].epclen);
    cJSON_AddStringToObject(scan_object, "EPC", bytes2hex(scanResults[i].epc, scanResults[i].epclen).c_str());
    cJSON_AddItemToArray(scan_array, scan_object);
    std::string epc = std::string((char *)scanResults[i].epc, scanResults[i].epclen);
  }

  char *json_string = cJSON_PrintUnformatted(scan_array);
  printf("LOG: JSON: %s\n", json_string);
  cJSON_Delete(scan_array); // Free JSON object

  return json_string; // Return the dynamically allocated JSON string with newline
}

static char *handle_start_cont_scan()
{
  if (pxCont_scan_task_handle != NULL)
  {
    return "{\"error\": \"already running\"}";
  }
  if (start_cont_scan())
  {
    return "{\"status\": \"sending started according to setting\"}";
  }

  return "{\"error\": \" Failed to start continuous scan.\"}";
}

static char *handle_stop_cont_scan()
{
  if (pxCont_scan_task_handle == NULL)
  {
    return "{\"error\": \"continuous scan wasn't running\"}";
  }
  if (stop_cont_scan())
  {
    return "{\"status\": \"scan_stopped successfully\"}";
  }
  if (pxCont_scan_task_handle != NULL)
  {
    return "{\"error\": \"couldn't stop continous scan\"}";
  }
  return "{\"error\": \"Unknown error\"}";
}
