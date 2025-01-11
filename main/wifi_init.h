#ifndef WIFI_INIT_H
#define WIFI_INIT_H

#include "esp_wifi_types_generic.h"
#include "string.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

void wifi_init_sta_ap(const char *sta_ssid, const char *sta_password,
                      const char *ap_ssid, const char *ap_password);

#endif // WIFI_INIT_H