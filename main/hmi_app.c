#include "hmi_app.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdio.h>

const char *TAG = "HMI";

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

static esp_err_t index_html_get_handler(httpd_req_t *req)
{
    const size_t index_html_size = (index_html_end - index_html_start);
    ESP_LOGI(TAG, "Handling index HTML request");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html_start, index_html_size);
    return ESP_OK;
}

httpd_uri_t index_html_uri = {.uri = "/", .method = HTTP_GET, .handler = index_html_get_handler, .user_ctx = NULL};

esp_err_t submit_handler(httpd_req_t *req)
{
    char content[512];
    // 假设POST请求体的最大长度为512字节
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if(ret > 0)
    {
        content[ret] = '\0';  // 确保字符串以null结尾
        printf("%s\n", content);
        // parse content
        // 查找ssid和password的位置
        char *ssid_start   = strstr(content, "ssid=");
        char *passwd_start = strstr(content, "&password=");
        if(ssid_start && passwd_start)
        {
            // 解析SSID
            ssid_start += strlen("ssid=");
            char local_ssid[33] = {0};  // SSID最大长度是32加上一个终止符
            strncpy(local_ssid, ssid_start, passwd_start - ssid_start);

            // 解析Password
            passwd_start += strlen("&password=");
            char local_passwd[65] = {0};         // 密码最大长度是64加上一个终止符
            strcpy(local_passwd, passwd_start);  // 直接复制直到末尾

            // 打印输出结果
            ESP_LOGI(TAG, "Parsed SSID: %s\n", local_ssid);
            ESP_LOGI(TAG, "Parsed Password: %s\n", local_passwd);

            // 在这里你可以添加更改Wi-Fi配置等逻辑...
        }
        else
        {
            ESP_LOGI(TAG, "Failed to parse ssid or password from request.\n");
        }
    }
    else
    {
        ESP_LOGI(TAG, "Error reading POST data.\n");
    }
    // 根据解析结果更新Wi-Fi设置等操作
    // 返回响应给客户端
    const char resp[] = "<html><body><h2>设置已保存</h2></body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t submit_uri = {.uri = "/submit", .method = HTTP_POST, .handler = submit_handler, .user_ctx = NULL};

// httpd_err_handler_func_t custom_404_handler(httpd_req_t *req) {
//     const char resp[] = "<html><body><h1>Error 404: Not Found</h1> <p>The requested URL was not found on this
//     server.</p></body></html>"; httpd_resp_set_status(req, "404"); httpd_resp_set_type(req, "text/html");
//     httpd_resp_send(req, resp, strlen(resp));
//     return ESP_OK;
// }

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
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
    }
    return server;
}
