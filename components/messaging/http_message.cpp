#include "http_message.h"

#include "esp_http_client.h"
#include "settings.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define SOCKET_TIMEOUT_MS 5000

#define TAG "http_message.cpp"
#define SOCKET_CLOSE_BIT BIT0

static SemaphoreHandle_t data_passed_bin_sem;
SemaphoreHandle_t mutex_http = NULL;
QueueHandle_t fifo_queue = NULL;
EventGroupHandle_t socket_event_group = NULL;
int scan_msg_sock_ = 0;

// START  STATIC METHOD SIGNATURES
static void http_client_task(void *pvParameters);
static void socket_listener_task(void *pvParameters);
// END    STATIC METHOD SIGNATURES

bool send_socket_msg(int sock, const char *message)
{
  int err = send(sock, message, strlen(message), 0);
  if (err < 0)
  {
    ESP_LOGE(TAG, "Send failed: errno %d", errno);
    return false;
  }
  return true;
}

bool write_to_sock_fifo(const char *message)
{
  if (fifo_queue == NULL)
  {
    ESP_LOGE(TAG, "FIFO queue not initialized");
    return false;
  }

  if (xQueueSend(fifo_queue, message, portMAX_DELAY) != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to send message to FIFO queue");
    return false;
  }

  return true;
}

void start_msg_sender_task()
{
  printf("start socket listener task \n");
  if (fifo_queue != NULL)
  {
    printf("Continuous Scan Socket task Already running.\n");
    return;
  }

  fifo_queue = xQueueCreate(10, sizeof(char[1024]));
  if (fifo_queue == NULL)
  {
    ESP_LOGE(TAG, "Failed to create FIFO queue");
    return;
  }

  if (socket_event_group == NULL)
  {
    socket_event_group = xEventGroupCreate();
    if (socket_event_group == NULL)
    {
      ESP_LOGE(TAG, "Failed to create event group");
      return;
    }
  }

  xTaskCreate(socket_listener_task, "socket_listener_task", 4096, NULL, 5, NULL);
}

void stop_socket_msg_task()
{
  if (socket_event_group != NULL)
  {
    xEventGroupSetBits(socket_event_group, SOCKET_CLOSE_BIT);
  }
}

int create_socket_connection(const char *host, uint16_t port)
{

  if (scan_msg_sock_ != -1)
  {
    close(scan_msg_sock_);
  }

  struct sockaddr_in dest_addr;
  struct hostent *hp;
  int sock = -1;

  // Create socket
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (sock < 0)
  {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    return -1;
  }

  // Set socket timeout
  struct timeval timeout;
  timeout.tv_sec = SOCKET_TIMEOUT_MS / 1000;
  timeout.tv_usec = (SOCKET_TIMEOUT_MS % 1000) * 1000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  // Resolve host
  hp = gethostbyname(host);
  if (!hp)
  {
    ESP_LOGE(TAG, "Could not resolve host");
    close(sock);
    return -1;
  }

  // Set up address structure
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(port);
  memcpy(&dest_addr.sin_addr, hp->h_addr, hp->h_length);

  // Connect
  int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err != 0)
  {
    ESP_LOGE(TAG, "Socket connect failed errno=%d", errno);
    close(sock);
    return -1;
  }
  else if (err == ESP_OK)
  {
    // xTaskCreate()
    // start_socket_listener_task();
  }

  return sock;
}

static void socket_listener_task(void *pvParameters)
{
  printf("socket listener task\n");
  int sock = create_socket_connection(SOCKET_SERVER_IP, PORT);
  if (sock < 0)
  {
    ESP_LOGE(TAG, "Failed to create socket connection");
    vTaskDelete(NULL);
    return;
  }

  char rx_buffer[128];
  while (true)
  {
    // int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    // if (len < 0)
    // {
    //   ESP_LOGE(TAG, "Recv failed: errno %d", errno);
    //   break;
    // }
    // else if (len == 0)
    // {
    //   ESP_LOGI(TAG, "Connection closed");
    //   break;
    // }
    // else
    // {
    //   rx_buffer[len] = 0; // Null-terminate the received data
    //   ESP_LOGI(TAG, "Received: %s", rx_buffer);
    // }

    // Check FIFO queue for messages to send
    char tx_buffer[1024];
    if (xQueueReceive(fifo_queue, &tx_buffer, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
      send_socket_msg(sock, tx_buffer);
    }
    EventBits_t bits = xEventGroupWaitBits(socket_event_group, SOCKET_CLOSE_BIT, pdTRUE, pdFALSE, 0);
    if (bits & SOCKET_CLOSE_BIT)
    {
      ESP_LOGI(TAG, "Socket close signal received");
      close(sock);
      vTaskDelete(NULL);
      break;
    }
  }

  close(sock);
  vTaskDelete(NULL);
}

bool send_json_http_message(char *message)
{
  if (strcmp(functionality_status_.data_output_loc, "none") == 0)
  {
    printf("No link available..\n");
    return false;
  }
  else if (false)
  {
    // TODO: IMPORTANT: URL formatting error condition
    return false;
  }

  data_passed_bin_sem = xSemaphoreCreateBinary();
  // if (xSemaphoreTake(mutex_http, portMAX_DELAY) == pdTRUE)
  // {
  //   xSemaphoreGive(mutex_http);
  // }

  xTaskCreate(&http_client_task, "http_client_task", 4 * 1024, (void *)message, 5, NULL);

  xSemaphoreTake(data_passed_bin_sem, portMAX_DELAY); // wait till the semaphore is released

  printf("LOG-message: %s\n", message);
  return true;
}

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

static void http_client_task(void *pvParameters)
{
  // if socket exist then send msg through socket.

  char *message = (char *)pvParameters;

  xSemaphoreGive(data_passed_bin_sem); // data copy done releasing binary semaphore

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
