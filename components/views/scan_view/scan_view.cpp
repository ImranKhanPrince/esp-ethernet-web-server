#include "api_json.h"

#include "scan_task.h"

#include "common.h"
#include "helper_func.h"

// ===============STATIC SIGNATURES=======================
static std::string tohex(const std::string &uid);

static char *handle_stop_cont_scan();
static char *handle_start_cont_scan();
static char *handle_single_scan();

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
    return strdup("{\"error\":\"Invalid JSON FORMAT\"}\n");
  }

  char *command = cJSON_Print(root_object);
  if (command == NULL)
  {
    cJSON_Delete(root_object);
    return strdup("{\"error\":\"Failed to print JSON\"}\n");
  }

  printf("LOG: command: %s\n", command);
  cJSON_Delete(root_object); // Free the JSON object

  free(command); // Free the command string

  const char *scan_response = handle_single_scan();
  if (scan_response == NULL)
  {
    return strdup("{\"error\":\"Failed to handle scan\"}\n");
  }

  printf("LOG: response: %s\n", scan_response);

  // Assuming handle_single_scan returns a dynamically allocated string
  char *response = strdup(scan_response);
  if (response == NULL)
  {
    return strdup("{\"error\":\"Failed to allocate response\"}\n");
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

// char *handle_single_scan()
// {
//   std::vector<ScanData> scanResults = single_scan();
//   cJSON *root_object = cJSON_CreateObject();
//   if (root_object == NULL)
//   {
//     return strdup("{\"error\":\"Failed to create JSON object\"}\n");
//   }

//   cJSON *scan_array = cJSON_CreateArray();
//   if (scan_array == NULL)
//   {
//     cJSON_Delete(root_object);
//     return strdup("{\"error\":\"Failed to create JSON array\"}\n");
//   }

//   for (const auto &scanData : scanResults)
//   {
//     cJSON *scan_object = cJSON_CreateObject();
//     if (scan_object == NULL)
//     {
//       cJSON_Delete(root_object);
//       return strdup("{\"error\":\"Failed to create JSON object\"}\n");
//     }

//     cJSON_AddItemToArray(scan_array, scan_object);
//     cJSON_AddNumberToObject(scan_object, "ant", scanData.ant);
//     cJSON_AddNumberToObject(scan_object, "RSSI", scanData.RSSI);
//     cJSON_AddNumberToObject(scan_object, "count", scanData.count);
//     cJSON_AddNumberToObject(scan_object, "type", scanData.type);
//     cJSON_AddStringToObject(scan_object, "EPC", tohex(scanData.data).c_str());
//   }

//   cJSON_AddItemToObject(root_object, "scan", scan_array);
//   char *json_string = cJSON_PrintUnformatted(root_object);
//   cJSON_Delete(root_object); // Free JSON object

//   if (json_string == NULL)
//   {
//     return strdup("{\"error\":\"Failed to print JSON\"}\n");
//   }

//   // Ensure the JSON string is null-terminated and add a newline
//   size_t json_len = strlen(json_string);
//   char *json_with_newline = (char *)malloc(json_len + 2); // +1 for newline, +1 for null terminator
//   if (json_with_newline == NULL)
//   {
//     free(json_string);
//     return strdup("{\"error\":\"Failed to allocate memory\"}\n");
//   }
//   strcpy(json_with_newline, json_string);
//   json_with_newline[json_len] = '\n';
//   json_with_newline[json_len + 1] = '\0';

//   free(json_string); // Free the original JSON string

//   printf("LOG: JSON: %s\n", json_with_newline);

//   return json_with_newline; // Return the dynamically allocated JSON string with newline
// }

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
  return "{\"status\": \"ok\"}";
}

static char *handle_stop_cont_scan()
{
  return "{\"status\": \"ok\"}";
}
