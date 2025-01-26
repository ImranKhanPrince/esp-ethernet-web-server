#include "scan_task.h"

#include <stdio.h>

#include "global_status.h"
#include "http_message.h"

TaskHandle_t pxCont_scan_task_handle = NULL;
SemaphoreHandle_t mutex_continuous_scan_busy = NULL;

std::vector<ScanResult> single_scan()
{
  printf("LOG: single_scan called\n");
  std::vector<ScanResult> scanResults;
  ScanResult sd;
  if (xSemaphoreTake(xUhfUartMutex, portMAX_DELAY) == pdTRUE)
  {
    int n = Inventory(false);
    printf("LOG: TAGS FOUND: %d\n", n);

    if (n == 0)
    {
      return scanResults;
    }

    for (int i = 0; i < n; i++)
    {
      if (GetResult((unsigned char *)&sd, i))
      {
        scanResults.push_back(sd);
      }
    }
    xSemaphoreGive(xUhfUartMutex);
    return scanResults;
  }

  return scanResults;
}

bool start_cont_scan()
{
  printf("LOG: Star cont scan called\n");

  if (functionality_status_.trigger == NO_TRIGGER)
  {
    BaseType_t result = xTaskCreate(rtos_cont_scan_task,
                                    "scan_task",
                                    4 * 1024,
                                    NULL,
                                    10,
                                    &pxCont_scan_task_handle);

    if (result == pdPASS)
    {
      return true;
    }
    else
    {
      printf("Failed to create rtos_cont_scan_task.\n");
    }
  }
  else if (functionality_status_.trigger == IR_SENSOR)
  {
    // TODO: IMPORTANT: create a ISR here
  }
  else if (functionality_status_.trigger == MOTION_SENSOR)
  {
    // TODO: IMPORTANT: create a ISR here
  }
  else if (functionality_status_.trigger == IR_MOTION_SENSOR)
  {
    // TODO: IMPORTANT: create a ISR here
  }

  return false;
}

void rtos_cont_scan_task(void *pvParams)
{
  mutex_continuous_scan_busy = xSemaphoreCreateMutex();

  while (true)
  {
    if (xSemaphoreTake(mutex_continuous_scan_busy, portMAX_DELAY))
    {
      std::vector<ScanResult> scanResults;
      ScanResult sd;
      if (xSemaphoreTake(xUhfUartMutex, portMAX_DELAY) == pdTRUE)
      {
        int n = Inventory(false);
        printf("LOG: TAGS FOUND: %d\n", n);

        for (int i = 0; i < n; i++)
        {
          if (GetResult((unsigned char *)&sd, i))
          {
            scanResults.push_back(sd);
          }
        }

        xSemaphoreGive(xUhfUartMutex);
      }
      if (scanResults.size() != 0)
      {
        char *scan_json_data = format_scan_result_arr(scanResults);
        // TODO: MAYBE - add a direct socket msg along with HTTP
        // status will say if socket is online or not use esp_http client
        printf("LOG: cont scan data %s\n", scan_json_data);
        send_json_http_message(scan_json_data);
        printf("LOG: tags found \n");
      }
      else
      {
        printf("LOG: No tags found \n");
      }

      xSemaphoreGive(mutex_continuous_scan_busy);
    }
    vTaskDelay(functionality_status_.scan_interval / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void rtos_single_scan_task(void *pvParams)
{
  int n = 0;
  while (n < 2)
  {
    printf("LOG: SIngle Scan\n");
  }
  vTaskDelete(NULL);
}

bool stop_cont_scan()
{
  if (pxCont_scan_task_handle != NULL)
  {
    xSemaphoreTake(mutex_continuous_scan_busy, portMAX_DELAY);
    vSemaphoreDelete(mutex_continuous_scan_busy);
    mutex_continuous_scan_busy = NULL;

    vTaskDelete(pxCont_scan_task_handle);
    pxCont_scan_task_handle = NULL;

    return true;
  }

  return false;
}