#ifndef LV_CUSTOM_UI_H
#define LV_CUSTOM_UI_H

#include "WeatherIcon.h"
#include "esp_log.h"
#include "lvgl.h"
#include "parse_json.h"
#include "wifi_app.h"


void display_test_image(lv_disp_t *disp);

void create_main_display(lv_disp_t *disp);
void update_time_display();

void update_weather_info_display();

void create_upper_container(lv_obj_t *obj,lv_style_t *style);
void create_middle_container(lv_obj_t *obj,lv_style_t *style);
void create_bottom_container(lv_obj_t *obj,lv_style_t *style);

#endif
