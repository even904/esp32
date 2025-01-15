/* SPI Master example: jpeg decoder.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
The image used for the effect on the LCD in the SPI master example is stored in flash
as a jpeg file. This file contains the decode_image routine, which uses the tiny JPEG
decoder library to decode this JPEG into a format that can be sent to the display.

Keep in mind that the decoder library cannot handle progressive files (will give
``Image decoder: jd_prepare failed (8)`` as an error) so make sure to save in the correct
format if you want to use a different image file.
*/

#include "decode_image.h"
#include "jpeg_decoder.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <stdint.h>
#include <string.h>

const char *TAG = "ImageDec";

// Decode the embedded image into pixel lines that can be used with the rest of the logic.
esp_err_t decode_image(uint8_t **pixels, uint16_t *width, uint16_t *height, uint8_t *jpg_start, uint8_t *jpg_end)
{
    esp_err_t ret = ESP_OK;

    // Alocate pixel memory. Each line is an array of IMAGE_W 16-bit pixels; the `*pixels` array itself contains
    // pointers to these lines.
    *pixels = calloc(IMAGE_H * IMAGE_W, sizeof(uint16_t));
    ESP_GOTO_ON_FALSE(*pixels, ESP_ERR_NO_MEM, err, TAG, "Error allocating memory for lines");

    // JPEG decode config
    esp_jpeg_image_cfg_t jpeg_cfg
        = {.indata      = jpg_start,
           .indata_size = jpg_end - jpg_start,
           .outbuf      = *pixels,
           .outbuf_size = IMAGE_W * IMAGE_H * sizeof(uint16_t),
           .out_format  = JPEG_IMAGE_FORMAT_RGB565,
           .out_scale   = JPEG_IMAGE_SCALE_0,
           .flags       = {
                     .swap_color_bytes = 1,
           }};

    // JPEG decode
    esp_jpeg_image_output_t outimg;
    esp_jpeg_decode(&jpeg_cfg, &outimg);

    ESP_LOGI(TAG, "JPEG image decoded! Size of the decoded image is: %dpx x %dpx", outimg.width, outimg.height);
    *width  = outimg.width;
    *height = outimg.height;
    return ret;
err:
    // Something went wrong! Exit cleanly, de-allocating everything we allocated.
    if(*pixels != NULL)
    {
        free(*pixels);
    }
    return ret;
}
