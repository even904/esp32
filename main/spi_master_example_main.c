/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "core/lv_obj.h"
#include "display/lv_display.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "hal/spi_types.h"
#include "lvgl.h"
#include "misc/lv_color.h"
#include "pretty_effect.h"

#include "esp_http_client.h"
#include "wifi_init.h"

// LCD configuration
#define LCD_HOST SPI2_HOST

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 19
#define PIN_NUM_CS 22

#define PIN_NUM_DC 21
#define PIN_NUM_RST 18
#define PIN_NUM_BCKL 5

#define LCD_BK_LIGHT_ON_LEVEL 1

#define PARALLEL_LINES 16
#define DISP_WIDTH 240  // 240
#define DISP_HEIGHT 280 // 300
#define X_OFFSET 0
#define Y_OFFSET 20
// WIFI configuration
#define AP_TO_CONN_SSID "Even"
#define AP_TO_CONN_PASS "12345687"
#define ESP_AS_AP_SSID "ESP32"
#define ESP_AS_AP_PASS "12345687"

// debug
#define ENABLE_MEMORY_CHECK 1

static const char *TAG = "LCD";
extern void example_lvgl_demo_ui(lv_disp_t *disp);

void display_test_image(lv_disp_t *disp) {
  const char *TAG = "Task";
  ESP_LOGI(TAG, "LVGL task running");
  // 创建一个全屏的颜色填充 (例如蓝色背景)
  lv_obj_t *background = lv_obj_create(lv_disp_get_scr_act(disp));
  lv_obj_set_size(background, lv_disp_get_hor_res(NULL),
                  lv_disp_get_ver_res(NULL));
  lv_obj_set_style_bg_color(background, lv_color_make(0, 0, 255), LV_PART_MAIN);
  lv_obj_align(background, LV_ALIGN_CENTER, 0, 0);

  // 绘制一个渐变矩形
  lv_obj_t *gradient_box = lv_obj_create(background);
  lv_obj_set_size(gradient_box, 200, 100);
  lv_obj_align(gradient_box, LV_ALIGN_CENTER, 0, 0);
  static lv_style_t style_grad;
  lv_style_init(&style_grad);
  lv_style_set_bg_opa(&style_grad, LV_OPA_COVER);
  lv_style_set_bg_grad_color(&style_grad, lv_color_make(255, 0, 0)); // 红色
  lv_style_set_bg_color(&style_grad, lv_color_make(0, 255, 0));      // 绿色
  lv_style_set_bg_grad_dir(&style_grad, LV_GRAD_DIR_VER); // 垂直渐变
  lv_obj_add_style(gradient_box, &style_grad, LV_STATE_DEFAULT);

  // 添加一些文本
  lv_obj_t *label = lv_label_create(lv_disp_get_scr_act(disp));
  lv_label_set_text(label, "Test Image");
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void create_loading_animation(lv_disp_t *disp) {
  // 创建一个 spinner 控件作为加载指示器
  lv_obj_t *spinner = lv_spinner_create(lv_disp_get_scr_act(disp));
  lv_obj_set_size(spinner, 50, 50);             // 设置 spinner 的大小
  lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0); // 将 spinner 居中对齐
}

void check_heap_memory(const char *TAG) {
  // 打印总的可用堆内存
  ESP_LOGI(TAG, "Free heap size: %d bytes", (int)esp_get_free_heap_size());

  // 打印自启动以来最小的空闲堆内存
  ESP_LOGI(TAG, "Minimum free heap size since boot: %d bytes",
           (int)esp_get_minimum_free_heap_size());

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

// This function is called (in irq context!) just before a transmission starts.
// It will set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
  int dc = (int)t->user;
  gpio_set_level(PIN_NUM_DC, dc);
}

typedef struct {
  uint16_t width;
  uint16_t height;
} esp_lcd_panel_st7789_config_t;

static void st7789_spi_init(void) {
  esp_err_t ret;
  spi_device_handle_t spi;
  spi_bus_config_t buscfg = {.miso_io_num = PIN_NUM_MISO,
                             .mosi_io_num = PIN_NUM_MOSI,
                             .sclk_io_num = PIN_NUM_CLK,
                             .quadwp_io_num = -1,
                             .quadhd_io_num = -1,
                             .max_transfer_sz = PARALLEL_LINES * 320 * 2 + 8};
  spi_device_interface_config_t devcfg = {
#ifdef CONFIG_LCD_OVERCLOCK
      .clock_speed_hz = 26 * 1000 * 1000, // Clock out at 26 MHz
#else
      .clock_speed_hz = 10 * 1000 * 1000, // Clock out at 10 MHz
#endif
      .mode = 0,                  // SPI mode 0
      .spics_io_num = PIN_NUM_CS, // CS pin
      .queue_size = 7, // We want to be able to queue 7 transactions at a time
      .pre_cb = lcd_spi_pre_transfer_callback, // Specify pre-transfer callback
                                               // to handle D/C line
  };
  // Initialize the SPI bus
  ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(ret);
  // Attach the LCD to the SPI bus
  ret = spi_bus_add_device(LCD_HOST, &devcfg, &spi);
  ESP_ERROR_CHECK(ret);
  // Initialize the LCD
  // lcd_init(spi);
  // lcd_draw_rectangle(spi, 0, 0, DISP_WIDTH, DISP_HEIGHT, 0x0000);//Black
  // lcd_draw_rectangle(spi, 0, 0, DISP_WIDTH, DISP_HEIGHT, 0xffff);
}

static lv_disp_t *lvgl_config_init(void) {
  lv_disp_t *disp_handle = NULL;
  // 2. Install LCD panel driver
  // 2.1 Configure panel IO
  ESP_LOGI(TAG, "Install ST7789 panel driver");
  ESP_LOGI(TAG, "Configuring ST7789 Panel IO");
  esp_lcd_spi_bus_handle_t spi_bus = LCD_HOST;
  esp_lcd_panel_io_spi_config_t io_config = {.cs_gpio_num = PIN_NUM_CS,
                                             .dc_gpio_num = PIN_NUM_DC,
                                             .pclk_hz = 10 * 1000 * 1000,
                                             .spi_mode = 0,
                                             .trans_queue_depth = 10,
                                             .lcd_cmd_bits = 8,
                                             .lcd_param_bits = 8};

  esp_lcd_panel_io_handle_t io_handle = NULL;
  ESP_LOGI(TAG, "Configuring ST7789 Panel");
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(spi_bus, &io_config, &io_handle));
  // 2.2 Configure panel devive
  esp_lcd_panel_handle_t lcd_panel_handle = NULL;
  esp_lcd_panel_st7789_config_t st7789_config = {.width = DISP_WIDTH,
                                                 .height = DISP_HEIGHT};
  esp_lcd_panel_dev_config_t panel_config = {.bits_per_pixel = 16,
                                             .reset_gpio_num = PIN_NUM_RST,
                                             .vendor_config = &st7789_config};
  esp_lcd_new_panel_st7789(io_handle, &panel_config, &lcd_panel_handle);

  ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_panel_handle, X_OFFSET,
                                        Y_OFFSET)); // Set Offset
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_panel_handle, true));
  // check_heap_memory();
  // 3. Start LVGL Port Task
  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
  // 4. Add Display
  ESP_LOGI(TAG, "Initialize LVGL");
  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = lcd_panel_handle,
      .buffer_size = DISP_WIDTH * DISP_HEIGHT / 4,
      .double_buffer = false,
      .hres = DISP_WIDTH,
      .vres = DISP_HEIGHT,
      .monochrome = false,
      .color_format = LV_COLOR_FORMAT_RGB565,
      .rotation =
          {
              .swap_xy = false,
              .mirror_x = false,
              .mirror_y = false,
          },
      .flags = {
          .buff_dma = true,
          .swap_bytes = true, // swap color bytes!
      }};
  ESP_LOGI(TAG, "Deinitialized num of disp_handle: %x",
           (uintptr_t)(disp_handle));
  disp_handle = lvgl_port_add_disp(&disp_cfg);
  ESP_LOGI(TAG, "SizeOf lv_color_t: %d", (int)sizeof(lv_color_t));
  ESP_LOGI(TAG, "Initialized num of disp_handle: %x", (uintptr_t)(disp_handle));
  return disp_handle;
}

void app_main(void) {
#if ENABLE_MEMORY_CHECK
  check_heap_memory("Before Initialized");
#endif
  // 1. SPI Init
  st7789_spi_init();
  // 2~4. Install LCD panel driver & Add Display
  const lv_disp_t *disp = lvgl_config_init();
  // Check if display is initialized, if so, disp pointer is not NULL
  ESP_LOGI(TAG, "Is num of disp_handle: %x", (uintptr_t)(disp));
  // lv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_270);
  ESP_LOGI(TAG, "Display LVGL Test Image");
  // check_heap_memory();
  if (lvgl_port_lock(0)) {
    display_test_image(disp);
    create_loading_animation(disp);
    lvgl_port_unlock();
  }
#if ENABLE_MEMORY_CHECK
  check_heap_memory("LVGL Initialized");
#endif
  // wifi configuration
  wifi_init_sta_ap(AP_TO_CONN_SSID, AP_TO_CONN_PASS, ESP_AS_AP_SSID,
                   ESP_AS_AP_PASS);
#if ENABLE_MEMORY_CHECK
  check_heap_memory("WiFi Initialized");
#endif
}
