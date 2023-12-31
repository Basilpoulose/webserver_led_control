
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "esp_http_server.h"
#include "driver/gpio.h"

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN


#define LED_PIN 2

static const char *TAG = "esp32-web-server";
int led_state = 0;

char on_resp[] = "<!DOCTYPE html><html><head><style type=\"text/css\">html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;} h1{ color: #070812; padding: 2vh;} .button { display: inline-block; background-color: #b30000; border: none; border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;} .button2 { background-color: #364cf4; } .content { padding: 50px;} .card-grid { max-width: 800px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));} .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);} .card-title { font-size: 1.2rem; font-weight: bold; color: #034078}</style> <title>ESP32 WEB SERVER</title> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <link rel=\"icon\" href=\"data:,\"> <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\" integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\"> <link rel=\"stylesheet\" type=\"text/css\" ></head><body> <h2>ESP32 WEB SERVER</h2> <div class=\"content\"> <div class=\"card-grid\"> <div class=\"card\"> <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i> <strong>GPIO2</strong></p> <p>GPIO state: <strong> ON</strong></p> <p> <a href=\"/led2on\"><button class=\"button\">ON</button></a> <a href=\"/led2off\"><button class=\"button button2\">OFF</button></a> </p> </div> </div> </div></body></html>";

char off_resp[] = "<!DOCTYPE html><html><head><style type=\"text/css\">html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;} h1{ color: #070812; padding: 2vh;} .button { display: inline-block; background-color: #b30000; border: none; border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;} .button2 { background-color: #364cf4; } .content { padding: 50px;} .card-grid { max-width: 800px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));} .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);} .card-title { font-size: 1.2rem; font-weight: bold; color: #034078}</style> <title>ESP32 WEB SERVER</title> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <link rel=\"icon\" href=\"data:,\"> <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\" integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\"> <link rel=\"stylesheet\" type=\"text/css\" ></head><body> <h2>ESP32 WEB SERVER</h2> <div class=\"content\"> <div class=\"card-grid\"> <div class=\"card\"> <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i> <strong>GPIO2</strong></p> <p>GPIO state: <strong> OFF</strong></p> <p> <a href=\"/led2on\"><button class=\"button\">ON</button></a> <a href=\"/led2off\"><button class=\"button button2\">OFF</button></a> </p> </div> </div> </div></body></html>";




static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                    .required = false,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

esp_err_t send_web_page(httpd_req_t *req)
{
    int response;
    if (led_state == 0)
        response = httpd_resp_send(req, off_resp, HTTPD_RESP_USE_STRLEN);
    else
        response = httpd_resp_send(req, on_resp, HTTPD_RESP_USE_STRLEN);
    return response;
}

esp_err_t get_req_handler(httpd_req_t *req)
{
    return send_web_page(req);
}

esp_err_t led_on_handler(httpd_req_t *req)
{
    gpio_set_level(LED_PIN, 1);
    led_state = 1;
    return send_web_page(req);
}

esp_err_t led_off_handler(httpd_req_t *req)
{
    gpio_set_level(LED_PIN, 0);
    led_state = 0;
    return send_web_page(req);
}

httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_req_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_on = {
    .uri = "/led2on",
    .method = HTTP_GET,
    .handler = led_on_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_off = {
    .uri = "/led2off",
    .method = HTTP_GET,
    .handler = led_off_handler,
    .user_ctx = NULL
};

httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_on);
        httpd_register_uri_handler(server, &uri_off);
    }

    return server;
}
void get_and_print_ip_address() {
    esp_netif_t *netif_ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif_ap != NULL) {
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(netif_ap, &ip_info);
        printf("IP Address: %s\n", ip4addr_ntoa((const ip4_addr_t*)&ip_info.ip));
    } else {
        printf("Failed to obtain AP interface\n");
    }
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
     ESP_LOGI(TAG, "LED Control Web Server is running ... ...");

    httpd_handle_t server = setup_server();
    get_and_print_ip_address();

    while (1) {
        // In this loop, you can add any other functionality you need.
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
        

    }

    // Clean up and stop the server when necessary
    httpd_stop(server);
}
