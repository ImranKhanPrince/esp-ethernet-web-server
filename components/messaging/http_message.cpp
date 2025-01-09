#include "http_message.h"

#include "esp_http_client.h"
#include "settings.h"
#include "esp_log.h"

#define TAG "http_message.cpp"
#define WEB_SERVER_URL "http://192.168.1.12:3001" // Replace with your target IP and port

static SemaphoreHandle_t data_passed_bin_sem;

esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    if (evt->data_len > 0)
    {
      ESP_LOGI(TAG, "Response: %.*s", evt->data_len, (char *)evt->data);
    }
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  case HTTP_EVENT_ON_FINISH: // New case added
    ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
    break;
  case HTTP_EVENT_REDIRECT: // New case added
    ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
    break;
  default:
    ESP_LOGW(TAG, "Unhandled event: %d", evt->event_id);
    break;
  }
  return ESP_OK;
}

void http_client_task(void *pvParameters)
{
  char *message = (char *)pvParameters;

  xSemaphoreGive(data_passed_bin_sem);

  // vTaskDelay(3000 / portTICK_PERIOD_MS); // wait for some moment so that got ip from router
  // .url = WEB_SERVER_URL
  esp_http_client_config_t config = {
      .url = functionality_status_.data_output_loc,
      .timeout_ms = 3000,
      .event_handler = http_event_handler,
      .keep_alive_enable = false, // Ensures the connection is closed after use
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

  // Set HTTP POST Method
  esp_http_client_set_method(client, HTTP_METHOD_POST);

  // Set POST data
  const char *post_data = "Hello";
  esp_http_client_set_post_field(client, message, strlen(message));

  // Perform HTTP request
  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK)
  {
    ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
  }
  else
  {
    ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
  }

  // Clean up
  esp_http_client_cleanup(client);

  // Delete the task
  vTaskDelete(NULL);
}

bool send_json_http_message(char *message)
{
  data_passed_bin_sem = xSemaphoreCreateBinary();

  xTaskCreate(&http_client_task, "http_client_task", 4 * 1024, (void *)message, 5, NULL);

  xSemaphoreTake(data_passed_bin_sem, portMAX_DELAY); // wait till the semaphore is released

  printf("LOG-message: %s\n", message);
  return true;
}