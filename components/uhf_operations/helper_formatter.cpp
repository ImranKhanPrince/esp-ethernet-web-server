#include <algorithm> // for std::sort

#include "scan_task.h"
#include "helper_func.h"
#include "cJSON.h"

// TODO: LATER: make a seperate string/json helper COMPONENT that has all the helper

bool compareEPCEqual(const ScanResult &a, const ScanResult &b)
{
  if (a.epclen != b.epclen)
  {
    return false;
  }
  return memcmp(a.epc, b.epc, a.epclen) == 0;
}

void removeDuplicateEPCs(std::vector<ScanResult> &scanResults)
{
  // Sort first to get duplicates adjacent
  std::sort(scanResults.begin(), scanResults.end(),
            [](const ScanResult &a, const ScanResult &b)
            {
              if (a.epclen != b.epclen)
              {
                return a.epclen < b.epclen;
              }
              return memcmp(a.epc, b.epc, a.epclen) < 0;
            });

  // Remove duplicates
  auto last = std::unique(scanResults.begin(), scanResults.end(), compareEPCEqual);
  scanResults.erase(last, scanResults.end());
}

char *format_scan_result_arr(std::vector<ScanResult> scanResults)
{
  printf("LOG: format_scan_result_arr called\n");

  removeDuplicateEPCs(scanResults);

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