#ifndef PARSE_JSON_H
#define PARSE_JSON_H

#include "cJSON.h"
#include "esp_log.h"
#include "wifi_app.h"

void parse_json_update_weather(const char *json_data);


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
    char *info;
    char *infocode;
    /****** weather info, one character occupies two chars ****** */
    char *province;
    char *city;
    char *adcode;
    char *weather;
    char *temperature;
    char *winddirection;
    char *windpower;
    char *humidity;
    char *reporttime;
    char *temperature_float;
    char *humidity_float;
    /******** signified data ******** */
    weather_icon_t        weather_icon;
    wind_direction_icon_t wind_direction_icon;
} weather_info_t;


extern weather_info_t parsed_weather_info;
#endif