set(srcs
    "main.c"
    "decode_image.c"
   "wifi_app.c"
  "WeatherIcon.c"
  "lv_custom_ui.c"
  "lcd_lv_init.c"
  "parse_json.c"
    )

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES
                    amap_com_root_cert.pem)
