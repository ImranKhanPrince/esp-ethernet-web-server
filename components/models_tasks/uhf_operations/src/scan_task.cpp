#include <stdio.h>
#include "esp_log.h"

#include "scan_task.h"
#include "helper_func.h"
#include "global_status.h"
#include "socket_message.h"
#include "driver/gpio.h"
#include "esp_timer.h"

TaskHandle_t pxCont_scan_task_handle = NULL;
SemaphoreHandle_t mutex_continuous_scan_busy = NULL;
SemaphoreHandle_t binsmphr_cont_task_params = NULL;
SemaphoreHandle_t binsmphr_isr_scan_task = NULL;
static bool ir_sensor_task_started = false;

#define SCAN_ON_TRIGGER_STACK_SIZE 12 * 1024
#define CONT_SCAN_STACK_SIZE 12 * 1024
#define STACK_WARNING_THRESHOLD 512 // bytes
#define SEND_DATA_AFTER_N_SCAN 3

#define TRIG1_GPIO GPIO_NUM_39
#define TRIG2_GPIO GPIO_NUM_38

#define IR_SENSOR_CONT_SCAN_STATE 0
#define ESP_INTR_FLAG_DEFAULT 0

// Debounce time in miliscond
#define DEBOUNCE_TIME_MS 200
SemaphoreHandle_t xSingleScanSemaphore = NULL;
static const char *TAG = "scan_task";
// STATIC SIGNATURES
static void scan_on_trigger_task(void *pvParameters);

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
  LOGI("", "LOG: single_scan called\n");
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
        LOGI("", "LOG: Failed to convert hex string\n");
        xSemaphoreGive(xUhfUartMutex);
        return scanResults;
        // TODO: BUG: has a bug when given full EPC request doesn't respond. maybe due to epc size and null terminator
      }

      SetFilter(offset, maskdata_length, maskdata);
      int n = Inventory(true);
      LOGI("", "LOG: TAGS FOUND: %d\n", n);

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
      LOGI("", "LOG: TAGS FOUND: %d\n", n);

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

static void trig1_isr(void *arg)
{
  static int64_t last_interrupt_time = 0;
  // DEBOUNCE
  int64_t current_time = esp_timer_get_time() / 1000;
  if (current_time - last_interrupt_time > DEBOUNCE_TIME_MS)
  {

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Give semaphore to signal the single scan event
    xSemaphoreGiveFromISR(xSingleScanSemaphore, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE)
    {
      portYIELD_FROM_ISR();
    }
    last_interrupt_time = current_time;
  }
}

static void trig2_isr(void *arg)
{

  static int64_t last_interrupt_time = 0;
  // DEBOUNCE
  int64_t current_time = esp_timer_get_time() / 1000;
  if (current_time - last_interrupt_time > DEBOUNCE_TIME_MS)
  {

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Give semaphore to signal the single scan event
    xSemaphoreGiveFromISR(xSingleScanSemaphore, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE)
    {
      portYIELD_FROM_ISR();
    }
    last_interrupt_time = current_time;
  }
}

static void scan_on_trigger_task(void *pvParameters)
{
  bool filter = ((ScanParams *)pvParameters)->filter;
  int offset = ((ScanParams *)pvParameters)->offset;
  char *value = ((ScanParams *)pvParameters)->value;
  free(pvParameters);
  pvParameters = NULL;
  xSemaphoreGive(binsmphr_isr_scan_task);

  while (true)
  {
    if (xSemaphoreTake(xSingleScanSemaphore, portMAX_DELAY) == pdTRUE)
    {
      // TODO: IMPORTANT: from functionality_status_ take the gpio and then do action of trigger based on that

      gpio_set_direction((gpio_num_t)TRIG1_GPIO, GPIO_MODE_INPUT);
      while (gpio_get_level((gpio_num_t)TRIG1_GPIO) == IR_SENSOR_CONT_SCAN_STATE)
      {
        int ir_sensor_level = gpio_get_level((gpio_num_t)TRIG1_GPIO);
        LOGI("", "GPIO LEVEL %d\n", ir_sensor_level);
        char *scan_json_data;
        // Perform the single scan.
        std::vector<ScanResult> ScanResults = single_scan(filter, offset, value);

        // Convert results to a message and send via socket.
        // (Assume send_scan_results_to_socket is your implementation.)
        scan_json_data = format_scan_result_arr(ScanResults); // removes duplicate also
        LOGI("", "LOG: cont scan data %s\n", scan_json_data);
        write_to_sock_fifo(scan_json_data);
        free(scan_json_data);
        scan_json_data = NULL;
        ScanResults.clear();
      }
      // Customize these parameters depending on your needs.
    }
  }
  vTaskDelete(NULL);
}

bool register_trig1_isr(p_gpio_conf_t pin_detail, bool filter, int offset, char *value)
{
  // TODO: IMPORTANT: handle and use filter for this
  if (ir_sensor_task_started == true || pxCont_scan_task_handle != NULL)
  {
    LOGI("", "The Scan on trigger task is already running!");
    return false;
  }

  LOGI("", "LOG: Start ir sensor trigger called\n");
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

  // Create the semaphore to signal single scan events.
  xSingleScanSemaphore = xSemaphoreCreateBinary();
  if (xSingleScanSemaphore == NULL)
  {
    ESP_LOGE(TAG, "Failed to create semaphore");
    return false;
  }

  // Configure the IR sensor GPIO pin.
  gpio_config_t io_conf = {};
  io_conf.intr_type = pin_detail.edge; // Trigger on rising edge
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << pin_detail.pin);
  io_conf.pull_down_en = pin_detail.pulldown;
  io_conf.pull_up_en = pin_detail.pullup;
  gpio_config(&io_conf);

  // Install and attach GPIO ISR service.
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  gpio_isr_handler_add((gpio_num_t)pin_detail.pin, trig1_isr, NULL);

  start_msg_sender_task();
  // Create the event task that waits on the semaphore.
  binsmphr_isr_scan_task = xSemaphoreCreateBinary();

  BaseType_t task_result = xTaskCreate(scan_on_trigger_task,
                                       "single_scan_event_task",
                                       SCAN_ON_TRIGGER_STACK_SIZE,
                                       (void *)params,
                                       10,
                                       &pxCont_scan_task_handle);

  xSemaphoreTake(binsmphr_isr_scan_task, portMAX_DELAY);
  if (task_result != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create single_scan_event_task");
    return false;
  }
  else
  {
    ir_sensor_task_started = true;
    ESP_LOGI(TAG, "IR sensor ISR registered and event task created");
    return true;
  }
  return false;
}

bool start_cont_scan(bool filter, int offset, char *value)
{
  // TODO: IMPORTANT: handle the case wheen scan is running and some one saves the func settings. tell them to stop scan first. also make a new command for restart the scan
  LOGI("", "LOG: Start cont scan called\n");
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
                                    CONT_SCAN_STACK_SIZE,
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
      LOGI("", "Failed to create rtos_cont_scan_task.\n");
    }
  }
  else if (functionality_status_.trigger == TRIG1_INTERRUPT)
  {
    LOGI("", "ISR REGISTERED...");
    p_gpio_conf_t pin_detail = {
        .pin = TRIG1_GPIO,
        .edge = GPIO_INTR_POSEDGE,
        .pullup = GPIO_PULLUP_DISABLE,
        .pulldown = GPIO_PULLDOWN_DISABLE,
    };

    register_trig1_isr(pin_detail, filter, offset, value);
    return true;
  }
  else if (functionality_status_.trigger == TRIG2_INTERRUPT)
  {
    // TODO: IMPORTANT: create a ISR here exactly similar to IR_SENSOR and just add more condition to the task
  }
  else if (functionality_status_.trigger == TRIG1_AND_TRIG2_INTERRUPT)
  { // WHEN IR AND MOTION both gets triggered
    //  TODO: IMPORTANT: create a ISR here
  }
  else if (functionality_status_.trigger == TRIG1_OR_TRIG2_INTERRUPT)
  { // WHEN IR AND MOTION both gets triggered
    //  TODO: IMPORTANT: create a ISR here
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

  // mutex_continuous_scan_busy = xSemaphoreCreateMutex();
  char *scan_json_data;
  std::vector<ScanResult> scanResults;
  int i = 0;
  while (true)
  {
    // if (xSemaphoreTake(mutex_continuous_scan_busy, portMAX_DELAY))
    // {
    static int iteration_count = 0;
    if (++iteration_count % 10 == 0)
    {
      check_stack_watermark(NULL); // NULL gets current task handle
    }
    LOGI("", "Scanning...\n");
    scanResults = single_scan(params->filter, params->offset, params->value);

    if (scanResults.size() != 0 &&
        i % SEND_DATA_AFTER_N_SCAN == 0 &&
        scan_msg_sock_ > 0)
    {
      scan_json_data = format_scan_result_arr(scanResults); // removes duplicate also
      LOGI("", "LOG: cont scan data %s\n", scan_json_data);
      write_to_sock_fifo(scan_json_data);
      free(scan_json_data);
      scan_json_data = NULL;
      scanResults.clear();
      LOGI("", "LOG: tags found \n");
    }
    else
    {
      LOGI("", "LOG: No tags found or not sending data. \n");
    }
    // xSemaphoreGive(mutex_continuous_scan_busy);
  }
  int scan_interval_ms = MAX(functionality_status_.scan_interval, 100);
  vTaskDelay(scan_interval_ms / portTICK_PERIOD_MS);
  ++i;
  // }
  vTaskDelete(NULL);
}

bool stop_cont_scan()
{
  if (pxCont_scan_task_handle != NULL)
  {
    // xSemaphoreTake(mutex_continuous_scan_busy, portMAX_DELAY);
    // vSemaphoreDelete(mutex_continuous_scan_busy);
    // mutex_continuous_scan_busy = NULL;
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