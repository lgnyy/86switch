/*
 * 用nvs存储配置
 */

#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef int (*nvs_cfg_read_cb_t)(void* arg, const char* key, char* value, size_t vsize);
typedef int (*nvs_cfg_write_cb_t)(void* arg, const char* key, const char* value);

esp_err_t nvs_cfg_init(void);

int nvs_cfg_load_wifi(int (load_cb)(void* ctx, nvs_cfg_read_cb_t read_cb, void* arg), void *ctx);
int nvs_cfg_save_wifi(int (save_cb)(void* ctx, nvs_cfg_write_cb_t write_cb, void* arg), void *ctx);

int nvs_cfg_load_xmiot(int (load_cb)(void* ctx, nvs_cfg_read_cb_t read_cb, void* arg), void *ctx);
int nvs_cfg_save_xmiot(int (save_cb)(void* ctx, nvs_cfg_write_cb_t write_cb, void* arg), void *ctx);


#ifdef __cplusplus
}
#endif
