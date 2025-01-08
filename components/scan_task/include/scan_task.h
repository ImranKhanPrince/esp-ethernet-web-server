#pragma once
#include "settings.h"

#include <stdbool.h>
#include <vector>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver.h"

// cpp extern
extern std::vector<ScanResult> single_scan();
// helper
extern char *format_scan_result_arr(std::vector<ScanResult> scanResults);

// RTOS TASKS
extern TaskHandle_t pxCont_scan_task_handle;
void rtos_cont_scan_task(void *pvParams);

#ifdef __cplusplus
extern "C"
{
#endif
  extern bool start_cont_scan();
  extern bool stop_cont_scan();

#ifdef __cplusplus
}
#endif