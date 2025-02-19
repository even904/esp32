#include "lv_custom_ui.h"
#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "custom_font.h"
#include "custom_image.h"
#include "draw/lv_image_dsc.h"
#include "font/lv_font.h"
#include "lv_api_map_v8.h"
#include "misc/lv_anim.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_style_gen.h"
#include "misc/lv_types.h"
#include "parse_json.h"
#include "widgets/bar/lv_bar.h"
#include "widgets/label/lv_label.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *lv_custom_ui_TAG = "UI";
void               update_background_image(lv_img_dsc_t *new_bg_img);

void display_test_image(lv_disp_t *disp)
{
    const char *lv_custom_ui_TAG = "Task";
    ESP_LOGI(lv_custom_ui_TAG, "LVGL task running");
    // 创建一个全屏的颜色填充 (例如蓝色背景)
    lv_obj_t *background = lv_obj_create(lv_disp_get_scr_act(disp));
    lv_obj_set_size(background, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_style_bg_color(background, lv_color_make(0, 0, 255), LV_PART_MAIN);
    lv_obj_align(background, LV_ALIGN_CENTER, 0, 0);

    // 绘制一个渐变矩形
    lv_obj_t *gradient_box = lv_obj_create(background);
    lv_obj_set_size(gradient_box, 200, 100);
    lv_obj_align(gradient_box, LV_ALIGN_CENTER, 0, 0);
    static lv_style_t style_grad;
    lv_style_init(&style_grad);
    lv_style_set_bg_opa(&style_grad, LV_OPA_COVER);
    lv_style_set_bg_grad_color(&style_grad, lv_color_make(255, 0, 0));  // 红色
    lv_style_set_bg_color(&style_grad, lv_color_make(0, 255, 0));       // 绿色
    lv_style_set_bg_grad_dir(&style_grad, LV_GRAD_DIR_VER);             // 垂直渐变
    lv_obj_add_style(gradient_box, &style_grad, LV_STATE_DEFAULT);

    // 添加一些文本
    lv_obj_t *label = lv_label_create(lv_disp_get_scr_act(disp));
    lv_label_set_text(label, "Test Image");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void create_loading_animation(lv_obj_t *obj)
{
    // 创建一个 spinner 控件作为加载指示器
    lv_obj_t *spinner = lv_spinner_create(obj);
    lv_obj_set_size(spinner, 50, 50);              // 设置 spinner 的大小
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);  // 将 spinner 居中对齐
}

static struct
{
    lv_obj_t *h_m_label;
    lv_obj_t *m_d_wd_label;
    lv_obj_t *year_label;
    lv_obj_t *sec_arc;
    lv_obj_t *sec_label;
} tdc;  // tdc

void create_time_display(lv_obj_t *left_container, lv_obj_t *right_container)
{
    time_t    now;
    char      s[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(s, sizeof(s), "%c", &timeinfo);
    ESP_LOGI(lv_custom_ui_TAG, "The current date/time in Shanghai is: %s", s);

    char year[] = {s[20], s[21], s[22], s[23], '\0'};
    char m_d_wd[]
        = {s[4], s[5], s[6], ' ', s[8], s[9], ' ', s[0], s[1], s[2], '\0'};  // month, day, weekday: Jan 01 Wed
    char h_m[10];                                                            // hour, min: 10:00
    char sec[] = {s[17], s[18], '\0'};
    snprintf(h_m, sizeof(h_m), "%02d:%02d", (uint8_t)timeinfo.tm_hour, (uint8_t)timeinfo.tm_min);

    // Left
    tdc.h_m_label = lv_label_create(left_container);
    if(tdc.h_m_label != NULL)
    {
        lv_label_set_text(tdc.h_m_label, h_m);
        lv_obj_set_style_text_font(tdc.h_m_label, &WenQuanWeiMiHei_48, LV_PART_MAIN);
        lv_obj_align(tdc.h_m_label, LV_ALIGN_CENTER, 0, 0);
    }
    // Right
    tdc.sec_arc = lv_arc_create(right_container);
    static lv_style_t bg_style;
    static lv_style_t indicator_style;
    if(tdc.sec_arc != NULL)
    {
        lv_style_init(&bg_style);
        lv_style_set_arc_width(&bg_style, 4);
        // lv_style_set_arc_opa(&bg_style, LV_OPA_TRANSP);
        lv_style_init(&indicator_style);
        lv_style_set_arc_width(&indicator_style, 4);
        lv_obj_add_style(tdc.sec_arc, &bg_style, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(tdc.sec_arc, &indicator_style, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_size(tdc.sec_arc, 50, 50);
        lv_obj_align(tdc.sec_arc, LV_ALIGN_CENTER, 0, 0);
        lv_arc_set_mode(tdc.sec_arc, LV_ARC_MODE_NORMAL);  // Circle
        lv_arc_set_rotation(tdc.sec_arc, 270);
        lv_arc_set_start_angle(tdc.sec_arc, 0);     // start 0
        lv_arc_set_end_angle(tdc.sec_arc, 360);     // end 360
        lv_arc_set_bg_start_angle(tdc.sec_arc, 0);  // start 0
        lv_arc_set_bg_end_angle(tdc.sec_arc, 360);  // end 360
        lv_arc_set_range(tdc.sec_arc, 0, 59);
        lv_arc_set_value(tdc.sec_arc, timeinfo.tm_sec);
    }
    tdc.sec_label = lv_label_create(right_container);
    static lv_style_t sec_label_style;
    if(tdc.sec_label != NULL)
    {
        lv_style_init(&sec_label_style);
        // lv_style_set_bg_color(&sec_label_style, lv_palette_main(11));
        lv_style_set_bg_opa(&sec_label_style, LV_OPA_TRANSP);
        // lv_style_set_text_color(&sec_label_style, lv_color_white());
        lv_obj_add_style(tdc.sec_label, &sec_label_style, 0);
        lv_label_set_text(tdc.sec_label, sec);
        lv_obj_set_style_text_font(tdc.sec_label, &WenQuanWeiMiHei_24, LV_PART_MAIN);
        lv_obj_align(tdc.sec_label, LV_ALIGN_CENTER, 0, 0);
    }
    tdc.m_d_wd_label = lv_label_create(right_container);
    if(tdc.m_d_wd_label != NULL)
    {
        lv_label_set_text(tdc.m_d_wd_label, m_d_wd);
        lv_obj_set_style_text_font(tdc.m_d_wd_label, &WenQuanWeiMiHei_18, LV_PART_MAIN);
        lv_obj_align(tdc.m_d_wd_label, LV_ALIGN_TOP_MID, 0, 5);
    }
    tdc.year_label = lv_label_create(right_container);
    if(tdc.year_label != NULL)
    {
        lv_label_set_text(tdc.year_label, year);
        lv_obj_set_style_text_font(tdc.year_label, &WenQuanWeiMiHei_24, LV_PART_MAIN);
        lv_obj_align(tdc.year_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
}

void device_info_update_bg_image(int bg_image)
{
    switch(bg_image)
    {
    case 0:
        update_background_image((lv_img_dsc_t *)&kw_bg);
        break;
    case 1:
        update_background_image((lv_img_dsc_t *)&missaka);
        break;
    case 2:
        update_background_image((lv_img_dsc_t *)&k1);
        break;
    case 3:
        update_background_image((lv_img_dsc_t *)&fuji);
        break;
    case 4:
        update_background_image((lv_img_dsc_t *)&hero);
        break;
    case 5:
        update_background_image((lv_img_dsc_t *)&roxy);
        break;
    // No enough flash memory, partition table only support 3M factory data 
    // case 6:
    //     update_background_image((lv_img_dsc_t *)&bridge);
    //     break;
    // case 7:
    //     update_background_image((lv_img_dsc_t *)&lukiya);
    //     break;
    // case 8:
    //     update_background_image((lv_img_dsc_t *)&cat);
    //     break;
    // case 9:
    //     update_background_image((lv_img_dsc_t *)&WuKong);
    //     break;
    default:
        break;
    }
}

void update_time_display()
{
    static uint8_t cnt = 0;
    cnt++;
    time_t    now;
    char      s[64];
    struct tm timeinfo;

    time(&now);

    // 设置时区为中国标准时间 (CST-8)
    setenv("TZ", "CST-8", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(s, sizeof(s), "%c", &timeinfo);
    if(cnt == 60)
    {
        ESP_LOGI(lv_custom_ui_TAG, "The current date/time in Shanghai is: %s", s);
        cnt = 0;
    }

    char year[] = {s[20], s[21], s[22], s[23], '\0'};
    char m_d_wd[]
        = {s[4], s[5], s[6], ' ', s[8], s[9], ' ', s[0], s[1], s[2], '\0'};  // month, day, weekday: Jan-01 Wed
    char h_m[10];                                                            // hour, min: 10:00
    char sec[] = {s[17], s[18], '\0'};

    snprintf(h_m, sizeof(h_m), "%02d:%02d", (uint8_t)timeinfo.tm_hour, (uint8_t)timeinfo.tm_min);

    if(tdc.h_m_label != NULL)
    {
        lv_label_set_text(tdc.h_m_label, h_m);
    }
    if(tdc.year_label != NULL)
    {
        lv_label_set_text(tdc.year_label, year);
    }
    if(tdc.m_d_wd_label != NULL)
    {
        lv_label_set_text(tdc.m_d_wd_label, m_d_wd);
    }
    if(tdc.sec_arc != NULL)
    {
        lv_arc_set_value(tdc.sec_arc, timeinfo.tm_sec);
    }
    if(tdc.sec_label != NULL)
    {
        lv_label_set_text(tdc.sec_label, sec);
    }
}

struct
{
    /* upper container */
    lv_obj_t *weather_icon_label;
    lv_obj_t *weather_label;
    lv_obj_t *temperature_label;
    //...
    /* bottom container */
    lv_obj_t *province_label;
    lv_obj_t *city_label;
    lv_obj_t *humidity_label;
    lv_obj_t *report_time_label;
    lv_obj_t *wind_direction_label;
    lv_obj_t *wind_power_label;
    /* bottom icon_label */
    lv_obj_t *humidity_icon_label;
    lv_obj_t *wind_direction_icon_label;
    lv_obj_t *wind_power_icon_label;
    lv_obj_t *report_time_icon_label;
} widc;  // weather_info_disp_content

void create_weather_info_display_upper(lv_obj_t *left_container, lv_obj_t *right_container)
{
    // lv_obj_t *img = lv_image_create(obj);
    // lv_image_set_src(img, &rain);
    // lv_obj_align(img, LV_ALIGN_RIGHT_MID, 0, 0);
    // Left
    widc.weather_icon_label = lv_label_create(left_container);
    lv_obj_set_style_text_font(widc.weather_icon_label, &WeatherIcon, LV_PART_MAIN);
    lv_label_set_text(widc.weather_icon_label, WI84_ee9bb2);
    ESP_LOGI(lv_custom_ui_TAG, "Icon");
    lv_obj_align(widc.weather_icon_label, LV_ALIGN_CENTER, 0, 0);
    // Right
    widc.weather_label = lv_label_create(right_container);
    lv_obj_set_style_text_font(widc.weather_label, &WenQuanWeiMiHei_24, LV_PART_MAIN);
    lv_obj_set_width(widc.weather_label, LV_SIZE_CONTENT);
    lv_label_set_long_mode(widc.weather_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_height(widc.weather_label, 50);
    lv_label_set_text(widc.weather_label, "未知");
    lv_obj_align(widc.weather_label, LV_ALIGN_TOP_LEFT, 0, 13);

    widc.temperature_label = lv_label_create(right_container);
    lv_obj_set_style_text_font(widc.temperature_label, &WenQuanWeiMiHei_36, LV_PART_MAIN);
    lv_label_set_text(widc.temperature_label, "??°C");
    lv_obj_set_height(widc.temperature_label, 50);
    lv_obj_align(widc.temperature_label, LV_ALIGN_BOTTOM_LEFT, 0, -7);
}

void create_weather_info_display_bottom(lv_obj_t *left_container, lv_obj_t *middle_container, lv_obj_t *right_container)
{
    // Left
    widc.province_label = lv_label_create(left_container);
    lv_obj_set_style_text_font(widc.province_label, &WenQuanWeiMiHei_24, LV_PART_MAIN);
    lv_label_set_text(widc.province_label, "测试 北京");
    lv_obj_align(widc.province_label, LV_ALIGN_TOP_MID, 0, 10);

    widc.city_label = lv_label_create(left_container);
    lv_obj_set_style_text_font(widc.city_label, &WenQuanWeiMiHei_24, LV_PART_MAIN);
    lv_label_set_text(widc.city_label, "测试 东城区");
    lv_obj_align(widc.city_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    // Middle
    widc.humidity_icon_label = lv_label_create(middle_container);
    lv_obj_set_style_text_font(widc.humidity_icon_label, &MoreIcon, LV_PART_MAIN);
    lv_label_set_text(widc.city_label, MI18_Humidity);

    widc.wind_direction_icon_label = lv_label_create(middle_container);
    lv_obj_set_style_text_font(widc.wind_direction_icon_label, &MoreIcon, LV_PART_MAIN);
    lv_label_set_text(widc.city_label, MI18_WindDirection);

    widc.wind_power_icon_label = lv_label_create(middle_container);
    lv_obj_set_style_text_font(widc.wind_power_icon_label, &MoreIcon, LV_PART_MAIN);
    lv_label_set_text(widc.city_label, MI18_WindPower);

    widc.report_time_icon_label = lv_label_create(middle_container);
    lv_obj_set_style_text_font(widc.report_time_icon_label, &MoreIcon, LV_PART_MAIN);
    lv_label_set_text(widc.city_label, MI18_UpdateTime);

    // Right
    widc.humidity_label = lv_label_create(right_container);
    lv_obj_set_style_text_font(widc.humidity_label, &WenQuanWeiMiHei_18, LV_PART_MAIN);
    lv_label_set_text(widc.humidity_label, "湿度 13%");
    // lv_obj_align(widc.humidity_label, LV_ALIGN_TOP_MID, 0, 0);

    widc.wind_direction_label = lv_label_create(right_container);
    lv_obj_set_style_text_font(widc.wind_direction_label, &WenQuanWeiMiHei_18, LV_PART_MAIN);
    lv_label_set_text(widc.wind_direction_label, "风向 N");
    // lv_obj_align(widc.wind_direction_label, LV_ALIGN_TOP_MID, 0, 0);

    widc.wind_power_label = lv_label_create(right_container);
    lv_obj_set_style_text_font(widc.wind_power_label, &WenQuanWeiMiHei_18, LV_PART_MAIN);
    lv_label_set_text(widc.wind_power_label, "风力 <=3");
    // lv_obj_align(widc.wind_power_label, LV_ALIGN_TOP_MID, 0, 0);

    widc.report_time_label = lv_label_create(right_container);
    lv_obj_set_style_text_font(widc.report_time_label, &WenQuanWeiMiHei_18, LV_PART_MAIN);
    lv_label_set_text(widc.report_time_label, "更新 16:06:57");
    // lv_obj_align(widc.report_time_label, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// Receive weather info json and parse, store in parsed_weather_info
void get_parsed_weather_info()
{
    EventBits_t uxBits = xEventGroupWaitBits(s_wifi_event_group, HTTP_GET_WEATHER_INFO_BIT, pdTRUE, pdFALSE, 100);

    if((uxBits & HTTP_GET_WEATHER_INFO_BIT) != 0)
    {
        char *weather_info_str = malloc(raw_weather_info.raw_content_length + 1);
        memcpy(weather_info_str, raw_weather_info.raw_content, raw_weather_info.raw_content_length);
        parse_json_update_weather(weather_info_str);

        weather_info_str[raw_weather_info.raw_content_length] = '\0';
        ESP_LOGI(lv_custom_ui_TAG, "Got HTTP_GET_WEATHER_INFO_BIT");
        ESP_LOGI(
            lv_custom_ui_TAG,
            "Receive buf length = %d, Weather info buf received:\n%s",
            raw_weather_info.raw_content_length,
            weather_info_str
        );
        free(weather_info_str);
        weather_info_str = NULL;
    }
}

void update_weather_info_display()
{
    // ESP_LOGI(lv_custom_ui_TAG, "Updating weather info display...");
    if(widc.weather_icon_label != NULL)
    {
        lv_label_set_text(widc.weather_icon_label, get_weather_icon(parsed_weather_info.weather));
    }
    if(widc.weather_label != NULL)
    {
        lv_label_set_text(widc.weather_label, parsed_weather_info.weather);
    }
    if(widc.province_label != NULL && parsed_weather_info.province != NULL)  // need deeper thinking
    {
        lv_label_set_text(widc.province_label, parsed_weather_info.province);
    }
    if(widc.city_label != NULL && parsed_weather_info.city != NULL)
    {
        lv_label_set_text(widc.city_label, parsed_weather_info.city);
    }
    if(widc.humidity_label != NULL && parsed_weather_info.humidity != NULL)
    {
        lv_label_set_text(widc.humidity_label, parsed_weather_info.humidity);
    }
    if(widc.report_time_label != NULL && parsed_weather_info.reporttime != NULL)
    {
        lv_label_set_text(widc.report_time_label, &parsed_weather_info.reporttime[11]);
    }
    if(widc.temperature_label != NULL && parsed_weather_info.temperature != NULL)
    {
        lv_label_set_text(widc.temperature_label, parsed_weather_info.temperature);
    }
    if(widc.wind_direction_label != NULL && parsed_weather_info.winddirection != NULL)
    {
        lv_label_set_text(widc.wind_direction_label, parsed_weather_info.winddirection);
    }
    if(widc.wind_power_label != NULL && parsed_weather_info.windpower != NULL)
    {
        lv_label_set_text(widc.wind_power_label, parsed_weather_info.windpower);
    }
    if(widc.humidity_icon_label != NULL)
    {
        lv_label_set_text(widc.humidity_icon_label, MI18_Humidity);
    }
    if(widc.wind_direction_icon_label != NULL)
    {
        lv_label_set_text(widc.wind_direction_icon_label, MI18_WindDirection);
    }
    if(widc.wind_power_icon_label != NULL)
    {
        lv_label_set_text(widc.wind_power_icon_label, MI18_WindPower);
    }
    if(widc.report_time_icon_label != NULL)
    {
        lv_label_set_text(widc.report_time_icon_label, MI18_UpdateTime);
    }
}

/* -----------------------*/
/*|                      |*/
/*|                      |*/
/*|    Upper Container   |*/
/*|                      |*/
/*|                      |*/
/* -----------------------*/
/*|                      |*/
/*|                      |*/
/*|    Middle Container  |*/
/*|                      |*/
/*|                      |*/
/* -----------------------*/
/*|                      |*/
/*|                      |*/
/*|    Bottom Container  |*/
/*|                      |*/
/* -----------------------*/
static lv_style_t main_container_style;
lv_obj_t         *main_container;

void create_main_display(lv_disp_t *disp)
{
    lv_style_init(&main_container_style);
    lv_style_set_layout(&main_container_style, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&main_container_style, LV_FLEX_FLOW_COLUMN);
    lv_style_set_pad_all(&main_container_style, 0);  // 设置所有方向的内边距为0
    lv_style_set_pad_row(&main_container_style, 0);  // 确保行之间的间距也为0
    lv_style_set_pad_column(&main_container_style, 0);
    lv_style_set_border_width(&main_container_style, 0);
    lv_style_set_bg_img_src(&main_container_style, &kw_bg);
    //
    // 创建一个容器作为主界面
    main_container = lv_obj_create(lv_disp_get_scr_act(disp));
    lv_obj_add_style(main_container, &main_container_style, 0);
    lv_obj_set_size(main_container, lv_disp_get_hor_res(NULL),
                    lv_disp_get_ver_res(NULL));  // 设置容器大小为全屏
    lv_obj_center(main_container);               // 将容器居中对齐

    /* 创建并初始化样式 */
    static lv_style_t child_container_style;
    lv_style_init(&child_container_style);
    lv_style_set_border_width(&child_container_style, 0);    // Set broder width = 0
    lv_style_set_pad_all(&child_container_style, 0);         // Set pad = 0 in all directions
    lv_style_set_radius(&child_container_style, 0);          // No radius
    lv_style_set_bg_opa(&child_container_style, LV_OPA_50);  // Transparent

    create_upper_container(main_container, &child_container_style);
    create_middle_container(main_container, &child_container_style);
    create_bottom_container(main_container, &child_container_style);
}

void update_background_image(lv_img_dsc_t *new_bg_img)
{
    lv_style_set_bg_img_src(&main_container_style, new_bg_img);  // 更新背景图片
    lv_obj_refresh_style(main_container, LV_PART_MAIN, LV_STYLE_BG_IMAGE_SRC);
}

// 在需要的地方调用此函数更新背景图片
// example:
// lv_img_dsc_t new_bg; // 新背景图片的描述符
// ...（加载或初始化new_bg）
// update_background_image(main_container, &new_bg);

/* -----------------------*/
/*|        |      |      |*/
/*|        |      |      |*/
/*|        |-------------|*/
/*|   Upper|Container    |*/
/*|        |             |*/
/* -----------------------*/
void create_upper_container(lv_obj_t *obj, lv_style_t *style)
{
    lv_obj_t *container = lv_obj_create(obj);
    lv_obj_set_size(container, LV_PCT(100), 100);
    lv_obj_add_style(container, style, 0);

    lv_obj_t *left_container = lv_obj_create(container);
    lv_obj_set_size(left_container, 100, LV_PCT(100));
    lv_obj_add_style(left_container, style, 0);
    // lv_obj_set_style_bg_color(left_container, lv_palette_main(10), 0);

    lv_obj_t *right_container = lv_obj_create(container);
    lv_obj_set_size(right_container, 140, LV_PCT(100));
    lv_obj_add_style(right_container, style, 0);
    // lv_obj_set_style_bg_color(right_container, lv_palette_main(0), 0);
    lv_obj_set_align(right_container, LV_ALIGN_RIGHT_MID);

    create_weather_info_display_upper(left_container, right_container);
}

/* -----------------------*/
/*|               |      |*/
/*|               |      |*/
/*|    Middle Container  |*/
/*|               |      |*/
/*|               |      |*/
/* -----------------------*/
void create_middle_container(lv_obj_t *obj, lv_style_t *style)
{
    lv_obj_t *container = lv_obj_create(obj);
    lv_obj_set_size(container, LV_PCT(100), 100);
    lv_obj_add_style(container, style, 0);

    lv_obj_t *left_container = lv_obj_create(container);
    lv_obj_set_size(left_container, 140, LV_PCT(100));
    lv_obj_add_style(left_container, style, 0);
    // lv_obj_set_style_bg_color(left_container, lv_palette_main(1), 0);

    lv_obj_t *right_container = lv_obj_create(container);
    lv_obj_set_size(right_container, 100, LV_PCT(100));
    lv_obj_add_style(right_container, style, 0);
    // lv_obj_set_style_bg_color(right_container, lv_palette_main(2), 0);
    lv_obj_set_align(right_container, LV_ALIGN_RIGHT_MID);

    create_time_display(left_container, right_container);
}

/* -----------------------*/
/*|        |    |        |*/
/*|        |    |        |*/
/*|    Bottom Container  |*/
/*|        |    |        |*/
/* -----------------------*/
void create_bottom_container(lv_obj_t *obj, lv_style_t *style)
{
    lv_obj_t *container = lv_obj_create(obj);
    lv_obj_set_size(container, LV_PCT(100), 80);
    lv_obj_add_style(container, style, 0);
    // create_loading_animation(container);

    lv_obj_t *left_container = lv_obj_create(container);
    lv_obj_set_size(left_container, 120, LV_PCT(100));
    lv_obj_add_style(left_container, style, 0);
    // lv_obj_set_style_bg_color(left_container, lv_palette_main(3), 0);

    static lv_style_t right_container_style;
    lv_style_init(&right_container_style);
    lv_style_set_layout(&right_container_style, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&right_container_style, LV_FLEX_FLOW_COLUMN);
    lv_style_set_pad_all(&right_container_style, 0);  // 设置所有方向的内边距为0
    lv_style_set_pad_row(&right_container_style, 0);  // 确保行之间的间距也为0
    lv_style_set_pad_column(&right_container_style, 0);
    lv_style_set_border_width(&right_container_style, 0);
    lv_style_set_bg_opa(&right_container_style, LV_OPA_50);  // Transparent

    lv_obj_t *middle_container = lv_obj_create(container);
    lv_obj_set_size(middle_container, 20, LV_PCT(100));
    lv_obj_add_style(middle_container, &right_container_style, 0);
    // lv_obj_set_style_bg_color(middle_container, lv_palette_main(12), 0);
    lv_obj_align(middle_container, LV_ALIGN_RIGHT_MID, -100, 0);

    lv_obj_t *right_container = lv_obj_create(container);
    lv_obj_set_size(right_container, 100, LV_PCT(100));
    lv_obj_add_style(right_container, &right_container_style, 0);
    // lv_obj_set_style_bg_color(right_container, lv_palette_main(4), 0);
    lv_obj_set_align(right_container, LV_ALIGN_RIGHT_MID);

    create_weather_info_display_bottom(left_container, middle_container, right_container);
}

const char *weatherPhenomena[] = {
    "晴",                // [0]Sunny
    "少云",              // [1]Cloudy
    "晴间多云",          // [2]
    "多云",              // [3]
    "阴",                // [4]Overcast
    "有风",              // [5]Breezee
    "平静",              // [6]
    "微风",              // [7]
    "和风",              // [8]
    "清风",              // [9]
    "强风/劲风",         // [10]StrongWind
    "疾风",              // [11]
    "大风",              // [12]
    "烈风",              // [13]
    "风暴",              // [14]Storm
    "狂爆风",            // [15]
    "飓风",              // [16]
    "热带风暴",          // [17]Haze
    "霾",                // [18]
    "中度霾",            // [19]
    "重度霾",            // [20]
    "严重霾",            // [21]
    "阵雨",              // [22]Shower
    "雷阵雨",            // [23]Thundershower
    "雷阵雨并伴有冰雹",  // [24]
    "小雨",              // [25]LightRain
    "中雨",              // [26]MiddleRain
    "大雨",              // [27]HeavyRain
    "暴雨",              // [28]HardRain
    "大暴雨",            // [29]
    "特大暴雨",          // [30]
    "强阵雨",            // [31]StrongThunderShower
    "强雷阵雨",          // [32]
    "极端降雨",          // [33]
    "毛毛雨/细雨",       // [34]=LightRain
    "雨",                // [35]=LightRain
    "小雨-中雨",         // [36]=MiddleRain
    "中雨-大雨",         // [37]=HeavyRain
    "大雨-暴雨",         // [38]=HardRain
    "暴雨-大暴雨",       // [39]=
    "大暴雨-特大暴雨",   // [40]=
    "雨雪天气",          // [41]Snowy
    "雨夹雪",            // [42]RainAndSnow
    "阵雨夹雪",          // [43]
    "冻雨",              // [44]
    "雪",                // [45]Snow
    "阵雪",              // [46]DaySnow/NightSnow
    "小雪",              // [47]LightSnow
    "中雪",              // [48]MiddleSnow
    "大雪",              // [49]HeavySnow
    "暴雪",              // [50]HardSnow
    "小雪-中雪",         // [51]=MiddleSnow
    "中雪-大雪",         // [52]=HeavySnow
    "大雪-暴雪",         // [53]=HardSnow
    "浮尘",              // [54]Dust
    "扬沙",              // [55]
    "沙尘暴",            // [56]DustStorm
    "强沙尘暴",          // [57]
    "龙卷风",            // [58]Tornado
    "雾",                // [59]Fog
    "浓雾",              // [60]HeavyFog
    "强浓雾",            // [61]
    "轻雾",              // [62]
    "大雾",              // [63]
    "特强浓雾",          // [64]
    "热",                // [65]Hot
    "冷",                // [66]Cold
    "未知"               // [67]UnknownWeather
};

char *get_weather_icon(char *weather)
{
    // ❗Day and Night need division
    int  find_weather = -1;
    bool is_day       = false;  // 1: day_time 0:night_time

    time_t    now;
    struct tm timeinfo;
    time(&now);
    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    if(timeinfo.tm_hour >= 5 && timeinfo.tm_hour <= 18)
    {
        is_day = true;
    }
    if(weather == NULL)
    {
        return WI84_ee9bb2;  // not any
    }
    for(int i = 0; i < 68; i++)
    {
        if(strcmp(weather, weatherPhenomena[i]) == 0)
        {
            find_weather = i;
        }
    }

    switch(find_weather)
    {
    case 0:
        return (is_day) ? WI84_ee99a2 : WI84_ee9e86;
        break;  // Sunny

    case 1:
    case 2:
    case 3:
        return (is_day) ? WI84_ee9d84 : WI84_ee9b93;
        break;  // DayCloudy

    case 4:
        return WI84_ee9d99;
        break;  // Overcast
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        return WI84_ee9882;
        break;  // Breezee
    case 10:
    case 11:
    case 12:
    case 13:
        return WI84_ee9887;
        break;  // StrongWind
    case 14:
    case 15:
    case 16:
        return WI84_ee9b91;
        break;  // Storm
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
        return WI84_ee98aa;
        break;  // Haze
    case 22:
        return WI84_ee9bb7;
        break;  // Shower
    case 23:
    case 24:
        return WI84_ee989d;
        break;  // ThunderShower
    case 25:
        return WI84_ee9898;
        break;  // LightRain
    case 26:
        return WI84_ee989a;
        break;  // MiddleRain
    case 27:
        return WI84_ee9899;
        break;  // HeavyRain
    case 28:
    case 29:
    case 30:
        return WI84_ee9892;
        break;  // HardRain
    case 31:
    case 32:
    case 33:
        return WI84_ee9ca2;
        break;  // StrongThunderShower
    case 34:
    case 35:
        return WI84_ee9898;
        break;  // LightRain
    case 36:
        return WI84_ee989a;
        break;  // MiddleRain
    case 37:
        return WI84_ee9899;
        break;  // HeavyRain
    case 38:
    case 39:
    case 40:
        return WI84_ee9892;
        break;  // HardRain
    case 41:
        return WI84_ee9990;
        break;  // Snowy
    case 42:
    case 43:
    case 44:
        return WI84_ee9893;
        break;  // RainAndSnow
    case 45:
        return WI84_ee9893;
        break;  // Snow
    case 46:
        return (is_day) ? WI84_ee9894 : WI84_ee9895;
        break;
    case 47:
        return WI84_ee98b6;
        break;  // LightSnow
    case 48:
        return WI84_ee98b8;
        break;  // MiddleSnow
    case 49:
        return WI84_ee98bc;
        break;  // HeavySnow
    case 50:
        return WI84_ee98b9;
        break;  // HardSnow
    case 51:
        return WI84_ee98b8;
        break;  // MiddleSnow
    case 52:
        return WI84_ee98bc;
        break;  // HeavySnow
    case 53:
        return WI84_ee98b9;
        break;  // HardSnow
    case 54:
    case 55:
        return WI84_ee98a8;
        break;  // Dust
    case 56:
    case 57:
        return WI84_ee98b4;
        break;  // DustStorm
    case 58:
        return WI84_ee9f96;
        break;  // Tornado
    case 59:
        return WI84_ee9880;
        break;  // Fog
    case 60:
    case 61:
    case 62:
    case 63:
    case 64:
        return WI84_ee9891;
        break;  // HeavyFog
    case 65:
        return WI84_ee9881;
        break;  // Hot
    case 66:
        return WI84_eea38e;
        break;  // Cold
    default:
        return WI84_ee9bb2;  // 67 is UnknownWeather
    }
}