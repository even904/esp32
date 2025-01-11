#include "wifi_init.h"
#include "esp_log.h"
#include "esp_netif_types.h"
#include <stdint.h>
#include <string.h>

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#define AP_WIFI_CHANNEL 6
#define MAX_STA_CONN 4

#define ESP_MAXIMUM_RETRY 5 // Currently set to 5, HMI can change this value

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "WiFi";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  // handle esp as STA events, HMI will be able to restart connection
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < ESP_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
  // handle esp as AP events
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
             event->aid);
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d",
             MAC2STR(event->mac), event->aid, event->reason);
  }
}

void wifi_init_sta_ap(const char *ap_to_conn_ssid,
                      const char *ap_to_conn_password,
                      const char *esp_as_ap_ssid,
                      const char *esp_as_ap_password) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Register event handler
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

  wifi_config_t wifi_config_sta = {
      .sta =
          {
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
              .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
              .sae_h2e_identifier = "",
          },
  };
  strcpy((char *)wifi_config_sta.sta.ssid, ap_to_conn_ssid);
  strcpy((char *)wifi_config_sta.sta.password, ap_to_conn_password);

  wifi_config_t wifi_config_ap = {
      .ap =
          {// .ssid = esp_as_ap_ssid,
           .ssid_len = strlen(esp_as_ap_ssid),
           .channel = AP_WIFI_CHANNEL,     // Default channel 1,6,11
                                           // .password = esp_as_ap_password,
           .max_connection = MAX_STA_CONN, // Default
           .authmode = strlen(esp_as_ap_password) == 0
                           ? WIFI_AUTH_OPEN
                           : WIFI_AUTH_WPA_WPA2_PSK},
  };
  strcpy((char *)wifi_config_ap.ap.ssid, esp_as_ap_ssid);
  strcpy((char *)wifi_config_ap.ap.password, esp_as_ap_password);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));

  ESP_LOGI(TAG,
           "wifi_init_sta_ap finished. esp_as_ap_ssid:%s esp_as_ap_password:%s "
           "AP_channel:%d",
           esp_as_ap_ssid, esp_as_ap_password, AP_WIFI_CHANNEL);

  ESP_ERROR_CHECK(esp_wifi_start());
}

// #include "esp_sntp.h"

// void initialize_sntp(void) {
//   ESP_LOGI(TAG, "Initializing SNTP...");
//   esp_sntp_stop();

//   // 设置时区为东八区（中国标准时间）
//   esp_setenv("TZ", "CST-8", 1);
//   esp_tzset();

//   // 初始化 SNTP
//   esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
//   esp_sntp_setservername(0, "pool.ntp.org");    // 可以添加更多服务器
//   sntp_set_time_sync_notification_cb(NULL); // 设置回调函数（可选）

//   // 启动 SNTP
//   esp_sntp_init();
// }

// // 在 Wi-Fi 连接成功后调用此函数
// void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
//                    void *event_data) {
//   if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//     esp_wifi_connect();
//   } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//     ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
//     ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
//     initialize_sntp(); // 在获得 IP 地址后初始化 SNTP
//   }
// }
#
void print_current_time() {
  time_t now;
  char strftime_buf[64];
  struct tm timeinfo;

  time(&now);
  // 将时区设置为中国标准时间
  setenv("TZ", "CST-8", 1);
  tzset();

  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
}