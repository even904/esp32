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

/*
 This code displays some fancy graphics on the 320x240 LCD on an ESP-WROVER_KIT
 board. This example demonstrates the use of both spi_device_transmit as well as
 spi_device_queue_trans/spi_device_get_trans_result and pre-transmit callbacks.

 Some info about the ILI9341/ST7789V: It has an C/D line, which is connected to
 a GPIO here. It expects this line to be low for a command and high for data. We
 use a pre-transmit callback here to control that line: every transaction has as
 the user-definable argument the needed state of the D/C line and just before
 the transaction is sent, the callback will set this line to the correct state.
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////// Please update the following configuration according to your
/// HardWare spec /////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define LCD_HOST SPI2_HOST

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 19
#define PIN_NUM_CS 22

#define PIN_NUM_DC 21
#define PIN_NUM_RST 18
#define PIN_NUM_BCKL 5

#define LCD_BK_LIGHT_ON_LEVEL 1

// To speed up transfers, every SPI transfer sends a bunch of lines. This define
// specifies how many. More means more memory use, but less overhead for setting
// up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16
#define DISP_WIDTH 240  // 240
#define DISP_HEIGHT 280 // 300
#define X_OFFSET 0
#define Y_OFFSET 20

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

void check_heap_memory(void) {
  const char *TAG = "MEMORY";
  // 打印总的可用堆内存
  ESP_LOGI(TAG, "Free heap size: %d bytes", (int)esp_get_free_heap_size());

  // 打印自启动以来最小的空闲堆内存
  ESP_LOGI(TAG, "Minimum free heap size since boot: %d bytes",
           (int)esp_get_minimum_free_heap_size());

  // 打印 DRAM 区域的详细信息
  ESP_LOGI(TAG, "DRAM heap info:");
  heap_caps_print_heap_info(MALLOC_CAP_8BIT);

  // 打印 IRAM 区域的详细信息
  ESP_LOGI(TAG, "IRAM heap info:");
  heap_caps_print_heap_info(MALLOC_CAP_IRAM_8BIT);

  // // 打印所有内存区域的详细分配情况
  // ESP_LOGI(TAG, "Heap details:");
  // heap_caps_dump_all();
}

/*
 The LCD needs a bunch of command/argument values to be initialized. They are
 stored in this struct.
*/
typedef struct {
  uint8_t cmd;
  uint8_t data[16];
  uint8_t databytes; // No of data in data; bit 7 = delay after set; 0xFF = end
                     // of cmds.
} lcd_init_cmd_t;

typedef enum {
  LCD_TYPE_ILI = 1,
  LCD_TYPE_ST,
  LCD_TYPE_MAX,
} type_lcd_t;

// Place data into DRAM. Constant data gets placed into DROM by default, which
// is not accessible by DMA.
DRAM_ATTR static const lcd_init_cmd_t st_init_cmds[] = {
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    {0x36, {(1 << 5) | (1 << 6)}, 1},
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Porch Setting */
    {0xB2, {0x0c, 0x0c, 0x00, 0x33, 0x33}, 5},
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    {0xB7, {0x45}, 1},
    /* VCOM Setting, VCOM=1.175V */
    {0xBB, {0x2B}, 1},
    /* LCM Control, XOR: BGR, MX, MH */
    {0xC0, {0x2C}, 1},
    /* VDV and VRH Command Enable, enable=1 */
    {0xC2, {0x01, 0xff}, 2},
    /* VRH Set, Vap=4.4+... */
    {0xC3, {0x11}, 1},
    /* VDV Set, VDV=0 */
    {0xC4, {0x20}, 1},
    /* Frame Rate Control, 60Hz, inversion=0 */
    {0xC6, {0x0f}, 1},
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    {0xD0, {0xA4, 0xA1}, 2},
    /* Positive Voltage Gamma Control */
    {0xE0,
     {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12,
      0x16, 0x19},
     14},
    /* Negative Voltage Gamma Control */
    {0xE1,
     {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18,
      0x16, 0x19},
     14},
    /* Display Inversion On */
    {0x21, {0}, 0},
    /* Sleep Out */
    {0x11, {0}, 0x80},
    /* Display On */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff}};

DRAM_ATTR static const lcd_init_cmd_t ili_init_cmds[] = {
    /* Power contorl B, power control = 0, DC_ENA = 1 */
    {0xCF, {0x00, 0x83, 0X30}, 3},
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
    {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
    /* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
    {0xE8, {0x85, 0x01, 0x79}, 3},
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    /* Pump ratio control, DDVDH=2xVCl */
    {0xF7, {0x20}, 1},
    /* Driver timing control, all=0 unit */
    {0xEA, {0x00, 0x00}, 2},
    /* Power control 1, GVDD=4.75V */
    {0xC0, {0x26}, 1},
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    {0xC1, {0x11}, 1},
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    {0xC5, {0x35, 0x3E}, 2},
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    {0xC7, {0xBE}, 1},
    /* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
    {0x36, {0x28}, 1},
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Frame rate control, f=fosc, 70Hz fps */
    {0xB1, {0x00, 0x1B}, 2},
    /* Enable 3G, disabled */
    {0xF2, {0x08}, 1},
    /* Gamma set, curve 1 */
    {0x26, {0x01}, 1},
    /* Positive gamma correction */
    {0xE0,
     {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02,
      0x07, 0x05, 0x00},
     15},
    /* Negative gamma correction */
    {0XE1,
     {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D,
      0x38, 0x3A, 0x1F},
     15},
    /* Column address set, SC=0, EC=0xEF */
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    /* Page address set, SP=0, EP=0x013F */
    {0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
    /* Memory write */
    {0x2C, {0}, 0},
    /* Entry mode set, Low vol detect disabled, normal display */
    {0xB7, {0x07}, 1},
    /* Display function control */
    {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
    /* Sleep out */
    {0x11, {0}, 0x80},
    /* Display on */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff},
};

/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 *
 * Since command transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active) {
  esp_err_t ret;
  spi_transaction_t t;
  memset(&t, 0, sizeof(t)); // Zero out the transaction
  t.length = 8;             // Command is 8 bits
  t.tx_buffer = &cmd;       // The data is the cmd itself
  t.user = (void *)0;       // D/C needs to be set to 0
  if (keep_cs_active) {
    t.flags = SPI_TRANS_CS_KEEP_ACTIVE; // Keep CS active after data transfer
  }
  ret = spi_device_polling_transmit(spi, &t); // Transmit!
  assert(ret == ESP_OK);                      // Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len) {
  esp_err_t ret;
  spi_transaction_t t;
  if (len == 0) {
    return; // no need to send anything
  }
  memset(&t, 0, sizeof(t)); // Zero out the transaction
  t.length = len * 8;       // Len is in bytes, transaction length is in bits.
  t.tx_buffer = data;       // Data
  t.user = (void *)1;       // D/C needs to be set to 1
  ret = spi_device_polling_transmit(spi, &t); // Transmit!
  assert(ret == ESP_OK);                      // Should have had no issues.
}

// This function is called (in irq context!) just before a transmission starts.
// It will set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
  int dc = (int)t->user;
  gpio_set_level(PIN_NUM_DC, dc);
}

uint32_t lcd_get_id(spi_device_handle_t spi) {
  // When using SPI_TRANS_CS_KEEP_ACTIVE, bus must be locked/acquired
  spi_device_acquire_bus(spi, portMAX_DELAY);

  // get_id cmd
  lcd_cmd(spi, 0x04, true);

  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 8 * 3;
  t.flags = SPI_TRANS_USE_RXDATA;
  t.user = (void *)1;

  esp_err_t ret = spi_device_polling_transmit(spi, &t);
  assert(ret == ESP_OK);

  // Release bus
  spi_device_release_bus(spi);

  return *(uint32_t *)t.rx_data;
}

// Initialize the display
void lcd_init(spi_device_handle_t spi) {
  int cmd = 0;
  const lcd_init_cmd_t *lcd_init_cmds;

  // Initialize non-SPI GPIOs
  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask =
      ((1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_BCKL));
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en = true;
  gpio_config(&io_conf);

  // Reset the display
  gpio_set_level(PIN_NUM_RST, 0);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  gpio_set_level(PIN_NUM_RST, 1);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // detect LCD type
  uint32_t lcd_id = lcd_get_id(spi);
  int lcd_detected_type = 0;
  int lcd_type;

  printf("LCD ID: %08" PRIx32 "\n", lcd_id);
  // if (lcd_id == 0) {
  //     //zero, ili
  //     lcd_detected_type = LCD_TYPE_ILI;
  //     printf("ILI9341 detected.\n");
  // } else {
  //     // none-zero, ST
  //     lcd_detected_type = LCD_TYPE_ST;
  //     printf("ST7789V detected.\n");
  // }
  // force ST7789V
  if (lcd_id == 0) {
    lcd_detected_type = LCD_TYPE_ST;
    printf("ST7789V detected.\n");
  }

#ifdef CONFIG_LCD_TYPE_AUTO
  lcd_type = lcd_detected_type;
#elif defined(CONFIG_LCD_TYPE_ST7789V)
  printf("kconfig: force CONFIG_LCD_TYPE_ST7789V.\n");
  lcd_type = LCD_TYPE_ST;
#elif defined(CONFIG_LCD_TYPE_ILI9341)
  printf("kconfig: force CONFIG_LCD_TYPE_ILI9341.\n");
  lcd_type = LCD_TYPE_ILI;
#endif
  if (lcd_type == LCD_TYPE_ST) {
    printf("LCD ST7789V initialization.\n");
    lcd_init_cmds = st_init_cmds;
  } else {
    printf("LCD ILI9341 initialization.\n");
    lcd_init_cmds = ili_init_cmds;
  }

  // Send all the commands
  while (lcd_init_cmds[cmd].databytes != 0xff) {
    if (lcd_init_cmds[cmd].cmd != 0x21 ||
        lcd_init_cmds[cmd].cmd != 0x20) // Handle cmd with parameters
    {
      lcd_cmd(spi, lcd_init_cmds[cmd].cmd, false);
      lcd_data(spi, lcd_init_cmds[cmd].data,
               lcd_init_cmds[cmd].databytes & 0x1F);
      // If command with arg, delay
      if (lcd_init_cmds[cmd].databytes & 0x80) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
    } else // No parameters
    {
      lcd_cmd(spi, lcd_init_cmds[cmd].cmd, false);
    }
    cmd++;
  }
  /// Enable backlight
  gpio_set_level(PIN_NUM_BCKL, LCD_BK_LIGHT_ON_LEVEL);
}

static void send_line_finish(spi_device_handle_t spi) {
  spi_transaction_t *rtrans;
  esp_err_t ret;
  // Wait for all 6 transactions to be done and get back the results.
  for (int x = 0; x < 6; x++) {
    ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
    assert(ret == ESP_OK);
    // We could inspect rtrans now if we received any info back. The LCD is
    // treated as write-only, though.
  }
}

static void lcd_draw_one_pixel(spi_device_handle_t spi, uint16_t x, uint16_t y,
                               uint16_t color) {
  esp_err_t ret;
  spi_transaction_t trans[6];

  // Column Address Set
  memset(&trans[0], 0, sizeof(spi_transaction_t));
  trans[0].length = 8;
  trans[0].tx_data[0] = 0x2A;
  trans[0].flags = SPI_TRANS_USE_TXDATA;
  trans[0].user = (void *)0;

  memset(&trans[1], 0, sizeof(spi_transaction_t));
  trans[1].length = 8 * 4;
  trans[1].tx_data[0] = x >> 8;
  trans[1].tx_data[1] = x & 0xff;
  trans[1].tx_data[2] = x >> 8;
  trans[1].tx_data[3] = x & 0xff;
  trans[1].flags = SPI_TRANS_USE_TXDATA;
  trans[1].user = (void *)1;

  // Row Address Set
  memset(&trans[2], 0, sizeof(spi_transaction_t));
  trans[2].length = 8;
  trans[2].tx_data[0] = 0x2B;
  trans[2].flags = SPI_TRANS_USE_TXDATA;
  trans[2].user = (void *)0;

  memset(&trans[3], 0, sizeof(spi_transaction_t));
  trans[3].length = 8 * 4;
  trans[3].tx_data[0] = y >> 8;
  trans[3].tx_data[1] = y & 0xff;
  trans[3].tx_data[2] = y >> 8;
  trans[3].tx_data[3] = y & 0xff;
  trans[3].flags = SPI_TRANS_USE_TXDATA;
  trans[3].user = (void *)1;

  // Memory Write
  memset(&trans[4], 0, sizeof(spi_transaction_t));
  trans[4].length = 8;
  trans[4].tx_data[0] = 0x2C;
  trans[4].flags = SPI_TRANS_USE_TXDATA;
  trans[4].user = (void *)0;

  memset(&trans[5], 0, sizeof(spi_transaction_t));
  trans[5].length = 16;
  trans[5].tx_data[0] = color >> 8;
  trans[5].tx_data[1] = color & 0xff;
  trans[5].flags = SPI_TRANS_USE_TXDATA;
  trans[5].user = (void *)1;

  // Queue all transactions.
  for (int i = 0; i < 6; i++) {
    ret = spi_device_queue_trans(spi, &trans[i], portMAX_DELAY);
    assert(ret == ESP_OK);
  }

  // Wait for all transactions to be done.
  for (int i = 0; i < 6; i++) {
    spi_transaction_t *rtrans;
    ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}

static void lcd_draw_rectangle(spi_device_handle_t spi, uint16_t x, uint16_t y,
                               uint16_t width, uint16_t height,
                               uint16_t color) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      lcd_draw_one_pixel(spi, x + i, y + j, color);
    }
  }
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
  ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_panel_handle, X_OFFSET, Y_OFFSET)); // Set Offset
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
      .buffer_size = DISP_WIDTH * DISP_HEIGHT / 2,
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
}
