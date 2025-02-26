#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "ethernet_init.h"
#include "sdkconfig.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include <esp_http_server.h>

#include "server.h"
#include "driver.h"
#include "global_status.h"
#include "socket_message.h"
#include "settings.h"
#include "scan_task.h"
#include "sdmmc_helper.h"

#define UHF_MODULE_CHECK_TASK_SIZE 4 * 1024
// TODO: this will come from CMAKELIST define;
#ifdef CMAKE_SERIAL_NO
#define SERIAL_NO CMAKE_SERIAL_NO
#else
#define SERIAL_NO "J4221-0001"
#endif

#ifdef CMAKE_DEFAULT_PASS
#define DEFAULT_PASS CMAKE_DEFAULT_PASS
#else
#define DEFAULT_PASS "0123456789"
#endif

static const char *TAG = "APP_MAIN";

credentials_t credentials_ = {
    .serial_num = SERIAL_NO,
    .default_pass = DEFAULT_PASS};

static void net_down_handler()
{
    stop_socket_msg_task();
}

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        if (scan_info_.scan_mode == SCAN_CONTINUOUS)
        {
            net_down_handler();
        }
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");

    LOGI("", "Starting web server...\n");
    start_web_server(); // For the api endpoints
    if (scan_info_.scan_mode == SCAN_CONTINUOUS)
    {
        start_msg_sender_task(); // for the sockets
        start_cont_scan(scan_info_.filter, scan_info_.offset, scan_info_.value);
    }
}

static void lost_ip_event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data)
{
    LOGI("", "IP LOST CHECK CONNECTION\n");
    if (scan_info_.scan_mode == SCAN_CONTINUOUS)
    {
        net_down_handler();
    }
}

extern "C" void app_main(void)
{

    uint32_t restart_counter = load_increment_store_restart_counter_till_last_flash();
    LOGI("", "restart_counter: %ld\n", restart_counter);
    if (restart_counter <= 1)
    {
        strcpy(credentials_.current_pass, credentials_.default_pass);
        store_serial_number_and_default_password(credentials_.serial_num,
                                                 credentials_.current_pass,
                                                 credentials_.default_pass);
    }

    load_serial_number_and_default_password(credentials_.serial_num, sizeof(credentials_.serial_num),
                                            credentials_.current_pass, sizeof(credentials_.current_pass),
                                            credentials_.default_pass, sizeof(credentials_.default_pass));

    // when loading the firmware first time uncomment this line

    // Load Saved Settings
    nvs_init();
    get_nvs_func_settings(&functionality_status_);
    nvs_load_scan_mode();

    // Initialize TCP/IP and create default event loop
    ESP_ERROR_CHECK(esp_netif_init());                // INTERNET INITIALIZATION
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // EVENT LOOP INITIALIZED

    // Initialize Ethernet driver
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(eth_init(&eth_handles, &eth_port_cnt)); // ETHERNET

    // Create instance(s) of esp-netif for Ethernet(s)
    if (eth_port_cnt == 1)
    {
        // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
        // default esp-netif configuration parameters.
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH(); // NETIF FOR ETHERNET GET CFG
        esp_netif_t *eth_netif = esp_netif_new(&cfg);     // NETIF FOR ETHERNET
        // Attach Ethernet driver to TCP/IP stack
        ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[0]))); // ETH NETIF ATTACHED
        ESP_ERROR_CHECK(esp_eth_start(eth_handles[0]));
        // for (int i = 0; i < eth_port_cnt; i++)
        // {
        //     ESP_ERROR_CHECK(esp_eth_start(eth_handles[i]));
        // }
    }

    // --- Initialize and Start WiFi ---
    // Create default Wi-Fi station (STA) netif
    esp_netif_t *wifi_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, "SOALIB2", sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, "bangladesh123", sizeof(wifi_config.sta.password) - 1);
    // Set WiFi to station mode and start the WiFi driver
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &wifi_config));
    // (Optionally, configure your SSID/password via wifi_config_t.)

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));

    // --- Register IP and Lost IP Handlers for both interfaces ---
    // These callbacks will be invoked regardless of whether IP was acquired via Ethernet or WiFi.
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_LOST_IP, &lost_ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &lost_ip_event_handler, NULL));

    // Start Ethernet driver state machine

    xUhfUartMutex = xSemaphoreCreateMutex();

    // start uhf module at last bcz it takes 1s to start once powered
    if (xSemaphoreTake(xUhfUartMutex, portMAX_DELAY) == pdTRUE)

    {
        bool connected = false;
        connected = OpenComPort("com3", 57600);

        if (connected)
        {
            LOGI("", "Successfully connected with 57600 baud\n");
            set_uhf_status(UHF_CONNECTED);
        }
        else
        {
            CloseComPort();
            connected = OpenComPort("com3", 115200);
            if (!connected)
            {
                CloseComPort();
                LOGI("", "Failed to open com port!\n");
                set_uhf_status(UHF_DISCONNECTED);
            }
            else
            {
                LOGI("", "Successfuly connected with 115200 baud\n");
                set_uhf_status(UHF_CONNECTED);
            }
        }
        xSemaphoreGive(xUhfUartMutex);
    }

    xTaskCreate(rtos_check_uhf_module_task, "check uhf module task", UHF_MODULE_CHECK_TASK_SIZE, NULL, 1, NULL);
    // int n = Inventory(false);
    // LOGI("","n=%d\n", n);

    print_device_func_settings(&functionality_status_);

    // TODO: LATER: WAit for 30s and see if internet is connected or not
    sd_init_mount("/sdcard");
    char buf[1000];
    sd_read_json_file("/sdcard/setup_info.txt", buf, sizeof(buf));
}
