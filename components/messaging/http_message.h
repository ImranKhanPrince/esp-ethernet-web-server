

#ifdef __cplusplus

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PORT 3001
#define WEB_SERVER_URL "http://192.168.1.10:3001" // Replace with your target IP and port
#define SOCKET_SERVER_IP "192.168.1.15"

extern "C"
{
#endif
  extern EventGroupHandle_t socket_event_group;
  extern int scan_msg_sock_;

  extern SemaphoreHandle_t mutex_http;
  extern bool send_json_http_message(char *message);
  extern int create_socket_connection(const char *host, uint16_t port);
  extern void start_msg_sender_task();
  extern bool send_socket_msg(int sock, const char *msg);
  extern void stop_socket_msg_task();
  extern bool write_to_sock_fifo(const char *message);

#ifdef __cplusplus
}
#endif