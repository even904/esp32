set(srcs "pretty_effect.c"
    "spi_master_example_main.c"
    "decode_image.c"
   "wifi_app.c"
    )

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES
                    amap_com_root_cert.pem)
                    # EMBED_FILES image.jpg)
