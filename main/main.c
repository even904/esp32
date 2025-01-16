/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "WeatherIcon.h"
#include "core/lv_obj.h"
#include "core/lv_obj_style_gen.h"
#include "display/lv_display.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcd_lv_init.h"
#include "esp_log.h"

#include "esp_http_client.h"
#include "widgets/image/lv_image.h"
#include "widgets/label/lv_label.h"
#include "wifi_app.h"
#include "lv_custom_ui.h"
#include "parse_json.h"

// WIFI configuration
#define AP_TO_CONN_SSID "Even"
#define AP_TO_CONN_PASS "12345687"
#define ESP_AS_AP_SSID  "ESP32"
#define ESP_AS_AP_PASS  "12345687"

// debug
#define ENABLE_MEMORY_CHECK 1

static const char *TAG = "LCD";

void check_heap_memory(const char *TAG)
{
    // 打印总的可用堆内存
    ESP_LOGI(TAG, "Free heap size: %d bytes", (int)esp_get_free_heap_size());

    // 打印自启动以来最小的空闲堆内存
    ESP_LOGI(TAG, "Minimum free heap size since boot: %d bytes", (int)esp_get_minimum_free_heap_size());

    // 打印 DRAM 区域的详细信息
    ESP_LOGI(TAG, "DRAM heap info:");
    heap_caps_print_heap_info(MALLOC_CAP_8BIT);

    // // 打印 IRAM 区域的详细信息
    // ESP_LOGI(TAG, "IRAM heap info:");
    // heap_caps_print_heap_info(MALLOC_CAP_IRAM_8BIT);

    // // 打印所有内存区域的详细分配情况
    // ESP_LOGI(TAG, "Heap details:");
    // heap_caps_dump_all();
}


static lv_timer_t *lv_timer_handle = NULL;

static void lv_timer_callback(lv_timer_t *timer)
{
    // static uint64_t cnt = 0;
    update_time_display();
    // cnt++;
}

void start_time_update_lv_timer()
{
    // 创建一个每秒触发一次的 LVGL 定时器
    lv_timer_handle = lv_timer_create(lv_timer_callback, 1000, NULL);
}

void event_update_lv_task(void *pvParameters)
{

    for(;;)
    {
        update_weather_info_display();
        // #if ENABLE_MEMORY_CHECK
        //         check_heap_memory("Update weather info task");
        // #endif
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void start_event_update_lv_task()
{
    // No need to handle(delete), no handle left.
    xTaskCreate(
        event_update_lv_task,
        "event_update_lv_task",
        4096,  // words
        NULL,
        5,  // lower than lvgl handle task
        NULL
    );
}

void app_main(void)
{
#if ENABLE_MEMORY_CHECK
    check_heap_memory("Before Initialized");
#endif
    // 1. SPI Init
    st7789_spi_init();
    // 2~4. Install LCD panel driver & Add Display
    lv_disp_t *disp = lvgl_config_init();
    // Check if display is initialized, if so, disp pointer is not NULL
    ESP_LOGI(TAG, "Is num of disp_handle: %x", (uintptr_t)(disp));
    // lv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_270);
    ESP_LOGI(TAG, "Display LVGL Test Image");
    // check_heap_memory();
    if(lvgl_port_lock(0))
    {
        // display_test_image(disp);
        // create_loading_animation(disp);
        create_main_display(disp);
        lvgl_port_unlock();
    }
#if ENABLE_MEMORY_CHECK
    check_heap_memory("LVGL Initialized");
#endif
    // wifi configuration
    wifi_init_sta_ap(AP_TO_CONN_SSID, AP_TO_CONN_PASS, ESP_AS_AP_SSID, ESP_AS_AP_PASS);
#if ENABLE_MEMORY_CHECK
    check_heap_memory("WiFi Initialized");
#endif
    // timer update time configuration
    start_time_update_lv_timer();
    start_event_update_lv_task();
#if ENABLE_MEMORY_CHECK
    check_heap_memory("Tasks Initialized");
#endif
}
