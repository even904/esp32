#include "hmi_app.h"
#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi_types_generic.h"
#include "nvs.h"
#include "wifi_app.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char *TAG              = "HMI";
const char *NVS_DV_NameSpace = "device_info";

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

static void device_info_update(
    device_info_t *info,        //
    char          *ssid,        //
    char          *passwd,      //
    char          *api_key,     //
    char          *city_code,   //
    int            conn_retry,  //
    int            bg_image     //
);
esp_err_t nvs_load_info_str(char *key, char *destination);
esp_err_t nvs_load_info_int(char *key, int *val);

device_info_t device_info;

void device_info_init(
    device_info_t *info,        //
    char          *ssid,        //
    char          *passwd,      //
    char          *api_key,     //
    char          *city_code,   //
    int            conn_retry,  //
    int            bg_image     //
)
{
    // Clearing the info structure to ensure we have a clean state
    memset(info, 0, sizeof(device_info_t));
    // 如果不是全片擦除，但NVS设置信息有误，可以调用此函数以便在烧录后重新设置一次正常的wifi连接
    // device_info_update(info,ssid,passwd,api_key,city_code,conn_retry,bg_image);
    // Load ssid
    if(nvs_load_info_str("ssid", info->ssid) != ESP_OK)
    {
        snprintf(info->ssid, MAX_SSID_STR_LEN, "%s", ssid);
        ESP_LOGW(TAG, "Failed to load ssid from NVS, using default: %s", ssid);
    }
    // Load passwd
    if(nvs_load_info_str("passwd", info->passwd) != ESP_OK)
    {
        snprintf(info->passwd, MAX_PASSWD_STR_LEN, "%s", passwd);
        ESP_LOGW(TAG, "Failed to load passwd from NVS, using default: %s", passwd);
    }
    // Load api_key
    if(nvs_load_info_str("api_key", info->api_key) != ESP_OK)
    {
        snprintf(info->api_key, MAX_API_KEY_STR_LEN, "%s", api_key);
        ESP_LOGW(TAG, "Failed to load api_key from NVS, using default: %s", api_key);
    }
    // Load city_code
    if(nvs_load_info_str("city_code", info->city_code) != ESP_OK)
    {
        snprintf(info->city_code, MAX_CITY_CODE_STR_LEN, "%s", city_code);
        ESP_LOGW(TAG, "Failed to load city_code from NVS, using default: %s", city_code);
    }
    // Load conn_retry
    if(nvs_load_info_int("conn_retry", &info->conn_retry) != ESP_OK)
    {
        info->conn_retry = conn_retry;
        ESP_LOGW(TAG, "Failed to load conn_retry from NVS, using default: %d", conn_retry);
    }
    // Load bg_image
    if(nvs_load_info_int("bg_image", &info->bg_image) != ESP_OK)
    {
        info->bg_image = bg_image;
        ESP_LOGW(TAG, "Failed to load bg_image from NVS, using default: %d", bg_image);
    }
    local_city_code_update(info->city_code);
    local_api_key_update(info->api_key);
    device_update_nvs_info(info);
}

// Only be called after saving web configuration, and hard set nvs.
static void device_info_update(
    device_info_t *info,        //
    char          *ssid,        //
    char          *passwd,      //
    char          *api_key,     //
    char          *city_code,   //
    int            conn_retry,  //
    int            bg_image     //
)
{
    // Update the structure members with new data
    snprintf(info->ssid, MAX_SSID_STR_LEN, "%s", ssid);
    snprintf(info->passwd, MAX_PASSWD_STR_LEN, "%s", passwd);
    snprintf(info->api_key, MAX_API_KEY_STR_LEN, "%s", api_key);
    snprintf(info->city_code, MAX_CITY_CODE_STR_LEN, "%s", city_code);
    info->conn_retry = conn_retry;
    info->bg_image   = bg_image;
}

esp_err_t device_update_nvs_info(device_info_t *info)
{
    nvs_handle_t my_handle;
    esp_err_t    ret = nvs_open(NVS_DV_NameSpace, NVS_READWRITE, &my_handle);
    if(ret != ESP_OK)
    {
        ret = ESP_FAIL;
    }
    else
    {
        esp_err_t err = nvs_set_str(my_handle, "ssid", info->ssid);
        err |= nvs_set_str(my_handle, "passwd", info->passwd);
        err |= nvs_set_str(my_handle, "city_code", info->city_code);
        err |= nvs_set_str(my_handle, "api_key", info->api_key);
        err |= nvs_set_i32(my_handle, "conn_retry", info->conn_retry);
        err |= nvs_set_i32(my_handle, "bg_image", info->bg_image);
        if(err != ESP_OK)
        {
            ESP_LOGW(TAG, "Something went wrong while writing device info to nvs");
        }
        else
        {
            nvs_commit(my_handle);
            printf(
                "New Device Info:\nSSID:%s\nPasswd:%s\nCitycode:%s\nApi_key:%s\nConn_retry:%d\nBg_imgae:%d\n",
                info->ssid,
                info->passwd,
                info->city_code,
                info->api_key,
                info->conn_retry,
                info->bg_image
            );
        }
        ret = err;
    }
    return ret;
}

esp_err_t nvs_load_info_str(char *key, char *destination)
{
    nvs_handle_t my_handle;
    esp_err_t    ret = nvs_open(NVS_DV_NameSpace, NVS_READWRITE, &my_handle);
    if(ret != ESP_OK)
    {
        ret = ESP_FAIL;
    }
    else
    {
        size_t length = -1;
        if(nvs_get_str(my_handle, key, destination, &length) == ESP_OK)
        {
            ESP_LOGI(TAG, "Got \'%s\' from nvs!", key);
            ret = ESP_OK;
        }
        else
        {
            ESP_LOGW(TAG, "\'%s\' not found in nvs, try writing value first", key);
            ret = ESP_FAIL;
        }
    }
    return ret;
}

esp_err_t nvs_load_info_int(char *key, int *val)
{
    nvs_handle_t my_handle;
    esp_err_t    ret = nvs_open(NVS_DV_NameSpace, NVS_READWRITE, &my_handle);
    if(ret != ESP_OK)
    {
        ret = ESP_FAIL;
    }
    else
    {
        if(nvs_get_i32(my_handle, key, (int32_t *)val) == ESP_OK)
        {
            ESP_LOGI(TAG, "Got \'%s\' from nvs!", key);
            ret = ESP_OK;
        }
        else
        {
            ESP_LOGW(TAG, "\'%s\' not found in nvs, try writing value first", key);
            ret = ESP_FAIL;
        }
    }
    return ret;
}

esp_err_t device_update_confiuration(device_info_t *info)
{
    wifi_reinit_sta(info->ssid, info->passwd, info->conn_retry);
    if(local_city_code_update(info->city_code) == ESP_OK)
    {
        esp_err_t err = client_get_weather(base);
        if(err == ESP_OK)
        {
            xEventGroupSetBits(s_wifi_event_group, HTTP_GET_WEATHER_INFO_BIT);
        }
    }
    device_info_update_bg_image(info->bg_image);
    return ESP_OK;
}

static esp_err_t index_html_get_handler(httpd_req_t *req)
{
    const size_t index_html_size = (index_html_end - index_html_start);
    ESP_LOGI(TAG, "Handling index HTML request");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html_start, index_html_size);
    return ESP_OK;
}

httpd_uri_t index_html_uri = {.uri = "/", .method = HTTP_GET, .handler = index_html_get_handler, .user_ctx = NULL};

static esp_err_t submit_handler(httpd_req_t *req)
{
    char content[512];
    int  ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if(ret > 0)
    {
        // JSON 字符串
        const char *json_data = content;

        // 解析 JSON 数据
        cJSON *root = cJSON_Parse(json_data);
        if(root == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            if(error_ptr != NULL)
            {
                ESP_LOGI(TAG, "Error parsing json:%s", error_ptr);
            }
            return ESP_FAIL;
        }

        // 获取各个字段的值
        char *ssid       = cJSON_GetObjectItemCaseSensitive(root, "ssid")->valuestring;
        char *password   = cJSON_GetObjectItemCaseSensitive(root, "password")->valuestring;
        char *conn_retry = cJSON_GetObjectItemCaseSensitive(root, "conn_retry")->valuestring;
        char *api_key    = cJSON_GetObjectItemCaseSensitive(root, "api_key")->valuestring;
        char *city_code  = cJSON_GetObjectItemCaseSensitive(root, "city_code")->valuestring;
        char *bg_image   = cJSON_GetObjectItemCaseSensitive(root, "bg_image")->valuestring;

        ESP_LOGI(TAG, "Parsed SSID: %s", ssid);
        ESP_LOGI(TAG, "Parsed Password: %s", password);
        ESP_LOGI(TAG, "Parsed Connection Retry: %s", conn_retry);
        ESP_LOGI(TAG, "Parsed City Code: %s", city_code);
        ESP_LOGI(TAG, "Parsed API Key: %s", api_key);
        ESP_LOGI(TAG, "Parsed Background Image: %s", bg_image);

        if (ssid && strlen(ssid) > 0) {
            snprintf(device_info.ssid, MAX_SSID_STR_LEN, "%s", ssid);
        }
        if (password && strlen(password) > 0) {
            snprintf(device_info.passwd, MAX_PASSWD_STR_LEN, "%s", password);
        }
        if (api_key && strlen(api_key) > 0) {
            snprintf(device_info.api_key, MAX_API_KEY_STR_LEN, "%s", api_key);
        }
        if (city_code && strlen(city_code) > 0) {
            snprintf(device_info.city_code, MAX_CITY_CODE_STR_LEN, "%s", city_code);
        }
        if (conn_retry && strlen(conn_retry) > 0) {
            device_info.conn_retry = atoi(conn_retry);
        }
        if (bg_image && strlen(bg_image) > 0) {
            device_info.bg_image = atoi(bg_image);
        }
        device_update_nvs_info(&device_info);
        device_update_confiuration(&device_info);

        // 释放 cJSON 结构体使用的内存
        cJSON_Delete(root);

        content[ret] = '\0';  // 确保字符串以null结尾
        ESP_LOGI(TAG, "Return content: %s", content);
    }
    else
    {
        ESP_LOGI(TAG, "Error reading POST data.");
        return ESP_FAIL;
    }
    const char resp[] = "<html><body><h2>设置已保存</h2></body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t submit_uri = {.uri = "/submit", .method = HTTP_POST, .handler = submit_handler, .user_ctx = NULL};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    // config.server_port = 5000;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if(httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers...");
        // Register URI handlers
        httpd_register_uri_handler(server, &index_html_uri);
        httpd_register_uri_handler(server, &submit_uri);
        // httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, custom_404_handler);
        xEventGroupSetBits(s_wifi_event_group, ESP_WEB_SERVER_RUNNING_BIT);
    }
    return server;
}
