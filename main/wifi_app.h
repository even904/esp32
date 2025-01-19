#ifndef WIFI_APP_H
#define WIFI_APP_H

#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_tls.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "nvs_flash.h"
#include "string.h"
#include "sys/time.h"
#include <stdint.h>
#include <time.h>

// Event group bit definitions, expand if needed
#define WIFI_IS_CONNECTED_BIT         BIT0  // 1: connected, 0: disconnected
#define IP_IS_OBTAINED_BIT            BIT1  // 1: IP obtained, 0: IP not obtained
#define SNTP_INITIALIZED_BIT          BIT2  // 1: SNTP initialized, 0: SNTP not initialized
#define WIFI_CONNECTED_SINCE_BOOT_BIT BIT3  // 1: WiFi connected since boot, 0: WiFi not connected since boot

#define STAION_IS_CONNECTED_BIT   BIT10  // 1: Station joined, 0: Station leave
#define HTTP_GET_WEATHER_INFO_BIT BIT11  // 1: Got weather info, 0: No weather info got
// #define UPDATE_WEATHER_INFO_REQUEST_BIT BIT12 // 1: Need to update, 0: No request

/******* WiFi Init and SNTP Sync ******* */
void wifi_init_sta_ap(
    const char *ap_to_conn_ssid,
    const char *ap_to_conn_password,
    const char *esp_as_ap_ssid,
    const char *esp_as_ap_password
);
void sntp_initialize(void);

extern EventGroupHandle_t s_wifi_event_group;

/********* Weather Info Request ********* */
#define AMAP_API_KEY           "c873fa5b2baf1a8f1b7b9db205c69ba0"
#define MAX_HTTP_OUTPUT_BUFFER 512
#define MIN(x, y)              ((x > y) ? y : x)

typedef enum
{
    base,
    all
} extensions_type;

typedef struct
{
    char    raw_content[MAX_HTTP_OUTPUT_BUFFER];
    uint16_t raw_content_length;
} raw_weather_info_t;

extern raw_weather_info_t raw_weather_info;
esp_err_t         client_get_weather(char *city_code, extensions_type extensions);

#endif  // WIFI_APP_H