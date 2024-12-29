/*
 * 用nvs存储配置
 */

#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

enum nvs_cfg_namespace_t
{
    NVS_CFG_WIFI_INFO_NAMESPACE,
    NVS_CFG_XMIOT_INFO_NAMESPACE,
    NVS_CFG_WEATHER_INFO_NAMESPACE,
};

typedef int (*nvs_cfg_read_cb_t)(void* arg, const char* key, char* value, size_t vsize);
typedef int (*nvs_cfg_write_cb_t)(void* arg, const char* key, const char* value);

esp_err_t nvs_cfg_init(void);

esp_err_t nvs_cfg_check(int namespace_type);
esp_err_t nvs_cfg_load(int namespace_type, int (load_cb)(void* ctx, nvs_cfg_read_cb_t read_cb, void* arg), void *ctx);
esp_err_t nvs_cfg_save(int namespace_type, int (save_cb)(void* ctx, nvs_cfg_write_cb_t write_cb, void* arg), void *ctx);

#ifdef __cplusplus
}
#endif
