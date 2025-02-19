#ifndef HMI_APP_H
#define HMI_APP_H

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdint.h>
#include "wifi_app.h"
#include "lv_custom_ui.h"


#define MAX_SSID_STR_LEN      32 + 1
#define MAX_PASSWD_STR_LEN    64 + 1
#define MAX_CITY_CODE_STR_LEN 6 + 1
#define MAX_API_KEY_STR_LEN   32 + 1

// Always treat as string, except in use
typedef struct
{
    char ssid[MAX_SSID_STR_LEN];
    char passwd[MAX_PASSWD_STR_LEN];
    int  conn_retry;  // 0 or negative: always try conntection
    char city_code[MAX_CITY_CODE_STR_LEN];
    char api_key[MAX_API_KEY_STR_LEN];
    int  bg_image;
    bool wifi_reinit_flag;// true:need reinit ,false:no need reinit
} device_info_t;

extern device_info_t device_info;

void device_info_init(
    device_info_t *info,        //
    char          *ssid,        //
    char          *passwd,      //
    char          *api_key,     //
    char          *city_code,   //
    int            conn_retry,  //
    int            bg_image     //
);
esp_err_t device_update_nvs_info(device_info_t *info);
esp_err_t device_update_confiuration(device_info_t *info);


httpd_handle_t start_webserver();

// esp_err_t init_server(httpd_handle_t *server);
#endif