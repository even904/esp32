set(srcs
    "main.c"
    "decode_image.c"
    "wifi_app.c"
    "fonts/WeatherIcon4b.c"
    "fonts/WenQuanWeiMiHei_12.c"
    "fonts/WenQuanWeiMiHei_18.c"
    "fonts/WenQuanWeiMiHei_24.c"
    "fonts/WenQuanWeiMiHei_36.c"
    "fonts/WenQuanWeiMiHei_48.c"
    "lv_custom_ui.c"
    "lcd_lv_init.c"
    "parse_json.c"
    )

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES
                    amap_com_root_cert.pem)
