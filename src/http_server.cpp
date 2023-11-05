/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_log.h>
#include <sys/param.h>

#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "esp_event.h"
#include "esp_tls.h"

#include <M5Unified.h>

#include <esp_wifi.h>
#include <esp_system.h>

#include "http_server.hpp"
#include "metrics.hpp"


static esp_err_t hello_get_handler(httpd_req_t *req)
{
    const char* resp_str = "{\"hello\":\"world\"}";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t hello = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    .user_ctx  = NULL,
};


static esp_err_t metrics_handler(httpd_req_t *req) {
    const char* resp_body = metrics_response();
    httpd_resp_send(req, resp_body, HTTPD_RESP_USE_STRLEN);
    free((void*)resp_body);
    return ESP_OK;
}

static const httpd_uri_t metrics_uri = {
    .uri       = "/metrics",
    .method    = HTTP_GET,
    .handler   = metrics_handler,
    .user_ctx  = NULL,
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    M5.Log.printf("Starting server on port: '%d'\n", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        M5.Log.println("Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &metrics_uri);
        return server;
    }

    M5.Log.println("Error starting server!");
    return NULL;
}

esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}