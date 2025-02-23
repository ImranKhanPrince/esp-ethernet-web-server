#include "global_status.h"
#include "driver.h"

SemaphoreHandle_t xUhfUartMutex = NULL;

device_status_t device_status_ = {
    .uhf_connected = UHF_DISCONNECTED,
    .internet_connected = INERNET_DISCONNECTED};

UHF_STATUS get_uhf_status()
{
  // don't need nvs cz each restart this changes
  return device_status_.uhf_connected;
}

bool set_uhf_status(UHF_STATUS uhf_status)
{
  device_status_.uhf_connected = uhf_status;
  return true;
}

void rtos_check_uhf_module_task(void *pvParams)
{
  while (true)
  {
    if (xSemaphoreTake(xUhfUartMutex, portMAX_DELAY) == pdTRUE)
    {
      LOGI("", "LOG: checking uhf module taks\n");
      // TODO: LATER: Do a open com port and close com port each time
      char status = GetGPI(0x01);
      // garbage value from previous command gives FF or baudrate mistake
      if (status == 0x00 || status == 0x01)
      {
        LOGI("", "\ngpi: %02X\n", status);
        LOGI("", "LOG: UHF connected\n");
        set_uhf_status(UHF_CONNECTED);
      }
      else
      {
        LOGI("", "\ngpi: %02X\n", status); // ff means -1 uint to int conversion
        LOGI("", "LOG: UHF disconnected\n");
        // TODO: MAYBE: make the gui to output a message
        set_uhf_status(UHF_DISCONNECTED);
      }
      xSemaphoreGive(xUhfUartMutex);
    }
    if (get_uhf_status() == UHF_DISCONNECTED)
    {
      vTaskDelay(1 * 1000 / portTICK_PERIOD_MS);
    }
    else
    {
      vTaskDelay(.5 * 60 * 1000 / portTICK_PERIOD_MS);
    }
  }
  vTaskDelete(NULL);
}