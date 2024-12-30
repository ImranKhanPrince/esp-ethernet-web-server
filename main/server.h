#ifndef SERVER_H
#define SERVER_H

#include <esp_http_server.h>

// Function to start the web server

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

  void start_web_server(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // SERVER_H
