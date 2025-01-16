#include "parse_json.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

// static const char *parse_json_TAG = "PARSE_JSON";

typedef enum
{
    s,
} weather_icon_t;

typedef enum
{
    w,
} wind_direction_icon_t;

typedef struct
{
    /******* status info ******** */
    bool valid_flag;  // false: deinitialized, unkown weather or return status = 0,infocode != 10000 ;true: valid
    int  count;       // total number of  report results 0:deinitialized
    char info[10];
    char infocode[5];
    /****** weather info, one character occupies two chars ****** */
    char province[10];
    char city[10];
    char adcode[10];
    char weather[10];
    char temperature[10];
    char winddirection[10];
    char windpower[5];
    char humidity[6];
    char reporttime[20];
    char temperature_float[6];
    char humidity_float[6];
    /******** signified data ******** */
    weather_icon_t        weather_icon;
    wind_direction_icon_t wind_direction_icon;
} weather_info_t;

// const char *json_str
//     = "{\"status\":\"1\",\"count\":\"1\",\"info\":\"OK\",\"infocode\":\"10000\",\"lives\":[{\"province\":\"北京\","
//       "\"city\":\"东城区\",\"adcode\":\"110101\",\"weather\":\"晴\",\"temperature\":\"-4\",\"winddirection\":\"西\","
//       "\"windpower\":\"≤3\",\"humidity\":\"19\",\"reporttime\":\"2025-01-14 "
//       "23:32:50\",\"temperature_float\":\"-4.0\",\"humidity_float\":\"19.0\"}]}";

weather_info_t parsed_weather_info = {.valid_flag = false};

void parse_json(const char *json_data)
{
    memset(&parsed_weather_info, 0, sizeof(weather_info_t));
    // 解析 JSON 字符串
    cJSON *root = cJSON_Parse(json_data);
    if(root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if(error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return;
    }

    // 获取状态信息
    cJSON *status = cJSON_GetObjectItemCaseSensitive(root, "status");
    if(cJSON_IsString(status) && (status->valuestring != NULL))
    {
        // printf("Status: %s\n", status->valuestring);
    }

    cJSON *info = cJSON_GetObjectItemCaseSensitive(root, "info");
    if(cJSON_IsString(info) && (info->valuestring != NULL))
    {
        // printf("Info: %s\n", info->valuestring);
    }

    cJSON *infocode = cJSON_GetObjectItemCaseSensitive(root, "infocode");
    if(cJSON_IsString(infocode) && (infocode->valuestring != NULL))
    {
        // printf("Infocode: %s\n", infocode->valuestring);
    }

    if((strcmp(status->valuestring, "1") == 0) && strcmp(status->valuestring, "10000"))
    {
        // 获取 lives 数组
        cJSON *lives = cJSON_GetObjectItemCaseSensitive(root, "lives");
        if(cJSON_IsArray(lives))
        {
            int i;
            for(i = 0; i < cJSON_GetArraySize(lives); i++)
            {
                cJSON *live          = cJSON_GetArrayItem(lives, i);
                cJSON *province      = cJSON_GetObjectItemCaseSensitive(live, "province");
                cJSON *city          = cJSON_GetObjectItemCaseSensitive(live, "city");
                cJSON *adcode        = cJSON_GetObjectItemCaseSensitive(live, "adcode");
                cJSON *weather       = cJSON_GetObjectItemCaseSensitive(live, "weather");
                cJSON *temperature   = cJSON_GetObjectItemCaseSensitive(live, "temperature");
                cJSON *winddirection = cJSON_GetObjectItemCaseSensitive(live, "winddirection");
                cJSON *windpower     = cJSON_GetObjectItemCaseSensitive(live, "windpower");
                cJSON *humidity      = cJSON_GetObjectItemCaseSensitive(live, "humidity");
                cJSON *reporttime    = cJSON_GetObjectItemCaseSensitive(live, "reporttime");

                if(cJSON_IsString(province) && cJSON_IsString(city) && cJSON_IsString(adcode) && cJSON_IsString(weather)
                   && cJSON_IsString(temperature) && cJSON_IsString(winddirection) && cJSON_IsString(windpower)
                   && cJSON_IsString(humidity) && cJSON_IsString(reporttime))
                {
                    // printf("Live Weather Data:\n");
                    // printf("Province: %s\n", province->valuestring);
                    // printf("City: %s\n", city->valuestring);
                    // printf("Adcode: %s\n", adcode->valuestring);
                    // printf("Weather: %s\n", weather->valuestring);
                    // printf("Temperature: %s°C\n", temperature->valuestring);
                    // printf("Wind Direction: %s\n", winddirection->valuestring);
                    // printf("Wind Power: %s\n", windpower->valuestring);
                    // printf("Humidity: %s%%\n", humidity->valuestring);
                    // printf("Report Time: %s\n", reporttime->valuestring);
                    // printf("\n");
                    strcpy(parsed_weather_info.province, province->valuestring);
                    strcpy(parsed_weather_info.city, city->valuestring);
                    strcpy(parsed_weather_info.adcode, adcode->valuestring);
                    strcpy(parsed_weather_info.weather, weather->valuestring);
                    strcpy(parsed_weather_info.temperature, temperature->valuestring);
                    strcpy(parsed_weather_info.winddirection, winddirection->valuestring);
                    strcpy(parsed_weather_info.windpower, windpower->valuestring);
                    strcpy(parsed_weather_info.humidity, humidity->valuestring);
                    strcpy(parsed_weather_info.reporttime, reporttime->valuestring);
                    parsed_weather_info.valid_flag = true;
                    printf("Parsed_weather_info data\n");
                    // ESP_LOGI(parse_json_TAG,"Parsed_weather_info data\n");
                    printf("Province: %s\n", parsed_weather_info.province);
                    printf("City: %s\n", parsed_weather_info.city);
                    printf("Adcode: %s\n", parsed_weather_info.adcode);
                    printf("Weather: %s\n", parsed_weather_info.weather);
                    printf("Temperature: %s°C\n", parsed_weather_info.temperature);
                    printf("Wind Direction: %s\n", parsed_weather_info.winddirection);
                    printf("Wind Power: %s\n", parsed_weather_info.windpower);
                    printf("Humidity: %s%%\n", parsed_weather_info.humidity);
                    printf("Report Time: %s\n", parsed_weather_info.reporttime);
                }
            }
        }
    }
    // 清理
    cJSON_Delete(root);
}

