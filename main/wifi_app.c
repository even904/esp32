#include "wifi_app.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif_types.h"
#include "esp_sntp.h"
#include "freertos/projdefs.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR     "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#define AP_WIFI_CHANNEL 6
#define MAX_STA_CONN    4

#define ESP_MAXIMUM_RETRY 5  // Currently set to 5, HMI can change this value

static const char *TAG = "WiFi";
EventGroupHandle_t s_wifi_event_group;
static int         s_retry_num = 0;

static void sntp_initialize_task(void *pvParameters)
{
    sntp_initialize();
    vTaskDelete(NULL);  // 初始化完成后删除任务
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // handle esp as STA events, HMI will be able to restart connection
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if(s_retry_num < ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupClearBits(s_wifi_event_group, WIFI_IS_CONNECTED_BIT);
            xEventGroupClearBits(s_wifi_event_group, IP_IS_OBTAINED_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail,reason=%d", ((wifi_event_sta_disconnected_t *)event_data)->reason);
    }
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        // 异步初始化SNTP，确保不会阻塞主任务
        xTaskCreatePinnedToCore(sntp_initialize_task, "sntp_init", 2048, NULL, 5, NULL, tskNO_AFFINITY);
        // 等待一段时间确保SNTP同步完成
        vTaskDelay(pdMS_TO_TICKS(5000));  // 延迟5秒
        // 检查SNTP是否成功同步时间
        if(gettimeofday(NULL, NULL) != -1)
        {
            // 发起HTTPS请求
            esp_err_t err = client_get_weather("110101", base);
            if(err == ESP_OK)
            {
                xEventGroupSetBits(s_wifi_event_group, HTTP_GET_WEATHER_INFO_BIT);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to sync time via SNTP");
        }
        xEventGroupSetBits(s_wifi_event_group, WIFI_IS_CONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, IP_IS_OBTAINED_BIT);
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_SINCE_BOOT_BIT);
    }
    // handle esp as AP events
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        xEventGroupSetBits(s_wifi_event_group, STAION_IS_CONNECTED_BIT);
    }
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d", MAC2STR(event->mac), event->aid, event->reason);
        xEventGroupClearBits(s_wifi_event_group, STAION_IS_CONNECTED_BIT);
    }
}

void wifi_init_sta_ap(
    const char *ap_to_conn_ssid,
    const char *ap_to_conn_password,
    const char *esp_as_ap_ssid,
    const char *esp_as_ap_password
)
{
    // 1. Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Create event group
    s_wifi_event_group = xEventGroupCreate();

    // 3. Network initialization
    // 3.1 Netif initialize (TCP/IP network interface)
    ESP_ERROR_CHECK(esp_netif_init());
    // 3.2 Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    // 3.3 Initialize Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 3.4 Register event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL)
    );

    // 3.5 Config start WiFi as AP and STA
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

    ESP_LOGI(
        TAG,
        "wifi_init_sta_ap finished. esp_as_ap_ssid:%s esp_as_ap_password:%s "
        "AP_channel:%d",
        esp_as_ap_ssid,
        esp_as_ap_password,
        AP_WIFI_CHANNEL
    );

    ESP_ERROR_CHECK(esp_wifi_start());
}

static void time_sync_notification_cb(struct timeval *tv)
{
    if(tv == NULL)
    {
        ESP_LOGE(TAG, "Time synchronization failed.");
    }

    // 将时间转换为本地时间
    struct tm timeinfo;
    localtime_r(&tv->tv_sec, &timeinfo);

    // 格式化输出时间
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
}

// Avoid redefining the function, change init to initialize
void sntp_initialize(void)
{
    ESP_LOGI(TAG, "Initializing SNTP...");
    esp_sntp_stop();

    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    // Initialize SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");                      // More time servers can be added
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);  // Set callback to get time

    // Start SNTP
    esp_sntp_init();
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from
                                 // event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGW(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        // Clean the buffer in case of a new request
        if(output_len == 0 && evt->user_data)
        {
            // we are just starting to copy the output data into the use
            memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
        }
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding used
         * in this example returns binary data. However, event handler can also be
         * used in case chunked encoding is used.
         */
        if(!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            int copy_len = 0;
            if(evt->user_data)
            {
                // The last byte in evt->user_data is kept for the NULL character in
                // case of out-of-bound access.
                copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                if(copy_len)
                {
                    memcpy(evt->user_data + output_len, evt->data, copy_len);
                }
            }
            else
            {
                int content_len = esp_http_client_get_content_length(evt->client);
                if(output_buffer == NULL)
                {
                    // We initialize output_buffer with 0 because it is used by strlen()
                    // and similar functions therefore should be null terminated.
                    output_buffer = (char *)calloc(content_len + 1, sizeof(char));
                    output_len    = 0;
                    if(output_buffer == NULL)
                    {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                copy_len = MIN(evt->data_len, (content_len - output_len));
                if(copy_len)
                {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }
            output_len += copy_len;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        if(output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to
            // print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int       mbedtls_err = 0;
        esp_err_t err         = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if(err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        if(output_buffer != NULL)
        {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        esp_http_client_set_header(evt->client, "From", "user@example.com");
        esp_http_client_set_header(evt->client, "Accept", "text/html");
        esp_http_client_set_redirection(evt->client);
        break;
    }
    return ESP_OK;
}

// Root cert for amap.com, stored as a binary array in binary file
extern const char amap_com_root_cert_pem_start[] asm("_binary_amap_com_root_cert_pem_start");
extern const char amap_com_root_cert_pem_end[] asm("_binary_amap_com_root_cert_pem_end");

raw_weather_info_t raw_weather_info;

esp_err_t client_get_weather(char *city_code, extensions_type extensions)
{
    esp_err_t e;
    // Use snprintf, avoid buffer overflow
    char url_str[500];
    int  len = snprintf(
        url_str,
        sizeof(url_str),
        "https://restapi.amap.com/v3/weather/"
         "weatherInfo?city=%s&key=%s&extensions=%s",
        city_code,
        AMAP_API_KEY,
        extensions != all ? "base" : "all"
    );

    if(len >= sizeof(url_str))
    {
        ESP_LOGE(TAG, "URL too long, buffer overflow detected");
    }
    ESP_LOGI(TAG, "The request url is:%s", url_str);

    esp_http_client_config_t config = {
        .url           = url_str,
        .cert_pem      = amap_com_root_cert_pem_start,  // Root cert for amap.com
        .event_handler = _http_event_handler,
        .user_data     = raw_weather_info.raw_content,
        .timeout_ms    = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_perform(client);

    if(err == ESP_OK)
    {
        raw_weather_info.raw_content_length = (uint16_t)esp_http_client_get_content_length(client);
        ESP_LOGI(
            TAG,
            "HTTPS Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            raw_weather_info.raw_content_length
        );
        e = ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
        e = ESP_FAIL;
    }
    esp_http_client_cleanup(client);
    return e;
}