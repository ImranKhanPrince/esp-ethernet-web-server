#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h> // For NULL

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

typedef enum
{
  ETH_CONNECTED_NO_IP,
  ETH_CONNECTED_GOT_IP,
  ETH_UNAVAILABLE
} ETH_STATUS;

typedef enum
{
  INTERNET_CONNECTED,
  INERNET_DISCONNECTED
} INTERNET_STATUS;

typedef enum
{
  UHF_CONNECTED,
  UHF_DISCONNECTED
} UHF_STATUS;

typedef struct
{
  UHF_STATUS uhf_connected;
  INTERNET_STATUS internet_connected;
} device_status_t;

//==================rtos semaphore handle==========
extern SemaphoreHandle_t xUhfUartMutex;

// ================EXTERNS VARIABLES ===============
extern device_status_t device_status_;

// ============ EXTERN FUNCTIONS ===================

#ifdef __cplusplus
extern "C"
{
#endif

  extern UHF_STATUS get_uhf_status();
  extern bool set_uhf_status(UHF_STATUS uhf_status);

  // TASK
  void rtos_check_uhf_module_task(void *pvParams);
  // TODO: MAYBE: create a uart interrup that on uart event will try to reconnect cz when the module is powered up it sends some data through tx.
  // not necessary for now as the esp powers up along with the module

#ifdef __cplusplus
}
#endif
