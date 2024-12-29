/* WiFi station 
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_cfg.h"


#define WIFI_MAXIMUM_RETRY  5

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

typedef struct _wifi_event_arg_t{
    EventGroupHandle_t wifi_event_group; /* FreeRTOS event group to signal when we are connected*/
    int retry_num ;
}wifi_event_arg_t;


static void event_handler(void* arg_, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    wifi_event_arg_t* arg = (wifi_event_arg_t*)arg_;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (arg->retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            arg->retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(arg->wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        arg->retry_num = 0;
        xEventGroupSetBits(arg->wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_connect_ap(wifi_config_t* wifi_config, bool is_first)
{
    wifi_event_arg_t event_arg = {.retry_num=0, .wifi_event_group = xEventGroupCreate()};

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        &event_arg,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        &event_arg,
                                                        &instance_got_ip));


    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, wifi_config) );

    ESP_ERROR_CHECK(is_first? esp_wifi_start() : esp_wifi_connect() );

    ESP_LOGI(TAG, "%s", (is_first? "esp_wifi_start." : "esp_wifi_connect."));

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(event_arg.wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi_config->sta.ssid, wifi_config->sta.password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config->sta.ssid, wifi_config->sta.password);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
    vEventGroupDelete(event_arg.wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT)? ESP_OK : ESP_FAIL;
}


static int wifi_load_config(void* ctx, nvs_cfg_read_cb_t read_cb, void* arg){
    wifi_sta_config_t* sta = (wifi_sta_config_t*)ctx;
    read_cb(arg, "wifi_ssid", (char*)(sta->ssid), sizeof(sta->ssid));
    return read_cb(arg, "wifi_passwd", (char*)(sta->password), sizeof(sta->password));
}
static int wifi_save_config(void* ctx, nvs_cfg_write_cb_t write_cb, void* arg){
    wifi_sta_config_t* sta = (wifi_sta_config_t*)ctx;
    write_cb(arg, "wifi_ssid", (char*)(sta->ssid));
    return write_cb(arg, "wifi_passwd", (char*)(sta->password));
}


esp_err_t wifi_station_init(void)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );


    wifi_config_t wifi_config = {
        .sta = {
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .sae_h2e_identifier = "",
        },
    };

    // 加载wifi配置
    int ret = nvs_cfg_load(NVS_CFG_WIFI_INFO_NAMESPACE, wifi_load_config, &(wifi_config.sta));
    if (ret == 0){
        ret = wifi_connect_ap(&wifi_config, true);
    }
    return ret;
}


int wifi_station_scan(char* ssids, size_t max_size)
{
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));

    uint16_t number = 10;
    wifi_ap_record_t ap_info[10];

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_LOGI(TAG, "Actual AP number ap_info holds = %u", number);
    ssids[0] = '\0';
    for (int i = 0; i < number; i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        size_t tlen = strlen((char*)(ap_info[i].ssid)) + ((i==0)?0 : 1);
        if (max_size <= tlen){
            break;
        }
        
        if (i > 0){
            strcpy(ssids, "\n");
        }
        strcat(ssids, (char*)(ap_info[i].ssid));

        ssids += tlen;
        max_size -= tlen;
    }
    return 0;
}

int wifi_station_connect(const char* ssid, const char* password)
{
    wifi_config_t wifi_config = {
        .sta = {
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .sae_h2e_identifier = "",
        },
    };
    strncpy((char*)(wifi_config.sta.ssid), ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)(wifi_config.sta.password), password, sizeof(wifi_config.sta.password));
 
    esp_wifi_disconnect();
  
    int ret = wifi_connect_ap(&wifi_config, false);
    ESP_LOGW(TAG, "wifi_connect_ap: %d", ret);
    if (ret == 0){ // 连接成功后保存配置
        int ret2 = nvs_cfg_save(NVS_CFG_WIFI_INFO_NAMESPACE, wifi_save_config, &(wifi_config.sta));
        ESP_LOGW(TAG, "nvs_cfg_save: %d", ret2);
    }
    return ret;
}
