/*
 * 录音及语音识别
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_websocket_client.h"
#include "yos_nvs.h"
#include "fvad.h"
#if CONFIG_SWITCH86_XMIOT_ENABLE
#include "xmiot_service.h"
#else
#include "miot_cloud.h"
#endif

#if CONFIG_SWITCH86_MIC_INMP411_ENABLE

static const char *TAG = "REC_ASR";
static void (*_lcd_sleep_cb)(bool sleep);


// 用新任务执行音箱命令
#if CONFIG_SWITCH86_XMIOT_ENABLE
static void send_speaker_cmd_task(void *pvParameters)
{
    char* cmd = (char*)pvParameters;
    void* ctx = xmiot_service_context_create();
    if (ctx != NULL){
        if (yos_nvs_load(YOS_NVS_XMIOT_INFO_NAMESPACE, xmiot_service_load_config, ctx) == 0){

            xmiot_service_send_speaker_cmd(ctx, cmd);
        }
    
        xmiot_service_context_destory(ctx);
    }

    free(cmd);
    vTaskDelete(NULL);
}

// 发送（小爱）音箱命令
static void send_speaker_cmd(const char* data, size_t cmd_len)
{
    char* arg = (char*)malloc(cmd_len + 1);
    if (arg != NULL){
        memcpy(arg, data, cmd_len);
        arg[cmd_len] = '\0';

        // 开启新任务
        xTaskCreate(&send_speaker_cmd_task, "cmd_task", 4096, arg, 5, NULL);
    }
}

#else
static void send_speaker_cmd_task(void *pvParameters)
{
    char* value = (char*)pvParameters;
    char device_speaker[32], access_token[256];
    yos_nvs_item_t items[] = {{"device_speaker", device_speaker, sizeof(device_speaker)}, {"access_token", access_token, sizeof(access_token)}};
    if (yos_nvs_load_ex(YOS_NVS_MIOT_INFO_NAMESPACE, items, 2) == 0){
        char* pos1 = strchr(device_speaker, ',');
        int siid = 5, aiid = 5;
        if (pos1 != NULL){
            char* pos2 = strchr(pos1+1, ',');
            *pos1++ = '\0';
            siid = atoi(pos1);
            if (pos2 != NULL){
                *pos2++ = '\0';
                aiid = atoi(pos2);
            }
        } 
        miot_cloud_action(access_token, device_speaker, siid, aiid, value, NULL);
    }

    free(value);
    vTaskDelete(NULL);
}

static void send_speaker_cmd(const char* data, size_t cmd_len)
{
    char* arg = (char*)malloc(cmd_len + 10);
    if (arg != NULL){
        arg[0] = '"';
        memcpy(arg+1, data, cmd_len);
        memcpy(arg+1+cmd_len, "\",1", 4);

        ESP_LOGI(TAG, "esp_get_free_heap_size:%ld, _internal_heap_size: %ld", esp_get_free_heap_size(), esp_get_free_internal_heap_size());

        // 开启新任务
        BaseType_t res = xTaskCreate(&send_speaker_cmd_task, "cmd_task", 0x2000, arg, 5, NULL);
        if (res != pdPASS){
            ESP_LOGE(TAG, "Failed to create send_speaker_cmd_task");
            free(arg);
        }
    }
}
#endif

// 语音识别结果处理
static void asr_result_process(const char* data_ptr, int data_len)
{
    // 简化：直接查找text字段，没有用json解析
    int i;
    for (i=0; i<(data_len-8); i++){
        if (memcmp(data_ptr+i, "\"text\":\"", 8) == 0){
            const char* startptr = data_ptr+i+8;
            const char* endptr = strchr(startptr, '"');
            if (endptr != NULL){
                size_t clen = (size_t)(endptr-startptr);
                ESP_LOGI(TAG, "Received CMD=%.*s", clen,  startptr);
 
                // 在这修改唤醒词
                if ((memcmp(startptr, "丫丫", 6) == 0) || (memcmp(startptr, "小丫", 6) == 0)
                    || (memcmp(startptr, "压压", 6) == 0)){

                    send_speaker_cmd(startptr+6, clen-6);
                }
                else if((memcmp(startptr, "打开", 6) == 0) || (memcmp(startptr, "关闭", 6) == 0)){
                    if (memcmp(startptr+6, "夜灯", 6) == 0){
                        _lcd_sleep_cb(memcmp(startptr, "关闭", 6) == 0);
                    }
                    else {
                        send_speaker_cmd(startptr, clen);
                    }
                }
            }
        }
    }
}

// 初始化麦克风
static i2s_chan_handle_t init_microphone(void)
{
    i2s_chan_handle_t rx_handle = NULL;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    i2s_std_config_t rx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
            .bclk = CONFIG_SWITCH86_MIC_INMP411_I2S_CLK_GPIO,
            .ws   = CONFIG_SWITCH86_MIC_INMP411_I2S_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din  = CONFIG_SWITCH86_MIC_INMP411_I2S_DATA_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
	rx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
	//rx_std_cfg.slot_cfg.left_align = false;
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &rx_std_cfg));

    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    return rx_handle;
}

// ws事件处理
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA, Received opcode=%d", data->op_code);
        if (data->data_len > 0){
            ESP_LOGI(TAG, "Received data=%.*s", data->data_len, (char *)data->data_ptr);
            asr_result_process(data->data_ptr, data->data_len);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}

// ws任务
static void websocket_task(i2s_chan_handle_t rx_handle)
{
    //Websocket客户端设置配置
    esp_websocket_client_config_t websocket_cfg = {.uri=CONFIG_SWITCH86_ASR_WEBSOCKET_URL, .reconnect_timeout_ms=10000, .network_timeout_ms=6000};
 
    ESP_LOGI(TAG, "Connecting to %s...", websocket_cfg.uri);
    //创建websocket句柄
    esp_websocket_client_handle_t client = esp_websocket_client_init(&websocket_cfg);
    assert(client);
    //注册websocket回调函数用于检测执行过程中的一些状态，跟wifi连接回调函数一样
    ESP_ERROR_CHECK(esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client));
    //启动websocket任务，开始检测回调状态
    ESP_ERROR_CHECK(esp_websocket_client_start(client));

    // 监测是否有语音
    Fvad *fvad = fvad_new();
    assert(fvad);
    fvad_set_sample_rate(fvad, 16000);
    fvad_set_mode(fvad, 2);

    // https://github.com/modelscope/FunASR/blob/main/runtime/docs/websocket_protocol_zh.md
    const char* fun_asr_start_str = "{\"audio_fs\":16000,\"chunk_size\":[5,10,5],\"hotwords\":\"{\\\"丫丫\\\":20}\",\"is_speaking\":true,\"itn\":false,\"mode\":\"offline\",\"wav_format\":\"PCM\",\"wav_name\":\"h5\"}";
    const char* fun_asr_finish_str = "{\"is_speaking\":false}";

    char i2s_readraw_buff[32*30];  // 录音缓冲区
    int rec_cnt = 0; // 有效录音数量
    int nav_cnt = 0; // 五声数量
    while (true){
        size_t bytes_read = 0;
        ESP_ERROR_CHECK(i2s_channel_read(rx_handle, i2s_readraw_buff, sizeof(i2s_readraw_buff), &bytes_read, 1000));

        // 检查WebSocket连接状态
        if (!esp_websocket_client_is_connected(client)) {
            continue;
        }

        // 检查是否有语音
        int vs = fvad_process(fvad, (int16_t*)i2s_readraw_buff, bytes_read/2);
        if (rec_cnt == 0) {
            if (vs == 1) {
                esp_websocket_client_send_text(client, fun_asr_start_str, strlen(fun_asr_start_str), portMAX_DELAY);
                esp_websocket_client_send_bin(client, i2s_readraw_buff, bytes_read, portMAX_DELAY);
                rec_cnt = 1;
                nav_cnt = 0;
                ESP_LOGI(TAG, "start");
            }
            continue;
        }

        esp_websocket_client_send_bin(client, i2s_readraw_buff, bytes_read, portMAX_DELAY);
        rec_cnt ++;
        nav_cnt = (vs == 1)? 0 : (nav_cnt + 1);

        if ((rec_cnt >= 100) || (nav_cnt >= 10)){ // 3秒 或 连续300毫秒没有声音
            esp_websocket_client_send_text(client, fun_asr_finish_str, strlen(fun_asr_finish_str), portMAX_DELAY);
            ESP_LOGI(TAG, "rec_cnt=%d, nav_cnt=%d, end", rec_cnt, nav_cnt);
            rec_cnt = 0;
        }
    }
    

    fvad_free(fvad);
    ESP_LOGI(TAG, "Websocket Stopped");
    esp_websocket_client_stop(client);
    esp_websocket_client_destroy(client);
}

// 录音和语音识别任务
static void rec_asr_task(void *pvParameters)
{
    i2s_chan_handle_t rx_handle = (i2s_chan_handle_t)pvParameters;

    websocket_task(rx_handle);

    ESP_ERROR_CHECK(i2s_channel_disable(rx_handle));
    ESP_ERROR_CHECK(i2s_del_channel(rx_handle));
    vTaskDelete(NULL);
}

#if CONFIG_SWITCH86_XMIOT_ENABLE
#if !CONFIG_SWITCH86_UI_ENABLE
#include "xmiot_account.h"
static int xmiot_save_config(void* ctx, yos_nvs_write_cb_t write_cb, void* arg){
    int ret = xmiot_account_login_auth(NULL, SWITCH86_TEMPORARY_MIOT_USERNAME, SWITCH86_TEMPORARY_MIOT_PASSWORD, write_cb, arg);
    ESP_LOGW(TAG, "xmiot_account_login_auth ret:%d", ret);
    ret = xmiot_service_get_speaker_did(ctx, write_cb, arg);
    ESP_LOGW(TAG, "xmiot_service_get_speaker_did ret:%d", ret);
    return ret;
}
#endif
#endif

// 录音和语音识别初始化
esp_err_t rec_asr_init(void (*sleep_cb)(bool sleep))
{
    _lcd_sleep_cb = sleep_cb;
#if CONFIG_SWITCH86_XMIOT_ENABLE   
#if !CONFIG_SWITCH86_UI_ENABLE // 无UI，根据配置登录小米云服务，并保存token等
    void* ctx = xmiot_service_context_create();
    if (yos_nvs_load(YOS_NVS_XMIOT_INFO_NAMESPACE, xmiot_service_load_config, ctx) != 0){
        ESP_LOGW(TAG, "yos_nvs_load(xmiot) ret:%d", -1);
        yos_nvs_save(YOS_NVS_XMIOT_INFO_NAMESPACE, xmiot_save_config, ctx);
        xmiot_service_context_destory(ctx);
    }
#endif
#else
#ifdef CONFIG_SWITCH86_MIOT_ACCESS_TOKEN
    yos_nvs_item_t items[] = {{"access_token", (char*)CONFIG_SWITCH86_MIOT_ACCESS_TOKEN, 0},
        {"refresh_token", (char*)CONFIG_SWITCH86_MIOT_REFRESH_TOKEN, 0}, 
        {"expires_ts", (char*)CONFIG_SWITCH86_MIOT_EXPIRES_TS, 0},
         {"device_speaker", (char*)CONFIG_SWITCH86_MIOT_DEVICE_SPEAKER, 0}};
    yos_nvs_save_ex(YOS_NVS_MIOT_INFO_NAMESPACE, items, 4);
#endif
#endif

    i2s_chan_handle_t rx_handle = init_microphone();

    ESP_LOGI(TAG, "Create REC ASR task");
    xTaskCreate(&rec_asr_task, "rec_asr_task", 8192, rx_handle, 5, NULL);
    return ESP_OK;
}

#endif
