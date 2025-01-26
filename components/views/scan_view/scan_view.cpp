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
  cJSON *auth_key = cJSON_GetObjectItem(root_object, "auth_key"); // a simple string/obj

  if (auth_key == NULL || strcmp(auth_key->valuestring, "1234") != 0)
  {
    cJSON_Delete(root_object);
    return strdup("{\"error\":\"Invalid Auth Key\"}\n");
  }
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

char *handle_single_scan()
{
  std::vector<ScanResult> scanResults = single_scan();

  return format_scan_result_arr(scanResults); // Return the dynamically allocated JSON string with newline
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
