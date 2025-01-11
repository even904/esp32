#ifndef WIFI_INIT_H
#define WIFI_INIT_H

#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
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
void log_current_time();
void sntp_initialize(void);

#endif // WIFI_INIT_H