#include "scan_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdio.h>

std::vector<ScanResult> single_scan()
{
  printf("LOG: single_scan called\n");
  std::vector<ScanResult> scanResults;
  ScanResult sd;

  int n = Inventory(false);
  printf("LOG: TAGS FOUND: %d\n", n);

  if (n == 0)
  {
    return scanResults; // TODO: handle the error
  }

  for (int i = 0; i < n; i++)
  {
    if (GetResult((unsigned char *)&sd, i))
    {
      scanResults.push_back(sd);
    }
  }

  return scanResults;
}

bool stop_scan_task()
{
  printf("LOG: stop_scan_task called\n");
  return true;
}
