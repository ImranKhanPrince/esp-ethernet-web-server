#include "api_json.h"

#include "scan_task.h"

#include "common.h"
#include "helper_func.h"
#include "global_status.h"

// ===============STATIC SIGNATURES=======================
static std::string tohex(const std::string &uid);

static char *handle_stop_cont_scan();
static char *handle_start_cont_scan(bool filter, int offset, char *value);
static char *handle_single_scan(bool filter, int offset, char *value);

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
  LOGI("", "LOG: handle_scan_command called\n");
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

  if (auth_key == NULL || strcmp(auth_key->valuestring, functionality_status_.auth_key) != 0)
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

  cJSON *params_obj = cJSON_GetObjectItem(data_object, "params");
  if (params_obj == NULL)
  {
    cJSON_Delete(root_object);
    return strdup("{\"error\":\"params format invalid\"}\n");
  }

  // Extract individual params
  cJSON *filter_obj = cJSON_GetObjectItem(params_obj, "filter");
  cJSON *offset_obj = cJSON_GetObjectItem(params_obj, "offset");
  cJSON *data_obj = cJSON_GetObjectItem(params_obj, "data");

  if (filter_obj == NULL || !cJSON_IsBool(filter_obj) ||
      offset_obj == NULL || !cJSON_IsNumber(offset_obj) ||
      data_obj == NULL || !cJSON_IsString(data_obj))
  {
    cJSON_Delete(root_object);
    return strdup("{\"error\":\"Invalid params\"}\n");
  }

  // Use the extracted params
  bool filter = cJSON_IsTrue(filter_obj);
  int offset = cJSON_GetNumberValue(offset_obj);
  char *value = cJSON_GetStringValue(data_obj);

  if (scan_mode == SCAN_ONCE)
  {

    LOGI("", "LOG: Handling single scan with filter=%d, offset=%d, data=%s\n",
         filter, offset, data);

    const char *scan_response = handle_single_scan(filter, offset, value);
    if (scan_response == NULL)
    {
      return strdup("{\"error\":\"Failed to handle scan\"}\n");
    }
    LOGI("", "LOG: response: %s\n", scan_response);
    response = strdup(scan_response);
    if (response == NULL)
    {
      return strdup("{\"error\":\"Failed to allocate response\"}\n");
    }
  }
  else if (scan_mode == SCAN_CONTINUOUS)
  {
    LOGI("", "LOG: Handling single scan with filter=%d, offset=%d, data=%s\n",
         filter, offset, value);
    const char *response_json = handle_start_cont_scan(filter, offset, value);
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

char *handle_single_scan(bool filter, int offset, char *value)
{
  std::vector<ScanResult> scanResults = single_scan(filter, offset, value);

  return format_scan_result_arr(scanResults); // Return the dynamically allocated JSON string with newline
}

static char *handle_start_cont_scan(bool filter, int offset, char *value)
{
  if (pxCont_scan_task_handle != NULL)
  {
    return "{\"error\": \"already running\"}";
  }
  if (start_cont_scan(filter, offset, value))
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
