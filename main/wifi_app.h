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
#include <time.h>


void wifi_init_sta_ap(const char *ap_to_conn_ssid,
                      const char *ap_to_conn_password,
                      const char *esp_as_ap_ssid,
                      const char *esp_as_ap_password);
void sntp_initialize(void);

extern EventGroupHandle_t s_wifi_event_group;

/********* Weather Info Request ********* */
#define AMAP_API_KEY "c873fa5b2baf1a8f1b7b9db205c69ba0"
#define MAX_HTTP_OUTPUT_BUFFER 1024 * 4
#define MIN(x, y) ((x > y) ? y : x)
typedef enum { base, all } extensions_type;

void client_get_weather(char *city_code, extensions_type extensions);

#endif // WIFI_APP_H