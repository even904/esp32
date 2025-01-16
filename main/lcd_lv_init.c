#include "lcd_lv_init.h"

static const char* lcd_lv_init_TAG = "LCD_Init";
// This function is called (in irq context!) just before a transmission starts.
// It will set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}

typedef struct
{
    uint16_t width;
    uint16_t height;
} esp_lcd_panel_st7789_config_t;

void st7789_spi_init(void)
{
    esp_err_t           ret;
    spi_device_handle_t spi;
    spi_bus_config_t    buscfg
        = {.miso_io_num     = PIN_NUM_MISO,
           .mosi_io_num     = PIN_NUM_MOSI,
           .sclk_io_num     = PIN_NUM_CLK,
           .quadwp_io_num   = -1,
           .quadhd_io_num   = -1,
           .max_transfer_sz = PARALLEL_LINES * 320 * 2 + 8};
    spi_device_interface_config_t devcfg = {
#ifdef CONFIG_LCD_OVERCLOCK
        .clock_speed_hz = 26 * 1000 * 1000,  // Clock out at 26 MHz
#else
        .clock_speed_hz = 10 * 1000 * 1000,  // Clock out at 10 MHz
#endif
        .mode         = 0,                              // SPI mode 0
        .spics_io_num = PIN_NUM_CS,                     // CS pin
        .queue_size   = 7,                              // We want to be able to queue 7 transactions at a time
        .pre_cb       = lcd_spi_pre_transfer_callback,  // Specify pre-transfer callback
                                                        // to handle D/C line
    };
    // Initialize the SPI bus
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    // Attach the LCD to the SPI bus
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
}

lv_disp_t *lvgl_config_init(void)
{
    lv_disp_t *disp_handle = NULL;
    // 2. Install LCD panel driver
    // 2.1 Configure panel IO
    ESP_LOGI(lcd_lv_init_TAG, "Install ST7789 panel driver");
    ESP_LOGI(lcd_lv_init_TAG, "Configuring ST7789 Panel IO");
    esp_lcd_spi_bus_handle_t      spi_bus = LCD_HOST;
    esp_lcd_panel_io_spi_config_t io_config
        = {.cs_gpio_num       = PIN_NUM_CS,
           .dc_gpio_num       = PIN_NUM_DC,
           .pclk_hz           = 10 * 1000 * 1000,
           .spi_mode          = 0,
           .trans_queue_depth = 10,
           .lcd_cmd_bits      = 8,
           .lcd_param_bits    = 8};

    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_LOGI(lcd_lv_init_TAG, "Configuring ST7789 Panel");
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(spi_bus, &io_config, &io_handle));
    // 2.2 Configure panel devive
    esp_lcd_panel_handle_t        lcd_panel_handle = NULL;
    esp_lcd_panel_st7789_config_t st7789_config    = {.width = DISP_WIDTH, .height = DISP_HEIGHT};
    esp_lcd_panel_dev_config_t    panel_config
        = {.bits_per_pixel = 16, .reset_gpio_num = PIN_NUM_RST, .vendor_config = &st7789_config};
    esp_lcd_new_panel_st7789(io_handle, &panel_config, &lcd_panel_handle);

    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_panel_handle, X_OFFSET,
                                          Y_OFFSET));  // Set Offset
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_panel_handle, true));
    // check_heap_memory();
    // 3. Start LVGL Port Task
    lv_init();
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
    // 4. Add Display
    ESP_LOGI(lcd_lv_init_TAG, "Initialize LVGL");
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
    ESP_LOGI(lcd_lv_init_TAG, "Deinitialized num of disp_handle: %x", (uintptr_t)(disp_handle));
    disp_handle = lvgl_port_add_disp(&disp_cfg);
    ESP_LOGI(lcd_lv_init_TAG, "SizeOf lv_color_t: %d", (int)sizeof(lv_color_t));
    ESP_LOGI(lcd_lv_init_TAG, "Initialized num of disp_handle: %x", (uintptr_t)(disp_handle));
    return disp_handle;
}