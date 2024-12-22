
file(GLOB UI_SRC_FILES ui/*.c)

idf_component_register(
    SRCS "86switch_main.c" "lvgl_port.c" "nvs_cfg.c" "wifi_station.c" "time_sync.c"  "rec_asr.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_event)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
