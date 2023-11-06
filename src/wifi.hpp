#include <stdint.h>

void wifi_init_sta(void);

typedef struct{
    uint8_t connected;
    int rssi;
    uint32_t disconnects;
} wifi_stats;

wifi_stats wifi_get_stats(void);