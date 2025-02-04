#include <stdio.h>
#include "esp_log.h"

#include "scan_task.h"
#include "helper_func.h"
#include "global_status.h"
#include "http_message.h"

TaskHandle_t pxCont_scan_task_handle = NULL;
SemaphoreHandle_t mutex_continuous_scan_busy = NULL;
SemaphoreHandle_t binsmphr_cont_task_params = NULL;

#define STACK_WARNING_THRESHOLD 512 // bytes
static const char *TAG = "scan_task";
#define SEND_DATA_AFTER_N_SCAN 3

// TODO: learn this function
bool hex_string_to_byte_array(const char *hex_string, unsigned char *byte_array, size_t *array_length)
{
  if (hex_string == nullptr || byte_array == nullptr || array_length == nullptr)
  {
    return false;
  }

  size_t hex_len = strlen(hex_string);
  if (hex_len % 2 != 0)
  {
    return false; // Hex string must have even length
  }

  *array_length = hex_len / 2;

  for (size_t i = 0; i < hex_len; i += 2)
  {
    char hex_byte[3] = {hex_string[i], hex_string[i + 1], '\0'};
    char *end_ptr;
    unsigned long value = strtoul(hex_byte, &end_ptr, 16);

    if (*end_ptr != '\0' || value > 0xFF)
    {
      return false; // Invalid hex value
    }

    byte_array[i / 2] = (unsigned char)value;
  }

  return true;
}

std::vector<ScanResult> single_scan(bool filter, int offset, char *value)
{
  printf("LOG: single_scan called\n");
  std::vector<ScanResult> scanResults;
  ScanResult sd;
  if (xSemaphoreTake(xUhfUartMutex, portMAX_DELAY) == pdTRUE)
  {
    if (filter)
    {
      unsigned char maskdata[24];
      size_t maskdata_length = 0;
      if (!hex_string_to_byte_array(value, maskdata, &maskdata_length))
      {
        printf("LOG: Failed to convert hex string\n");
        xSemaphoreGive(xUhfUartMutex);
        return scanResults;
        // TODO: has a bug when given full EPC request doesn't respond
      }
      //   unsigned char maskdata[] = {0x33, 0x30};
      //// SetFilter(int maskAdrInByte, int maskLenInByte, unsigned char *maskDataByte);
      //   SetFilter(0, 2, (unsigned char*) maskdata);

      SetFilter(offset, maskdata_length, maskdata);
      int n = Inventory(true);
      printf("LOG: TAGS FOUND: %d\n", n);

      if (n == 0)
      {
        xSemaphoreGive(xUhfUartMutex);
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
    else
    {
      int n = Inventory(false);
      printf("LOG: TAGS FOUND: %d\n", n);

      if (n == 0)
      {
        xSemaphoreGive(xUhfUartMutex);
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
  }
  return scanResults;
}

bool start_cont_scan(bool filter, int offset, char *value)
{
  printf("LOG: Start cont scan called\n");
  scan_info_.scan_mode = SCAN_CONTINUOUS;
  if (filter == true)
  {
    scan_info_.filter = filter;
    scan_info_.offset = offset;
    // free(scan_info_.value);
    strncpy(scan_info_.value, value, sizeof(scan_info_.value) - 1);
  }
  else
  {
    scan_info_.filter = filter;
  }
  nvs_save_scan_mode();

  ScanParams *params = (ScanParams *)pvPortMalloc(sizeof(ScanParams));
  if (params == NULL)
  {
    return false;
  }
  params->filter = filter;
  params->offset = offset;
  strncpy(params->value, value, sizeof(params->value) - 1);
  params->value[sizeof(params->value) - 1] = '\0';

  if (functionality_status_.trigger == NO_TRIGGER)
  {
    binsmphr_cont_task_params = xSemaphoreCreateBinary();
    start_msg_sender_task();
    BaseType_t result = xTaskCreate(rtos_cont_scan_task,
                                    "scan_task",
                                    8 * 1024,
                                    (void *)params,
                                    10,
                                    &pxCont_scan_task_handle);

    if (result == pdPASS)
    {
      xSemaphoreTake(binsmphr_cont_task_params, portMAX_DELAY); // wait till the semaphore is released
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

static void check_stack_watermark(TaskHandle_t task_handle)
{
  UBaseType_t watermark = uxTaskGetStackHighWaterMark(task_handle);
  if (watermark < STACK_WARNING_THRESHOLD)
  {
    ESP_LOGW(TAG, "WARNING: Task stack watermark low: %d bytes", watermark);
  }
  ESP_LOGI(TAG, "Current stack watermark: %d bytes", watermark);
}

void rtos_cont_scan_task(void *pvParams)
{
  ScanParams *params = (ScanParams *)pvParams;
  xSemaphoreGive(binsmphr_cont_task_params); // data copy done releasing binary semaphore

  mutex_continuous_scan_busy = xSemaphoreCreateMutex();
  char *scan_json_data;
  std::vector<ScanResult> scanResults;
  int i = 0;
  while (true)
  {
    if (xSemaphoreTake(mutex_continuous_scan_busy, portMAX_DELAY))
    {
      static int iteration_count = 0;
      if (++iteration_count % 10 == 0)
      {
        check_stack_watermark(NULL); // NULL gets current task handle
      }
      printf("Scanning...\n");
      scanResults = single_scan(params->filter, params->offset, params->value);

      if (scanResults.size() != 0 &&
          i % SEND_DATA_AFTER_N_SCAN == 0 &&
          scan_msg_sock_ > 0)
      {
        scan_json_data = format_scan_result_arr(scanResults); // removes duplicate also
        printf("LOG: cont scan data %s\n", scan_json_data);
        write_to_sock_fifo(scan_json_data);
        free(scan_json_data);
        scan_json_data = NULL;
        scanResults.clear();
        printf("LOG: tags found \n");
      }
      else
      {
        printf("LOG: No tags found or not sending data. \n");
      }
      xSemaphoreGive(mutex_continuous_scan_busy);
    }
    int scan_interval_ms = MAX(functionality_status_.scan_interval, 100);
    vTaskDelay(scan_interval_ms / portTICK_PERIOD_MS);
    ++i;
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
    stop_socket_msg_task(); // asyncronously turns off the socket msg task
    scan_info_.scan_mode = SCAN_OFF;
    scan_info_.filter = false;
    nvs_save_scan_mode();

    vTaskDelete(pxCont_scan_task_handle);
    pxCont_scan_task_handle = NULL;

    return true;
  }

  return false;
}