#ifndef LV_CUSTOM_UI_H
#define LV_CUSTOM_UI_H

#include "custom_font.h"
#include "esp_log.h"
#include "lvgl.h"
#include "parse_json.h"
#include "wifi_app.h"
#include "esp_lvgl_port.h"


void display_test_image(lv_disp_t *disp);

void create_main_display(lv_disp_t *disp);
void update_time_display();

void device_info_update_bg_image(int bg_image);
void get_parsed_weather_info();
void update_weather_info_display();


void create_upper_container(lv_obj_t *obj,lv_style_t *style);
void create_middle_container(lv_obj_t *obj,lv_style_t *style);
void create_bottom_container(lv_obj_t *obj,lv_style_t *style);
char *get_weather_icon(char *weather);

#endif
