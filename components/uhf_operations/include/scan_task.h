#pragma once
#include "settings.h"

#include <stdbool.h>
#include <vector>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver.h"
#include "driver/gpio.h"

struct ScanParams
{
  bool filter;
  int offset;
  char value[24]; // Size matches maskdata array
};

typedef struct
{
  gpio_num_t pin;
  gpio_int_type_t edge;
  gpio_pullup_t pullup;
  gpio_pulldown_t pulldown;
} p_gpio_conf_t;

// cpp extern
extern std::vector<ScanResult>
single_scan(bool filter, int offset, char *value);
// helper
extern char *format_scan_result_arr(std::vector<ScanResult> scanResults);

// RTOS TASKS
extern TaskHandle_t pxCont_scan_task_handle;
void rtos_cont_scan_task(void *pvParams);

#ifdef __cplusplus
extern "C"
{
#endif
  extern bool start_cont_scan(bool filter, int offset, char *value);
  extern bool stop_cont_scan();

#ifdef __cplusplus
}
#endif