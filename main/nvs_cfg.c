#include <string.h> // strcpy
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs_cfg.h"

static const char *TAG = "NVS_CFG";
/* 用于读取nvs的命名空间 */
static const char* namespace_array[] = { "wifi_info", "time_info", "xmiot_info", "weather_info" };


#if !CONFIG_SWITCH86_UI_ENABLE // 没有UI，用配置信息
static int nvs_cfg_read_def(void* arg, const char* key, char* value, size_t vsize)
{
    //int namespace_type = *((int*)arg);
    memset(value, 0, vsize);
    if (strcmp(key, "wifi_ssid") == 0){
        strncpy(value, CONFIG_SWITCH86_UI_OFF_WIFI_SSID, vsize);
    }
    else if (strcmp(key, "wifi_passwd") == 0){
        strncpy(value, CONFIG_SWITCH86_UI_OFF_WIFI_PASSWORD, vsize);
    }
    return 0;
}
#endif

static int nvs_cfg_read(void* arg, const char* key, char* value, size_t vsize)
{
    memset(value, 0, vsize);
    esp_err_t ret = nvs_get_str((nvs_handle)arg, key, value, &vsize);
    ESP_LOGI(TAG, "key:%s, value:%s, ret:%d", key, value,ret);
	return ret;
}
static int nvs_cfg_write(void* arg, const char* key, const char* value) 
{
    esp_err_t ret = nvs_set_str((nvs_handle)arg, key, value);
    ESP_LOGI(TAG, "key:%s, value:%s, ret:%d", key, value,ret);
	return ret;
}


esp_err_t nvs_cfg_init(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    return ret;
}

esp_err_t nvs_cfg_check(int namespace_type){
    nvs_handle wifi_info_handle;
    esp_err_t ret = nvs_open(namespace_array[namespace_type], NVS_READONLY, &wifi_info_handle);
    if (ret == ESP_OK){
        nvs_close(wifi_info_handle);
    }
    return ret;
}

esp_err_t nvs_cfg_load(int namespace_type, int (load_cb)(void* ctx, nvs_cfg_read_cb_t read_cb, void* arg), void *ctx){
    nvs_handle wifi_info_handle;
    esp_err_t ret = nvs_open(namespace_array[namespace_type], NVS_READONLY, &wifi_info_handle);
    if (ret == ESP_OK){
        ret = load_cb(ctx, nvs_cfg_read, (void*)wifi_info_handle);
        nvs_close(wifi_info_handle);
    }
#if !CONFIG_SWITCH86_UI_ENABLE // 没有UI，用配置信息
    else if (namespace_type == NVS_CFG_WIFI_INFO_NAMESPACE) {
        ret = load_cb(ctx, nvs_cfg_read_def, &namespace_type);
    }
#endif
    return ret;
}

esp_err_t nvs_cfg_save(int namespace_type, int (save_cb)(void* ctx, nvs_cfg_write_cb_t write_cb, void* arg), void *ctx){
    nvs_handle wifi_info_handle;
    ESP_ERROR_CHECK(nvs_open(namespace_array[namespace_type], NVS_READWRITE, &wifi_info_handle));
    
    int ret = save_cb(ctx, nvs_cfg_write, (void*)wifi_info_handle);

    ESP_ERROR_CHECK(nvs_commit(wifi_info_handle));
    nvs_close(wifi_info_handle);
    return ret;
}

esp_err_t nvs_cfg_load_time(int64_t* out_value) {
    nvs_handle time_info_handle;
    esp_err_t ret = nvs_open(namespace_array[NVS_CFG_TIME_INFO_NAMESPACE], NVS_READONLY, &time_info_handle);
    if (ret == ESP_OK){
        ret = nvs_get_i64(time_info_handle, "timestamp", out_value);
        nvs_close(time_info_handle);
    }
    return ret;
}

esp_err_t nvs_cfg_save_time(int64_t value) {
    nvs_handle time_info_handle;
    ESP_ERROR_CHECK(nvs_open(namespace_array[NVS_CFG_TIME_INFO_NAMESPACE], NVS_READWRITE, &time_info_handle));
    
    int ret = nvs_set_i64(time_info_handle, "timestamp", value);

    ESP_ERROR_CHECK(nvs_commit(time_info_handle));
    nvs_close(time_info_handle);
    return ret;
}
