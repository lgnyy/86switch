/*
 * wifi搜索及连接
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int wifi_station_scan(char* ssids, size_t max_size);
int wifi_station_connect(const char* ssid, const char* password);

#ifdef __cplusplus
}
#endif
