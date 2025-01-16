#ifndef LCD_LV_INIT_H
#define LCD_LV_INIT_H

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "lv_init.h"
#include "lvgl.h"
#include "hal/spi_types.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_types.h"
#include "esp_log.h"

// LCD configuration
#define LCD_HOST SPI2_HOST

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5

#define LCD_BK_LIGHT_ON_LEVEL 1

#define PARALLEL_LINES 16
#define DISP_WIDTH     240  // 240
#define DISP_HEIGHT    280  // 300
#define X_OFFSET       0
#define Y_OFFSET       20

void st7789_spi_init(void);
lv_disp_t *lvgl_config_init(void);


#endif