#include "parse_json.h"
#include "esp_log.h"
#include "lwipopts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// static const char *parse_json_TAG = "PARSE_JSON";

// const char *json_str
//     = "{\"status\":\"1\",\"count\":\"1\",\"info\":\"OK\",\"infocode\":\"10000\",\"lives\":[{\"province\":\"北京\","
//       "\"city\":\"东城区\",\"adcode\":\"110101\",\"weather\":\"晴\",\"temperature\":\"-4\",\"winddirection\":\"西\","
//       "\"windpower\":\"≤3\",\"humidity\":\"19\",\"reporttime\":\"2025-01-14 "
//       "23:32:50\",\"temperature_float\":\"-4.0\",\"humidity_float\":\"19.0\"}]}";

weather_info_t parsed_weather_info = {.valid_flag = false};

void parse_json_update_weather(const char *json_data)
{
    if(parsed_weather_info.infocode != NULL)
    {
        free(parsed_weather_info.info);
        free(parsed_weather_info.infocode);
        free(parsed_weather_info.province);
        free(parsed_weather_info.city);
        free(parsed_weather_info.adcode);
        free(parsed_weather_info.weather);
        free(parsed_weather_info.temperature);
        free(parsed_weather_info.winddirection);
        free(parsed_weather_info.windpower);
        free(parsed_weather_info.humidity);
        free(parsed_weather_info.reporttime);
        free(parsed_weather_info.temperature_float);
        free(parsed_weather_info.humidity_float);
    }
    memset(&parsed_weather_info, 0, sizeof(weather_info_t));
    parsed_weather_info.valid_flag = false;
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

    cJSON *status = cJSON_GetObjectItemCaseSensitive(root, "status");
    if(cJSON_IsString(status) && (status->valuestring != NULL))
    {
    }

    cJSON *info = cJSON_GetObjectItemCaseSensitive(root, "info");
    if(cJSON_IsString(info) && (info->valuestring != NULL))
    {
        parsed_weather_info.info = malloc(strlen(info->valuestring) + 1);
        memcpy(parsed_weather_info.info, info->valuestring, strlen(info->valuestring));
        parsed_weather_info.info[strlen(info->valuestring)] = '\0';
    }

    cJSON *infocode = cJSON_GetObjectItemCaseSensitive(root, "infocode");
    if(cJSON_IsString(infocode) && (infocode->valuestring != NULL))
    {
        parsed_weather_info.infocode = malloc(strlen(infocode->valuestring) + 1);
        memcpy(parsed_weather_info.infocode, infocode->valuestring, strlen(infocode->valuestring));
        parsed_weather_info.info[strlen(infocode->valuestring)] = '\0';
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
                    // Province
                    parsed_weather_info.province = malloc(strlen(province->valuestring) + 1);
                    if(parsed_weather_info.province != NULL)
                    {
                        memcpy(parsed_weather_info.province, province->valuestring, strlen(province->valuestring));
                        parsed_weather_info.province[strlen(province->valuestring)] = '\0';
                    }

                    // City
                    parsed_weather_info.city = malloc(strlen(city->valuestring) + 1);
                    if(parsed_weather_info.city != NULL)
                    {
                        memcpy(parsed_weather_info.city, city->valuestring, strlen(city->valuestring));
                        parsed_weather_info.city[strlen(city->valuestring)] = '\0';
                    }

                    // Adcode
                    parsed_weather_info.adcode = malloc(strlen(adcode->valuestring) + 1);
                    if(parsed_weather_info.adcode != NULL)
                    {
                        memcpy(parsed_weather_info.adcode, adcode->valuestring, strlen(adcode->valuestring));
                        parsed_weather_info.adcode[strlen(adcode->valuestring)] = '\0';
                    }

                    // Weather
                    parsed_weather_info.weather = malloc(strlen(weather->valuestring) + 1);
                    if(parsed_weather_info.weather != NULL)
                    {
                        memcpy(parsed_weather_info.weather, weather->valuestring, strlen(weather->valuestring));
                        parsed_weather_info.weather[strlen(weather->valuestring)] = '\0';
                    }

                    // Temperature
                    parsed_weather_info.temperature = malloc(strlen(temperature->valuestring) + 1);
                    if(parsed_weather_info.temperature != NULL)
                    {
                        memcpy(
                            parsed_weather_info.temperature, temperature->valuestring, strlen(temperature->valuestring)
                        );
                        parsed_weather_info.temperature[strlen(temperature->valuestring)] = '\0';
                    }

                    // Wind Direction
                    parsed_weather_info.winddirection = malloc(strlen(winddirection->valuestring) + 1);
                    if(parsed_weather_info.winddirection != NULL)
                    {
                        memcpy(
                            parsed_weather_info.winddirection,
                            winddirection->valuestring,
                            strlen(winddirection->valuestring)
                        );
                        parsed_weather_info.winddirection[strlen(winddirection->valuestring)] = '\0';
                    }

                    // Wind Power
                    parsed_weather_info.windpower = malloc(strlen(windpower->valuestring) + 1);
                    if(parsed_weather_info.windpower != NULL)
                    {
                        memcpy(parsed_weather_info.windpower, windpower->valuestring, strlen(windpower->valuestring));
                        parsed_weather_info.windpower[strlen(windpower->valuestring)] = '\0';
                    }

                    // Humidity
                    parsed_weather_info.humidity = malloc(strlen(humidity->valuestring) + 1);
                    if(parsed_weather_info.humidity != NULL)
                    {
                        memcpy(parsed_weather_info.humidity, humidity->valuestring, strlen(humidity->valuestring));
                        parsed_weather_info.humidity[strlen(humidity->valuestring)] = '\0';
                    }

                    // Report Time
                    parsed_weather_info.reporttime = malloc(strlen(reporttime->valuestring) + 1);
                    if(parsed_weather_info.reporttime != NULL)
                    {
                        memcpy(
                            parsed_weather_info.reporttime, reporttime->valuestring, strlen(reporttime->valuestring)
                        );
                        parsed_weather_info.reporttime[strlen(reporttime->valuestring)] = '\0';
                    }

                    parsed_weather_info.valid_flag = true;

                    printf("Parsed_weather_info data\n");
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
