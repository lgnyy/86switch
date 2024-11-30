
# 将反斜杠替换为正斜杠
set(IDF_PATH_UNIX $ENV{IDF_PATH})
string(REGEX REPLACE "\\\\" "/" IDF_PATH_UNIX ${IDF_PATH_UNIX})

idf_component_register(SRCS "i2s_recorder_main.c"
                    PRIV_REQUIRES esp_driver_i2s fatfs driver
                    INCLUDE_DIRS "${IDF_PATH_UNIX}/examples/peripherals/i2s/common")
