#include "server.h"

#include "esp_http_server.h"
#include "esp_log.h"

#include <stddef.h> // Add this include for size_t
#include "math.h"

#include "api_json.h"
#include "global_status.h"
#define KILOBYTE 1024

#define HTTPD_TASK_SCAN_SIZE 15 * KILOBYTE
static const char *TAG = "HTTP_SERVER";

extern const uint8_t _binary_html_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t _binary_html_index_html_end[] asm("_binary_index_html_end");

extern const uint8_t _binary_html_assets_styles_style_css_start[] asm("_binary_style_css_start");
extern const uint8_t _binary_html_assets_styles_style_css_end[] asm("_binary_style_css_end");

extern const uint8_t _binary_html_assets_scripts_app_js_start[] asm("_binary_app_js_start");
extern const uint8_t _binary_html_assets_scripts_app_js_end[] asm("_binary_app_js_end");

extern const uint8_t _binary_html_assets_images_favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t _binary_html_assets_images_favicon_ico_end[] asm("_binary_favicon_ico_end");

// CONTROLLERS if gets bigger in size then move to a separate component
esp_err_t handle_root(httpd_req_t *req)
{
  const size_t index_html_len = _binary_html_index_html_end - _binary_html_index_html_start;
  httpd_resp_send(req, (const char *)_binary_html_index_html_start, index_html_len);
  return ESP_OK;
}

esp_err_t handle_css(httpd_req_t *req)
{
  const size_t style_css_len = _binary_html_assets_styles_style_css_end - _binary_html_assets_styles_style_css_start;
  httpd_resp_set_type(req, "text/css");
  httpd_resp_send(req, (const char *)_binary_html_assets_styles_style_css_start, style_css_len);
  return ESP_OK;
}

esp_err_t handle_js(httpd_req_t *req)
{
  const size_t app_js_len = _binary_html_assets_scripts_app_js_end - _binary_html_assets_scripts_app_js_start;
  httpd_resp_set_type(req, "application/javascript");
  httpd_resp_send(req, (const char *)_binary_html_assets_scripts_app_js_start, app_js_len);
  return ESP_OK;
}

esp_err_t handle_favicon(httpd_req_t *req)
{
  const size_t favicon_ico_len = _binary_html_assets_images_favicon_ico_end - _binary_html_assets_images_favicon_ico_start;
  httpd_resp_set_type(req, "image/x-icon");
  httpd_resp_send(req, (const char *)_binary_html_assets_images_favicon_ico_start, favicon_ico_len);
  return ESP_OK;
}

// TODO: IMPORTANT: all the settings realted things verify the key and encryption first if matches then do any operation. else sen 404
esp_err_t handle_api_root(httpd_req_t *req)
{
  char *json_response[20];
  status_view(json_response, sizeof(json_response));
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, json_response, strlen(json_response));
  return ESP_OK;
}

esp_err_t handle_get_settings(httpd_req_t *req)
{

  if (get_uhf_status() == UHF_DISCONNECTED)
  {
    httpd_resp_set_type(req, "application/json");
    char *response = strdup("{\"error\":\"Couldn't communicate with RF. Please restart the power.\"}\n");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }

  char *json_response = get_settings();
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, json_response, strlen(json_response));
  return ESP_OK;
}

esp_err_t handle_post_settings(httpd_req_t *req)
{
  /* Destination buffer for content of HTTP POST request.
   * httpd_req_recv() accepts char* only, but content could
   * as well be any binary data (needs type casting).
   * In case of string data, null termination will be absent, and
   * content length would give length of string */

  // TODO: LATER: find way better way to allocate the buffer

  char content[req->content_len];

  if (req->content_len > 1000)
  {
    ESP_LOGE(TAG, "Content length is too large");
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int ret = httpd_req_recv(req, content, req->content_len);
  if (ret <= 0)
  { /* 0 return value indicates connection closed */
    /* Check if timeout occurred */
    if (ret == HTTPD_SOCK_ERR_TIMEOUT)
    {
      /* In case of timeout one can choose to retry calling
       * httpd_req_recv(), but to keep it simple, here we
       * respond with an HTTP 408 (Request Timeout) error */
      httpd_resp_send_408(req);
    }
    /* In case of error, returning ESP_FAIL will
     * ensure that the underlying socket is closed */
    return ESP_FAIL;
  }

  /* Send a simple response */

  httpd_resp_set_type(req, "application/json");
  char *resp = set_settings(content);
  if (resp == NULL)
  {
    // TODO: MAYBE: sends a 400 when the json is error msg
    //  Send the complete response in one go
    httpd_resp_send(req, "{\"error\":\"Invalid JSON\"}\n", HTTPD_RESP_USE_STRLEN);

    // Return ESP_OK to indicate the response was handled
    return ESP_OK;
  }
  httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t handle_post_func_settings(httpd_req_t *req)
{
  httpd_resp_set_type(req, "application/json");
  char content[req->content_len];

  if (req->content_len > 1000)
  {
    ESP_LOGE(TAG, "Content length is too large");
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int ret = httpd_req_recv(req, content, req->content_len);
  if (ret <= 0)
  {
    if (ret == HTTPD_SOCK_ERR_TIMEOUT)
    {
      httpd_resp_send_408(req);
    }
    return ESP_FAIL;
  }

  char *resp = set_func_settings(content);
  if (resp == NULL)
  {
    httpd_resp_send(req, "{\"error\":\"Invalid JSON\"}\n", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }
  httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t handle_get_func_settings(httpd_req_t *req)
{
  const char *json_response = get_json_device_func_settings();
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, json_response, strlen(json_response));
  return ESP_OK;
}

esp_err_t handle_post_scan(httpd_req_t *req)
{
  httpd_resp_set_type(req, "application/json");

  int content_len = req->content_len;
  char data[content_len];
  if (content_len > 500)
  {
    ESP_LOGE(TAG, "Content length is too large");
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int ret = httpd_req_recv(req, data, content_len);

  if (ret <= 0)
  {
    if (ret == HTTPD_SOCK_ERR_TIMEOUT)
    {
      httpd_resp_send_408(req);
    }
    return ESP_FAIL;
  }
  // PASSING THE CONTROL TO VIEW
  char *response = handle_scan_command(data);
  printf("DEBUG: got out of the scan command\n");

  httpd_resp_send(req, response, strlen(response));

  return ESP_OK;
}

esp_err_t handle_memory_commands(httpd_req_t *req)
{
  httpd_resp_set_type(req, "application/json");

  int content_length = req->content_len;
  if (content_length > 500)
  {
    ESP_LOGE(TAG, "Content length is too large");
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  char data[content_length];
  int stat = httpd_req_recv(req, data, content_length);
  if (stat <= 0)
  {
    httpd_resp_send_408(req);
    return ESP_FAIL;
  }
  char *response = view_handle_memory_command(data);
  httpd_resp_send(req, response, strlen(response));
  return ESP_OK;
}

// TODO: LATER: Each of the endpoint([GET] /api/settings [POST] /api/settings) need a documentation that tells what is the key point like the link need to math exact etc etc
// TODO: IMPORTANT: handle api if the reader is not connected
// TODO: IMPORTANT: authenticate like a default pass like router. user can use that pass to set settings nad encryption key however if they want to change the password then need encryption[] must.
// TODO: IMPORTANT: Encryption will be applied to total buffer and it will be decrypted in the view-component.

void start_web_server()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 12;
  config.max_resp_headers = 12;
  config.stack_size = HTTPD_TASK_SCAN_SIZE;
  httpd_handle_t server = NULL;

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

  //=================== ROUTER STAYS HERE================================= No formward slash in the links
  if (httpd_start(&server, &config) == ESP_OK)
  {
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = handle_root};
    httpd_register_uri_handler(server, &root_uri);

    // if root api is hit it will return the status
    httpd_uri_t api_root_uri = {
        .uri = "/api",
        .method = HTTP_GET,
        .handler = handle_api_root};
    httpd_register_uri_handler(server, &api_root_uri);

    httpd_uri_t api_get_settings_uri = {
        .uri = "/api/settings",
        .method = HTTP_GET,
        .handler = handle_get_settings};
    httpd_register_uri_handler(server, &api_get_settings_uri);

    httpd_uri_t api_post_settings_uri = {
        .uri = "/api/settings",
        .method = HTTP_POST,
        .handler = handle_post_settings};
    httpd_register_uri_handler(server, &api_post_settings_uri);

    httpd_uri_t api_post_func_settings_uri = {
        .uri = "/api/func_settings",
        .method = HTTP_POST,
        .handler = handle_post_func_settings};
    httpd_register_uri_handler(server, &api_post_func_settings_uri);

    httpd_uri_t api_get_func_settings_uri = {
        .uri = "/api/func_settings",
        .method = HTTP_GET,
        .handler = handle_get_func_settings};
    httpd_register_uri_handler(server, &api_get_func_settings_uri);

    httpd_uri_t api_post_scan_uri = {
        .uri = "/api/scan",
        .method = HTTP_POST,
        .handler = handle_post_scan};
    httpd_register_uri_handler(server, &api_post_scan_uri);

    httpd_uri_t api_mem_commands_uri = {
        .uri = "/api/memory",
        .method = HTTP_POST,
        .handler = handle_memory_commands};
    httpd_register_uri_handler(server, &api_mem_commands_uri);

    httpd_uri_t css_uri = {
        .uri = "/assets/styles/style.css",
        .method = HTTP_GET,
        .handler = handle_css};
    httpd_register_uri_handler(server, &css_uri);

    httpd_uri_t js_uri = {
        .uri = "/assets/scripts/app.js",
        .method = HTTP_GET,
        .handler = handle_js};
    httpd_register_uri_handler(server, &js_uri);

    httpd_uri_t favicon_uri = {
        .uri = "/assets/images/favicon.ico",
        .method = HTTP_GET,
        .handler = handle_favicon};
    httpd_register_uri_handler(server, &favicon_uri);
  }
  else
  {
    ESP_LOGE(TAG, "Failed to start server!");
  }
}
