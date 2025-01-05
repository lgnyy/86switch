
if(CONFIG_SWITCH86_UI_ENABLE)
    file(GLOB UI_SRC_FILES ui/*.c)
endif()

idf_component_register(
    SRCS "86switch_main.c" "lvgl_port.c" "time_sync.c"  "rec_asr.c" ${UI_SRC_FILES}
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_event json)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
