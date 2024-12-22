#include <string.h> // strcpy
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs_cfg.h"


#define NVS_XMIOT_INFO_NAMESPACE  "xmiot_info" /* 用于读取nvs的命名空间 */
#define NVS_WIFI_INFO_NAMESPACE  "wifi_info" /* 用于读取nvs的命名空间 */

static const char *TAG = "NVS_CFG";


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

int nvs_cfg_load_wifi(int (load_cb)(void* ctx, nvs_cfg_read_cb_t read_cb, void* arg), void *ctx){
    nvs_handle wifi_info_handle;
    esp_err_t ret = nvs_open(NVS_WIFI_INFO_NAMESPACE, NVS_READONLY, &wifi_info_handle);
    if (ret == ESP_OK){
        ret = load_cb(ctx, nvs_cfg_read, (void*)wifi_info_handle);
        nvs_close(wifi_info_handle);
    }
    return ret;
}

int nvs_cfg_save_wifi(int (save_cb)(void* ctx, nvs_cfg_write_cb_t write_cb, void* arg), void *ctx){
    nvs_handle wifi_info_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_WIFI_INFO_NAMESPACE, NVS_READWRITE, &wifi_info_handle));
    
    int ret = save_cb(ctx, nvs_cfg_write, (void*)wifi_info_handle);

    ESP_ERROR_CHECK(nvs_commit(wifi_info_handle));
    nvs_close(wifi_info_handle);
    return ret;
}


int nvs_cfg_load_xmiot(int (load_cb)(void* ctx, nvs_cfg_read_cb_t read_cb, void* arg), void *ctx){
    nvs_handle xmiot_info_handle;
    esp_err_t ret = nvs_open(NVS_XMIOT_INFO_NAMESPACE, NVS_READONLY, &xmiot_info_handle);
    if (ret == ESP_OK){
        ret = load_cb(ctx, nvs_cfg_read, (void*)xmiot_info_handle);
        nvs_close(xmiot_info_handle);
    }
    return ret;
}

int nvs_cfg_save_xmiot(int (save_cb)(void* ctx, nvs_cfg_write_cb_t write_cb, void* arg), void *ctx){
    nvs_handle xmiot_info_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_XMIOT_INFO_NAMESPACE, NVS_READWRITE, &xmiot_info_handle));
    
    int ret = save_cb(ctx, nvs_cfg_write, (void*)xmiot_info_handle);

    ESP_ERROR_CHECK(nvs_commit(xmiot_info_handle));
    nvs_close(xmiot_info_handle);
    return ret;
}