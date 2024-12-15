/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Update the system time from time stored in NVS.
 *
 */

esp_err_t time_sync_update_from_nvs(void);

/**
 * @brief Fetch the current time from SNTP and stores it in NVS.
 *
 */
esp_err_t time_sync_fetch_and_store_in_nvs(void*);

#ifdef __cplusplus
}
#endif
