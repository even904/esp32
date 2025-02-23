/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once
#include "esp_err.h"
#include <stdint.h>


#define IMAGE_W 96
#define IMAGE_H 96

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Decode the jpeg ``image.jpg`` embedded into the program file into pixel data.
     *
     * @param pixels A pointer to a pointer for an array of rows, which themselves are an array of pixels.
     *        Effectively, you can get the pixel data by doing ``decode_image(&myPixels);
     * pixelval=myPixels[ypos][xpos];``
     * @return - ESP_ERR_NOT_SUPPORTED if image is malformed or a progressive jpeg file
     *         - ESP_ERR_NO_MEM if out of memory
     *         - ESP_OK on succesful decode
     */
    esp_err_t decode_image(uint8_t **pixels, uint16_t *width, uint16_t *height, uint8_t *jpg_start, uint8_t *jpg_end);

#ifdef __cplusplus
}
#endif
