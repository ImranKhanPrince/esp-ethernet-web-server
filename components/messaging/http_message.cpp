#include "http_message.h"

#include "esp_http_client.h"
#include "settings.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "global_status.h"

#define SOCKET_TIMEOUT_MS 1000

#define TAG "http_message.cpp"
#define SOCKET_CLOSE_BIT BIT0

static SemaphoreHandle_t data_passed_bin_sem;
SemaphoreHandle_t mutex_http = NULL;
QueueHandle_t fifo_queue = NULL;
EventGroupHandle_t socket_event_group = NULL;

int scan_msg_sock_ = 0;
static bool message_sender_socket_task_started = false;

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
  if (message_sender_socket_task_started)
  {
    return;
  }
  LOGI("", "start socket listener task \n");
  if (fifo_queue != NULL)
  {
    LOGI("", "Continuous Scan Socket task Already running.\n");
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
  char *data_output_loc = functionality_status_.data_output_loc;
  char host[64];
  int port = 0;

  if (data_output_loc == NULL || data_output_loc[0] == '\0')
  {
    ESP_LOGE(TAG, "Data output location is not set. Closing socket task.");
    message_sender_socket_task_started = false;
    vTaskDelete(NULL);
    return;
  }

  // Make a copy of data_output_loc because strtok modifies the string
  char *data_output_loc_copy = strdup(data_output_loc);
  if (data_output_loc_copy == NULL)
  {
    ESP_LOGE(TAG, "Failed to duplicate data_output_loc");
    message_sender_socket_task_started = false;
    vTaskDelete(NULL);
    return;
  }

  // Parse the data_output_loc
  char *protocol = strtok(data_output_loc_copy, "//"); // Remove "http://"
  if (protocol == NULL)
  {
    ESP_LOGE(TAG, "Invalid data output location format. Closing socket task.");
    message_sender_socket_task_started = false;
    free(data_output_loc_copy);
    vTaskDelete(NULL);
    return;
  }

  char *address_and_port = strtok(NULL, "/"); // Get IP address and port
  if (address_and_port == NULL)
  {
    ESP_LOGE(TAG, "Invalid data output location format. Closing socket task.");
    message_sender_socket_task_started = false;
    free(data_output_loc_copy);
    vTaskDelete(NULL);
    return;
  }

  char *address = strtok(address_and_port, ":"); // Get IP address
  if (address == NULL)
  {
    ESP_LOGE(TAG, "Invalid data output location format. Closing socket task.");
    message_sender_socket_task_started = false;
    free(data_output_loc_copy);
    vTaskDelete(NULL);
    return;
  }
  strcpy(host, address);

  char *port_str = strtok(NULL, "/"); // Get port
  if (port_str == NULL)
  {
    ESP_LOGE(TAG, "Invalid data output location format. Closing socket task.");
    message_sender_socket_task_started = false;
    free(data_output_loc_copy);
    vTaskDelete(NULL);
    return;
  }

  port = atoi(port_str); // Convert port to integer

  LOGI("", "socket listener task\n");
  LOGI("http_message", "Trying to send in %s:%d", host, port);

  int sock = create_socket_connection(host, port);
  if (sock < 0)
  {
    ESP_LOGE(TAG, "Failed to create socket connection");
    vTaskDelete(NULL);
    return;
  }
  message_sender_socket_task_started = true;
  scan_msg_sock_ = sock;

  char rx_buffer[128];
  while (true)
  {
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
      scan_msg_sock_ = -1;
      close(sock);
      message_sender_socket_task_started = false;
      vTaskDelete(NULL);
      break;
    }
  }
  scan_msg_sock_ = -1;
  close(sock);
  vTaskDelete(NULL);
}
