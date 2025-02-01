#include <M5GFX.h>
#include <M5Unified.h>

#include "esp_netif.h"
#include <esp_http_server.h>
#include "esp_wifi.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "const.hpp"
#include "http_server.hpp"
#include "wifi.hpp"
#include "metrics.hpp"
#include "VL53L0X.h"

#include "driver/i2c_master.h"

#include "esp_heap_trace.h"

#define NUM_RECORDS 100
static heap_trace_record_t trace_record[NUM_RECORDS]; // This buffer must be in internal RAM


#define UNDEF_RANGE UINT16_MAX

M5Canvas wifi_canvas(&M5.Lcd);
M5Canvas sensor_canvas(&M5.Lcd);

static VL53L0X range_sensor;

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);

static void draw_wifi_disconnected();

static void init_lcd();
static void draw_sensor(float_t temp);

static void sensor_loop(void *arg);
uint16_t ranger_measure(void);

static bool ranger_started = false;


static int32_t wifi_x, wifi_y, sensor_x, sensor_y;

extern "C" void app_main() {
    ESP_ERROR_CHECK( heap_trace_init_standalone(trace_record, NUM_RECORDS) );
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
  
    esp_timer_create_args_t sensor_timer_args = {
        .callback = sensor_loop,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sensor",
        .skip_unhandled_events = true,
    };
    esp_timer_handle_t sensor_timer_handle;
    ESP_ERROR_CHECK(esp_timer_create(&sensor_timer_args, &sensor_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(sensor_timer_handle, 2000000));
}

static void sensor_loop(void *arg) {
    ESP_LOGI("sensor_loop", "doing loop stuff");    
    ESP_ERROR_CHECK( heap_trace_start(HEAP_TRACE_LEAKS) );

    if (!ranger_started) {
         int i2c_master_port = 0;

        i2c_master_bus_config_t i2c_mst_config = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = GPIO_NUM_26,
            .scl_io_num = GPIO_NUM_0,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags = { 
                .enable_internal_pullup = true,
            },
        };

        i2c_master_bus_handle_t bus_handle;
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = 0x58,
            .scl_speed_hz = 100000,
        };

        i2c_master_dev_handle_t dev_handle;
        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

       
        // i2c_config_t conf = {
        //     .mode = I2C_MODE_MASTER,
        //     .sda_io_num = 0,         // select SDA GPIO specific to your project
        //     .scl_io_num = 26,         // select SCL GPIO specific to your project
        //     .sda_pullup_en = GPIO_PULLUP_ENABLE,
        //     .scl_pullup_en = GPIO_PULLUP_ENABLE,
        //     .master = {
        //         .clk_speed = 100000,  // select frequency specific to your project
        //     },
        //     .clk_flags = 0,                          // optional; you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here
        // };
        // ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
        // ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, NULL, 0, 0));
        // ESP_LOGI("main",
        // "I2C Port: %u SCL: %hhd SDA: %hhd",
        // M5.Ex_I2C.getPort(),
        // M5.Ex_I2C.getSCL(),
        // M5.Ex_I2C.getSDA());

        // ESP_ERROR_CHECK(!M5.Ex_I2C.begin(I2C_NUM_0, 0, 26));

        // ESP_LOGI("main",
        // "I2C Port: %u SCL: %hhd SDA: %hhd",
        // M5.Ex_I2C.getPort(),
        // M5.Ex_I2C.getSCL(),
        // M5.Ex_I2C.getSDA());

        // m5::I2C_Device d = m5::I2C_Device(0b0101001, 100000, &(M5.Ex_I2C));

        // range_sensor.setBus(&M5.Ex_I2C);
        ranger_started = true;
            // range_sensor.setAddress(0b0101001);
        range_sensor.setTimeout(500);
        if (!range_sensor.init())
        {
            ESP_LOGI("VL53L0X", "failed to init range sensor");
            esp_system_abort("failed to init range sensor");
        }

        range_sensor.startContinuous(2000);
        return;
    }
    uint16_t r = ranger_measure();

    ESP_ERROR_CHECK( heap_trace_stop() );
    heap_trace_dump();

    // uint16_t r = 0;
    draw_sensor(float(r)/10.0);
}

uint16_t ranger_measure(void)
{
  uint16_t range = range_sensor.readRangeContinuousMillimeters();
  if (range_sensor.timeoutOccurred())
  {
    return UNDEF_RANGE;
  }
  return range;
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

static void draw_sensor(float_t temp) {
    sensor_canvas.setCursor(0,0);
    sensor_canvas.fillScreen(BLACK);
    sensor_canvas.setTextColor(ORANGE);
    sensor_canvas.setFont(&FreeMono24pt7b);
    char temp_s[32];
    sprintf(temp_s, "%0.1f", temp /* *(9.0/5.0) + 32.0 */);
    sensor_canvas.drawCenterString(temp_s, (M5.Lcd.width() - 4)/2, 2, &FreeMono24pt7b);
    M5.Lcd.startWrite();
    sensor_canvas.pushSprite(sensor_x, sensor_y);
    M5.Lcd.endWrite();
}