/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_netif_sntp.h"
#include "nvs_cfg.h"

static const char *TAG = "time_sync";

/* Timer interval once every day (24 Hours) */
#define NVS_TIME_PERIOD (86400000000ULL)


static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
#if CONFIG_LWIP_SNTP_MAX_SERVERS >= 2
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2,
                               ESP_SNTP_SERVER_LIST("ntp1.aliyun.com", "time.windows.com" ) );
#else
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(1,
                               ESP_SNTP_SERVER_LIST("time.windows.com") );
#endif
    esp_netif_sntp_init(&config);
}

static esp_err_t obtain_time(void)
{
    // wait for time to be set
    int retry = 0;
    const int retry_count = 10;
    while (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(2000)) != ESP_OK && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    if (retry == retry_count) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t fetch_and_store_time_in_nvs(void *args)
{
    initialize_sntp();

    esp_err_t err = obtain_time();
    if (err == ESP_OK){
        time_t now;
        time(&now);

        err = nvs_cfg_save_time(now);
    }

    esp_netif_sntp_deinit();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error updating time in nvs");
    } else {
        ESP_LOGI(TAG, "Updated time in NVS");
    }
    return err;
}

static esp_err_t update_time_from_nvs(void)
{
    int64_t timestamp = 0;
    esp_err_t err = nvs_cfg_load_time(&timestamp);
    if (err != ESP_OK) { // ESP_ERR_NVS_NOT_FOUND
        ESP_LOGI(TAG, "Time not found in NVS. Syncing time from SNTP server.");
        err = fetch_and_store_time_in_nvs(NULL);
    } else {
        struct timeval get_nvs_time;
        get_nvs_time.tv_sec = timestamp;
        settimeofday(&get_nvs_time, NULL);
    }

    return err;
}


esp_err_t time_sync_init(void)
{
    if (esp_reset_reason() == ESP_RST_POWERON) {
        ESP_LOGI(TAG, "Updating time from NVS");
        ESP_ERROR_CHECK(update_time_from_nvs());
    }

    const esp_timer_create_args_t nvs_update_timer_args = {
        .callback = (void *)&fetch_and_store_time_in_nvs,
    };

    esp_timer_handle_t nvs_update_timer;
    ESP_ERROR_CHECK(esp_timer_create(&nvs_update_timer_args, &nvs_update_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(nvs_update_timer, NVS_TIME_PERIOD));
    return ESP_OK;
}