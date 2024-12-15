
file(GLOB YOS_ESP_SRC_FILES yos/src_esp/*.c)
file(GLOB YOS_SRC_FILES yos/*.c)
set(YOS_SRC_FILES ${YOS_SRC_FILES} ${YOS_ESP_SRC_FILES})

file(GLOB XMIOT_SRC_FILES xmiot/*.c)

idf_component_register(
    SRCS "86switch_main.c" "lvgl_port.c" "wifi_station.c" "http_client.c" "time_sync.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_event)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
