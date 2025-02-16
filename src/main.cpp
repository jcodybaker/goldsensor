#include <M5GFX.h>
#include <M5Unified.h>

#include <esp_netif.h>
#include <esp_http_server.h>
#include <esp_wifi.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <esp_log.h>
// #include <esp_heap_trace.h>

#include "const.hpp"
#include "http_server.hpp"
#include "wifi.hpp"
#include "metrics.hpp"
#include "ranger.hpp"

#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include <malloc.h>

// #define NUM_HEAP_DEBUG_RECORDS 100
// static heap_trace_record_t trace_record[NUM_HEAP_DEBUG_RECORDS]; // This buffer must be in internal RAM

M5Canvas wifi_canvas(&M5.Lcd);
M5Canvas sensor_canvas(&M5.Lcd);

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);

static void draw_wifi_disconnected();

static void init_lcd();
static void draw_sensor(VL53L0X_RangingMeasurementData_t *data);
static void draw_error(const char *msg, const char *detail);
static void sensor_loop(void *arg);

static int32_t wifi_x, wifi_y, sensor_x, sensor_y;

static VL53L0X_Dev_t *ranger_device;

const uint16_t max_range_mm = 2000;

extern "C" void app_main() {
    // ESP_ERROR_CHECK( heap_trace_init_standalone(trace_record, NUM_HEAP_DEBUG_RECORDS) );
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
    M5.Ex_I2C.release();
    M5.Ex_I2C.begin(I2C_NUM_0, 0, 26);
    printf("M5.Ex_I2C.port = %d, SDA %d, SCL %d \n", M5.Ex_I2C.getPort(), M5.Ex_I2C.getSDA(), M5.Ex_I2C.getSCL());
    printf("M5.In_I2C.port = %d, SDA %d, SCL %d\n", M5.In_I2C.getPort(), M5.In_I2C.getSDA(), M5.In_I2C.getSCL());
    
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

    ranger_device = ranger_init();
  
    esp_timer_create_args_t sensor_timer_args = {
        .callback = sensor_loop,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sensor",
        .skip_unhandled_events = true,
    };
    esp_timer_handle_t sensor_timer_handle;
    ESP_ERROR_CHECK(esp_timer_create(&sensor_timer_args, &sensor_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(sensor_timer_handle, 500000));
}

static void sensor_loop(void *arg) {
    // ESP_ERROR_CHECK( heap_trace_start(HEAP_TRACE_LEAKS) );
    VL53L0X_RangingMeasurementData_t measurement;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    Status = ranger_measure(ranger_device, &measurement);
    // ESP_ERROR_CHECK( heap_trace_stop() );
    // heap_trace_dump();
    if (Status == VL53L0X_ERROR_NONE) {
        if (measurement.RangeMilliMeter < max_range_mm) {
            draw_sensor(&measurement);
        } else {
            draw_error("ERROR", "max range");
        }
    } else {
        char buf_s[32];
        if (VL53L0X_GetPalErrorString(Status, buf_s) == VL53L0X_ERROR_NONE) {
            draw_error("ERROR", buf_s);
        } else {
            draw_error("ERROR", "unknown");
        }
    }
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
    wifi_canvas.println("");
    wifi_canvas.setTextColor(WHITE);
    wifi_canvas.print("Host:");
    wifi_canvas.println(HOSTNAME);

    M5.Lcd.startWrite();
    wifi_canvas.pushSprite(2, 2);
    M5.Lcd.endWrite();
}

static void init_lcd() {
    M5.Lcd.setRotation(0);
    M5.Lcd.fillScreen(BLACK);
    wifi_x = 2;
    wifi_y = 2;
    wifi_canvas.createSprite(M5.Lcd.width() - 4, wifi_canvas.fontHeight()*3 + 4);
    sensor_x = 2;
    sensor_y = wifi_canvas.height() + wifi_x + 2;
    sensor_canvas.createSprite(M5.Lcd.width() - 4, sensor_canvas.fontHeight(&FreeMono24pt7b)+sensor_canvas.fontHeight(&FreeMono12pt7b) + 6);
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
    wifi_canvas.print("IPv4: ");
    wifi_canvas.setTextColor(RED);
    wifi_canvas.println("disconnected");
    wifi_canvas.setTextColor(WHITE);
    wifi_canvas.print("Host:");
    wifi_canvas.println(HOSTNAME);

    M5.Lcd.startWrite();
    wifi_canvas.pushSprite(2, 2);
    M5.Lcd.endWrite();
}

static void draw_sensor(VL53L0X_RangingMeasurementData_t *data) {
    sensor_canvas.setCursor(0,0);
    sensor_canvas.fillScreen(BLACK);
    sensor_canvas.setTextColor(ORANGE);
    sensor_canvas.setFont(&FreeMono24pt7b);
    char buf_s[32];
    sprintf(buf_s, "%0.1f", float(data->RangeMilliMeter) / 10.0);
    sensor_canvas.drawCenterString(buf_s, (M5.Lcd.width() - 4)/2, 2, &FreeMono24pt7b);
    sensor_canvas.setTextColor(LIGHTGREY);
    // SignalRateRtnMegaCps measures reflectivity it's a fixed 16-bit/16-bit number
    sprintf(buf_s, "%0.1f%%", float(data->SignalRateRtnMegaCps) / (float(0xFFFFFF) / 100));
    sensor_canvas.drawCenterString(buf_s, (M5.Lcd.width() - 4)/2, 4+sensor_canvas.fontHeight(&FreeMono24pt7b), &FreeMono12pt7b);
    M5.Lcd.startWrite();
    sensor_canvas.pushSprite(sensor_x, sensor_y);
    M5.Lcd.endWrite();
}

static void draw_error(const char *msg, const char *detail) {
    sensor_canvas.setCursor(0,0);
    sensor_canvas.fillScreen(BLACK);
    sensor_canvas.setTextColor(RED);
    sensor_canvas.setFont(&FreeMono24pt7b);
    sensor_canvas.drawCenterString(msg, (M5.Lcd.width() - 4)/2, 2, &FreeMono24pt7b);

    sensor_canvas.setTextColor(LIGHTGREY);
    // SignalRateRtnMegaCps measures reflectivity it's a fixed 16-bit/16-bit number
    sensor_canvas.drawCenterString(detail, (M5.Lcd.width() - 4)/2, 4+sensor_canvas.fontHeight(&FreeMono24pt7b), &FreeMono12pt7b);

    M5.Lcd.startWrite();
    sensor_canvas.pushSprite(sensor_x, sensor_y);
    M5.Lcd.endWrite();
}