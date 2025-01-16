#ifndef LV_CUSTOM_UI_H
#define LV_CUSTOM_UI_H


#include "lvgl.h"
#include "esp_log.h"
#include "wifi_app.h"
#include "WeatherIcon.h"
#include "parse_json.h"

void display_test_image(lv_disp_t *disp);

void create_main_display(lv_disp_t *disp);
void update_time_display();
void update_weather_info_display();

#endif
