#include "metrics.hpp"
#include "const.hpp"
#include "wifi.hpp"

#include <M5Unified.h>
#include <esp_err.h>
#include "esp_heap_caps.h"

extern "C" {
    #include "prom.h"
}

void metrics_refresh(void);

prom_metric_sample * device_temp;
prom_metric_sample * wifi_rssi;
prom_metric_sample * wifi_connected;
prom_metric_sample * wifi_disconnects;

prom_gauge_t * heap_memory_bytes;
prom_metric_sample * heap_memory_bytes_free;
prom_metric_sample * heap_memory_bytes_allocated;

prom_collector_registry_t *metrics_registry;
prom_collector_t *metrics_collector;

multi_heap_info_t heap_info;

void metrics_init(void) {
  // Prometheus can't directly hit mDNS addresses, so the mDNS service-discovery plugin
  // passes it IP addresses for the instance values. We add the hostname label to all metrics
  // to avoid an unnecessarily complicated relabeling in prom.
  const char * hostname_only_labels[] = {"hostname"};
  const char * hostname_only_label_values[] = {HOSTNAME};
  
  metrics_registry = prom_collector_registry_new("sensors");
  metrics_collector = prom_collector_new("sensors");
  ESP_ERROR_CHECK(prom_collector_registry_register_collector(metrics_registry, metrics_collector));
  

  prom_metric_t * device_temp_metric = prom_gauge_new("temperature_f", "temperature in degrees farenheit", 1, hostname_only_labels);
  ESP_ERROR_CHECK(prom_collector_add_metric(metrics_collector, device_temp_metric));
  device_temp = prom_metric_sample_from_labels(device_temp_metric, hostname_only_label_values);

  prom_metric_t * wifi_connected_metric = prom_gauge_new("wifi_connected", "True (1) if wifi is connected; false(0) otherwise.", 1, hostname_only_labels);
  ESP_ERROR_CHECK(prom_collector_add_metric(metrics_collector, wifi_connected_metric));
  wifi_connected = prom_metric_sample_from_labels(wifi_connected_metric, hostname_only_label_values);

  prom_metric_t * wifi_rssi_metric = prom_gauge_new("wifi_rssi", "Wifi RSSI if connected; -1 otherwise", 1, hostname_only_labels);
  ESP_ERROR_CHECK(prom_collector_add_metric(metrics_collector, wifi_rssi_metric));
  wifi_rssi = prom_metric_sample_from_labels(wifi_rssi_metric, hostname_only_label_values);

  prom_metric_t * wifi_disconnects_metric = prom_counter_new("wifi_disconnects", "Total number of disconnects for this boot.", 1, hostname_only_labels);
  ESP_ERROR_CHECK(prom_collector_add_metric(metrics_collector, wifi_disconnects_metric));
  wifi_disconnects = prom_metric_sample_from_labels(wifi_disconnects_metric, hostname_only_label_values);

  const char * heap_memory_bytes_labels[] = {"availability", "hostname"};
  heap_memory_bytes = prom_gauge_new("heap_memory_bytes", "Describes heap memory allocation", 2, heap_memory_bytes_labels);
  ESP_ERROR_CHECK(prom_collector_add_metric(metrics_collector, (prom_metric_t *)heap_memory_bytes));

  const char *heap_memory_bytes_free_label_values[] = {"free", HOSTNAME};
  heap_memory_bytes_free = prom_metric_sample_from_labels(heap_memory_bytes, heap_memory_bytes_free_label_values);
  
  const char *heap_memory_bytes_allocated_label_values[] = {"allocated", HOSTNAME};
  heap_memory_bytes_allocated = prom_metric_sample_from_labels(heap_memory_bytes, heap_memory_bytes_allocated_label_values);
}

const char * metrics_response(void) {
    // It's not really clear how to use a custom collector function w/o access
    // to the private `metrics` struct member or the default collector func. So
    // I'm just going to refresh the metric samples here before generating the
    // output.
    metrics_refresh();
    return prom_collector_registry_bridge(metrics_registry);
}

void metrics_refresh(void) {
  float_t temp;
  M5.Imu.getTemp(&temp);
  prom_metric_sample_set(device_temp, double(temp*(9.0/5.0) + 32.0));

  wifi_stats ws = wifi_get_stats();
  prom_metric_sample_set(wifi_rssi, double(ws.rssi));
  prom_metric_sample_set(wifi_connected, double(ws.connected));
  prom_metric_sample_set(wifi_disconnects, double(ws.disconnects));

  heap_caps_get_info(&heap_info, MALLOC_CAP_8BIT|MALLOC_CAP_32BIT);
  prom_metric_sample_set(heap_memory_bytes_free, double(heap_info.total_free_bytes));
  prom_metric_sample_set(heap_memory_bytes_allocated, double(heap_info.total_allocated_bytes));
}
