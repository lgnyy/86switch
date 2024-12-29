/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_netif_sntp.h"


static const char *TAG = "time_sync";

/* Timer interval once every day (24 Hours) */
#define NVS_TIME_PERIOD (86400000000ULL)


//static void time_sync_event(struct timeval *tv)
//{
//    ESP_LOGI(TAG, "time_sync_event");
//}

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
    //config.sync_cb = time_sync_event;
    esp_netif_sntp_init(&config);
}

static esp_err_t start_sntp(void *args)
{
    return esp_netif_sntp_start();
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

esp_err_t time_sync_init(void)
{
    //if (esp_reset_reason() == ESP_RST_POWERON) {
    //    ESP_LOGI(TAG, "Updating time from NVS");
    //}
    setenv("TZ", "CST-8", 1); // GMT+8

    initialize_sntp();
    obtain_time();
    //esp_netif_sntp_deinit();

    const esp_timer_create_args_t nvs_update_timer_args = {
        .callback = (void *)&start_sntp,
    };

    esp_timer_handle_t nvs_update_timer;
    ESP_ERROR_CHECK(esp_timer_create(&nvs_update_timer_args, &nvs_update_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(nvs_update_timer, NVS_TIME_PERIOD));
    return ESP_OK;
}
