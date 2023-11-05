#include <M5GFX.h>
#include <M5Unified.h>

#include "esp_netif.h"
#include <esp_http_server.h>
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "const.hpp"
#include "http_server.hpp"
#include "wifi.hpp"
#include "metrics.hpp"

M5Canvas wifi_canvas(&M5.Lcd);
M5Canvas sensor_canvas(&M5.Lcd);

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);

static void draw_wifi_disconnected();

static void init_lcd();
static void draw_sensor(float_t temp);


static int32_t wifi_x, wifi_y, sensor_x, sensor_y;

extern "C" void app_main() {
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    metrics_init();

    auto cfg = M5.config();
    M5.begin(cfg);

    init_lcd();
    draw_wifi_disconnected();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_STA_DISCONNECTED,
                                                        &disconnect_handler,
                                                        &server,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &connect_handler,
                                                        &server,
                                                        NULL));

    wifi_init_sta();

    float_t temp;
    M5.Imu.getTemp(&temp);

    draw_sensor(temp);
}



static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    draw_wifi_disconnected();
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        M5.Log.println("Stopping webserver");
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            M5.Log.println("Failed to stop http server");
        }
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = start_webserver();
    }

    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

    wifi_canvas.fillScreen(DARKGREY);
    wifi_canvas.setCursor(2, 2);

    wifi_canvas.setTextColor(WHITE);
    wifi_canvas.print("Wifi: ");
    wifi_canvas.setTextColor(GREEN);
    wifi_canvas.println(WIFI_SSID);
    wifi_canvas.setTextColor(WHITE);
    wifi_canvas.print("IPv4: ");
    wifi_canvas.setTextColor(GREEN);
    wifi_canvas.printf(IPSTR, IP2STR(&event->ip_info.ip));

    M5.Lcd.startWrite();
    wifi_canvas.pushSprite(2, 2);
    M5.Lcd.endWrite();
}

static void init_lcd() {
    M5.Lcd.setRotation(0);
    M5.Lcd.fillScreen(BLACK);
    wifi_x = 2;
    wifi_y = 2;
    wifi_canvas.createSprite(M5.Lcd.width() - 4, wifi_canvas.fontHeight()*2 + 4);
    sensor_x = 2;
    sensor_y = wifi_canvas.height() + wifi_x + 2;
    sensor_canvas.createSprite(M5.Lcd.width() - 4, sensor_canvas.fontHeight(&FreeMono24pt7b)*1 + 4);
    sensor_canvas.fillScreen(BLACK);
    
    M5.Lcd.startWrite();
    sensor_canvas.pushSprite(sensor_x, sensor_y);
    M5.Lcd.endWrite();
}

static void draw_wifi_disconnected() {
    wifi_canvas.fillScreen(DARKGREY);
    wifi_canvas.setCursor(2, 2);

    wifi_canvas.setTextColor(WHITE);
    wifi_canvas.print("Wifi: ");
    wifi_canvas.setTextColor(RED);
    wifi_canvas.println(WIFI_SSID);
    wifi_canvas.setTextColor(WHITE);
    wifi_canvas.print("Ipv4: ");
    wifi_canvas.setTextColor(RED);
    wifi_canvas.println("disconnected");

    M5.Lcd.startWrite();
    wifi_canvas.pushSprite(2, 2);
    M5.Lcd.endWrite();
}

static void draw_sensor(float_t temp) {
    sensor_canvas.setCursor(0,0);
    sensor_canvas.fillScreen(BLACK);
    sensor_canvas.setTextColor(ORANGE);
    sensor_canvas.setFont(&FreeMono24pt7b);
    char temp_s[32];
    sprintf(temp_s, "%0.1f", temp*(9.0/5.0) + 32.0);
    sensor_canvas.drawCenterString(temp_s, (M5.Lcd.width() - 4)/2, 2, &FreeMono24pt7b);
    M5.Lcd.startWrite();
    sensor_canvas.pushSprite(sensor_x, sensor_y);
    M5.Lcd.endWrite();
}