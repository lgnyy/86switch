#include <stdio.h>
#include <string.h>
#include <time.h>
#include "yos_nvs.h"
#include "yos_wifi.h"
#include "doMIoT.h"
#include "doWeather.h"
#include "doMain.h"
#ifdef _WIN32
#define localtime_r(_Time, _Tm) localtime_s(_Tm, _Time)
#endif

#define SCENE1_NAME "睡眠"
#define SCENE2_NAME "会客"
#define SCENE3_NAME "明亮"
#define SCENE4_NAME "观影"
#if CONFIG_SWITCH86_XMIOT_ENABLE
#define COMMAND_ON "打开"
#define COMMAND_OFF "关闭"
#define ROOM_NAME "卧室"
#define LIGHT_ALL_NAME "所有的灯"
#define LIGHT0_NAME "台灯"
#define LIGHT1_NAME "筒灯"
#define LIGHT2_NAME "吸顶灯"
#define LIGHT3_NAME "灯带"

static const char* lightp_on_command_list[] = {
    "",
    "",
    COMMAND_ON ROOM_NAME LIGHT0_NAME ",把亮度调到%ld，把色温调到%ld。",
};
static const char* lightp_off_command_list[] = {
    COMMAND_OFF LIGHT_ALL_NAME,
    COMMAND_OFF ROOM_NAME LIGHT_ALL_NAME,
    COMMAND_OFF ROOM_NAME LIGHT0_NAME,
};

static const char* light_on_command_list[] = {
    COMMAND_ON ROOM_NAME LIGHT_ALL_NAME,
    COMMAND_ON ROOM_NAME LIGHT1_NAME,
    COMMAND_ON ROOM_NAME LIGHT2_NAME,
    COMMAND_ON ROOM_NAME LIGHT3_NAME,
};
static const char* light_off_command_list[] = {
    COMMAND_OFF ROOM_NAME LIGHT_ALL_NAME,
    COMMAND_OFF ROOM_NAME LIGHT1_NAME,
    COMMAND_OFF ROOM_NAME LIGHT2_NAME,
    COMMAND_OFF ROOM_NAME LIGHT3_NAME,
};

static const char* scene_command_list[] = {"", SCENE1_NAME, SCENE2_NAME, SCENE3_NAME, SCENE4_NAME};

#else  /* #if CONFIG_SWITCH86_XMIOT_ENABLE */

const char* devLightp_did = "392563023";
const int devLightp_siid = 2;
const int devLightp_piids[] = {1, 2, 3, 0};

const char* devLights_did = "lumi.54ef441000339eca";
const int devLights_siids[] = {3, 2, 4, 0};
const int devLights_piid = 1;

#define miot_send_cmd(cmd) miot_action("402807754", 5, 5, cmd)

#define __SC(cmd) "\"" cmd "\",1"
static const char* scene_command_list[] = { "",  __SC(SCENE1_NAME), __SC(SCENE2_NAME), __SC(SCENE3_NAME), __SC(SCENE4_NAME) };
static const char* lightp_off_command_list[] = { __SC("关闭所有的灯") };
#endif  /* #if CONFIG_SWITCH86_XMIOT_ENABLE */

static bool weather_task_status = false;
static bool cmd_task_status = false;
static lv_thread_t threadConfig, threadWeather, threadCmd;

static char* merge_two_strings(const char* s1, const char* s2);
static void ui_load_cb(int32_t index);
static void wifi_command(int op, const char* ssid, const char* pswd);
#if CONFIG_SWITCH86_XMIOT_ENABLE
static void miot_command(int op, const char* username, const char* passsword);
#else
static void miot_command(int op, const char* reserve1, const char* reserve2);
#endif
static void weather_command(const char* city_pos, const char* api_key);
static void lightp_command(int32_t index, int32_t lightp, int32_t colorp);
static void light_command(int32_t index, bool on);
static void scene_command(int32_t index, bool on);
static void updateTime(lv_timer_t* timer);
static int updateWeather(void* arg, int index, const char* value);
static void getWeather(void* pvParameters);
typedef struct __command_param {
    char* cmd;
    int32_t index, lightp, colorp;
}_command_param_t;
static void sendCommand(const char* cmd);
static void sendCommand_param(_command_param_t* param);
static void refreshToken(lv_timer_t* timer);


void ui_main(void)
{
    ui_ScreenC1_set_command_cb(wifi_command);
    ui_ScreenC2_set_command_cb(miot_command);
    ui_ScreenC3_set_command_cb(weather_command);

    ui_Screen10_set_command_cb(lightp_command);
    ui_Screen11_set_command_cb(light_command);
    ui_Screen12_set_command_cb(scene_command);

    ui_init(ui_load_cb);

    int32_t index = (yos_nvs_check(YOS_NVS_WIFI_INFO_NAMESPACE) == 0) ? 0 : -1;
    lv_screen_load(ui_screen_get(index));

    lv_timer_create(updateTime, 1000, NULL);
    if (yos_nvs_check(YOS_NVS_WIFI_INFO_NAMESPACE) == 0) {
        weather_task_status = true;
        lv_thread_init(&threadWeather, LV_THREAD_PRIO_LOW, getWeather, 4096, NULL);
    }

    lv_timer_create(refreshToken, 60 * 1000, NULL);
}

static void refreshToken(lv_timer_t* timer) {
    time_t now = time(NULL);
    if (now > 1743500000) { // time ok
        int64_t expires_ts = 0;
        char expires_ts_str[32];
        if (miot_get_token_expires_ts(expires_ts_str) == 0) {
            expires_ts = atoll(expires_ts_str);
        }

        printf("now: %lld, expires_ts: %lld\n", now, expires_ts);
        if (now + 3600 < expires_ts) { // > 60 minute
            // 30 minute before
            //printf("new period: %d\n", (int)(expires_ts - 1800 - now));
            lv_timer_set_period(timer, (uint32_t)(expires_ts-1800-now) * 1000);
        }
        else {
            if (now < expires_ts) {
                sendCommand("refresh_token");
            }
            lv_timer_set_period(timer, 60 * 1000);
        }
    }
}

typedef struct __temp_load_context_t {
    const char** keys;
    void (*set_config_with_index)(int32_t index, const char* value);
}_temp_load_context_t;
static int load_config_cb(void* ctx_, yos_nvs_read_cb_t read_cb, void* arg) {
    _temp_load_context_t* ctx = (_temp_load_context_t*)ctx_;
    const char** keys = ctx->keys;
    char value[64];
    for (int i = 0; keys[i]; i++) {
        if (read_cb(arg, keys[i], value, sizeof(value)) == 0) {
            ctx->set_config_with_index(i, value);
        }
    }
    return 0;
}
static void ui_load_cb(int32_t index)
{
    if (index == -1) {
        char ip[20] = { 0 };
        yos_wifi_station_get_ip4(ip);
        ui_ScreenC1_set_config_with_index(0, ip);
    }
    else if (index == -2) {
#if CONFIG_SWITCH86_XMIOT_ENABLE
        _temp_load_context_t ctx = { miot_get_ui_config_keys(), ui_ScreenC2_set_config_with_index };
        yos_nvs_load(YOS_NVS_XMIOT_INFO_NAMESPACE, load_config_cb, &ctx);
#else
        _temp_load_context_t ctx = { miot_get_ui_config_keys(), ui_ScreenC2_set_config_with_index };
        yos_nvs_load(YOS_NVS_MIOT_INFO_NAMESPACE, load_config_cb, &ctx);
#endif
    }
    else if (index == -3) {
        _temp_load_context_t ctx = { weather_get_config_keys(), ui_ScreenC3_set_config_with_index };
        yos_nvs_load(YOS_NVS_WEATHER_INFO_NAMESPACE, load_config_cb, &ctx);
    }
}

static void lightp_command(int32_t index, int32_t lightp, int32_t colorp)
{
    _command_param_t* param = (_command_param_t*)malloc(sizeof(_command_param_t));
    if (param != NULL) {
        param->cmd = "table_lamp";
        param->index = index;
        param->lightp = lightp;
        param->colorp = colorp;
        sendCommand_param(param);
    }
}

static void light_command(int32_t index, bool on)
{
    _command_param_t* param = (_command_param_t*)malloc(sizeof(_command_param_t));
    if (param != NULL) {
        param->cmd = "lights";
        param->index = index;
        param->lightp = on ? 100 : 0;
        param->colorp = 0;
        sendCommand_param(param);
    }
}

static void scene_command(int32_t index, bool on)
{
    sendCommand(scene_command_list[index]);
}


static char* merge_two_strings(const char* s1, const char* s2)
{
    size_t len1 = strlen(s1) + 1;
    size_t len2 = strlen(s2) + 1;
    char* ss = (char*)malloc(len1 + len2);
    if (ss != NULL) {
        memcpy(ss, s1, len1);
        memcpy(ss + len1, s2, len2);
    }
    return ss;
}

static void wifi_command(int op, const char* ssid, const char* pswd)
{
    if (op == 1) {
        lv_thread_init(&threadConfig, LV_THREAD_PRIO_MID, wifi_scan_task, 4096, NULL);
    }
    else {
        char* ssid_pswd = merge_two_strings(ssid, pswd);
        if (ssid_pswd == NULL) {
            return;
        }

        lv_thread_init(&threadConfig, LV_THREAD_PRIO_MID, wifi_connect_task, 4096, ssid_pswd);
    }
}

void wifi_scan_task(void *pvParameters)
{
    lv_delay_ms(200);

    char ssids[256] = {0};
    yos_wifi_station_scan(ssids, sizeof(ssids));
  
    lv_lock();
    ui_ScreenC1_set_result(1, ssids);
    ui_ScreenC1_set_result(0, "PLS Connect WiFi");
    lv_unlock();
}

void wifi_connect_task(void *pvParameters)
{
    const char* ssid = (const char*)pvParameters;
    const char* pswd = ssid + strlen(ssid) + 1;

    lv_delay_ms(200);
    int ret = yos_wifi_station_connect(ssid, pswd);
   
    lv_lock();
    if (ret == 0) {
        char ip[20] = { 0 };
        yos_wifi_station_get_ip4(ip);
        ui_ScreenC1_set_config_with_index(0, ip);
    }
    ui_ScreenC1_set_result(0, (ret == 0)? "Success" : "Failed");
    lv_unlock();

    if (ret == 0){
        lv_delay_ms(1000);
        lv_lock();
        ui_screen_change(0);
        lv_unlock();

        if (!weather_task_status) {
            weather_task_status = true;
            lv_thread_init(&threadWeather, LV_THREAD_PRIO_LOW, getWeather, 4096, NULL);
        }
    }

    free(pvParameters);
}


#if CONFIG_SWITCH86_XMIOT_ENABLE
void miot_login_task(void* pvParameters) 
{
    const char* username = (const char*)pvParameters;
    const char* password = username + strlen(username) + 1;
    int ret = miot_login(username, password);

    lv_lock();
    ui_ScreenC2_set_result(0, (ret == 0) ? "Success" : "Failed");
    lv_unlock();

    free(pvParameters);
}

void miot_query_task(void* pvParameters)
{
    int ret = miot_query_speaker_did();

    lv_lock();
    ui_ScreenC2_set_result(0, (ret == 0) ? "Success" : "Failed");
    if (ret == 0) {
        ui_load_cb(-2);
    }
    lv_unlock();
}

static void miot_command(int op, const char* username, const char* passsword)
{
    if (op == 1) {
        char* name_pwd = merge_two_strings(username, passsword);
        if (name_pwd == NULL) {
            return;
        }

        lv_thread_init(&threadConfig, LV_THREAD_PRIO_MID, miot_login_task, 4096, name_pwd);
    }
    else {
        lv_thread_init(&threadConfig, LV_THREAD_PRIO_MID, miot_query_task, 4096, NULL);
    }
}
#else // #if CONFIG_SWITCH86_XMIOT_ENABLE


static void miot_command(int op, const char* reserve1, const char* reserve2)
{
    if (op == 1) {
        char url[512];
        miot_gen_auth_url(url, sizeof(url));
        ui_ScreenC2_set_result(1, url);

        lv_thread_init(&threadConfig, LV_THREAD_PRIO_MID, miot_login_task, 4096, NULL);
    }
    else {
        miot_set_speaker_did(reserve1);
        //lv_thread_init(&threadConfig, LV_THREAD_PRIO_MID, miot_query_task, 4096, NULL);
    }
}

void miot_login_task(void* pvParameters)
{
    int ret = miot_get_access_token();

    lv_lock();
    ui_ScreenC2_set_result(0, (ret == 0) ? "Success" : "Failed");
    lv_unlock();
}
#endif // #if CONFIG_SWITCH86_XMIOT_ENABLE

void weather_query_task(void* pvParameters)
{
    int ret = weather_query_first((char*)pvParameters, updateWeather, NULL);

    lv_lock();
    ui_ScreenC3_set_result(0, (ret == 0) ? "Success" : "Failed");
    lv_unlock();

    free(pvParameters);
}

static void weather_command(const char* city_pos, const char* api_key)
{
    char* pos_key = merge_two_strings(city_pos, api_key);
    if (pos_key == NULL) {
        return;
    }

    lv_thread_init(&threadConfig, LV_THREAD_PRIO_MID, weather_query_task, 4096, pos_key);
}


static void updateTime(lv_timer_t* timer)
{
    time_t now;
    struct tm timeinfo; 
 
    time(&now); 
    localtime_r(&now, &timeinfo);

    const char* weekArray[] = { "星期天", "星期一", "星期二", "星期三", "星期四", "星期五","星期六" };
    char topTimeText[16], topDateText[64];
    sprintf(topTimeText, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    sprintf(topDateText, "%s %d/%02d/%02d", weekArray[timeinfo.tm_wday], timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
 
    ui_Screen1_set_date_time(topDateText, topTimeText);
   
    ui_Screen13_set_time(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

static int updateWeather(void* arg, int index, const char* value) 
{
    printf("updateWeather index:%d, value:%s\n", index, value);
    lv_lock();
    ui_Screen1_set_weather_info(index, value);
    lv_unlock();
    return 0;
}

static void getWeather(void *pvParameters)
{
    lv_delay_ms(10 * 1000);
    while (1) {
        weather_query(updateWeather, NULL);

        lv_delay_ms(60 * 60 * 1000); // 1小时
    }
 }

static void cmd_task(void* arg)
{
    _command_param_t* param = (_command_param_t*)arg;
#if CONFIG_SWITCH86_XMIOT_ENABLE
    if (strcmp(param->cmd, "table_lamp") == 0){
        if (param->lightp > 0) {
            char cmd[256];
            const char* fmt = lightp_on_command_list[1 + param->index];
            snprintf(cmd, sizeof(cmd), fmt, param->lightp, 1700 + 48 * param->colorp);
            miot_send_cmd(cmd);
        }
        else {
            miot_send_cmd(lightp_off_command_list[1 + param->index]);
        }
    }
    else if (strcmp(param->cmd, "lights") == 0) {
        miot_send_cmd(param->lightp ? light_on_command_list[param->index] : light_off_command_list[param->index]);
    }
#else  /* #if CONFIG_SWITCH86_XMIOT_ENABLE */
    if (strcmp(param->cmd, "table_lamp") == 0) {
        if (param->lightp > 0) {
            char p[32], c[32];
            const char* values[] = { "true", p, c, NULL };
            snprintf(p, sizeof(p), "%ld", param->lightp);
            snprintf(c, sizeof(c), "%ld", 1700 + 48 * param->colorp);
            miot_set_props_piid(devLightp_did, devLightp_siid, devLightp_piids, values);
        }
        else {
            if (param->index < 0) {
                miot_send_cmd(lightp_off_command_list[0]);
            }
            else {
                if (param->index == 0) {
                    miot_set_props_siid(devLights_did, devLights_siids, devLights_piid, "false");
                }
                miot_set_prop(devLightp_did, devLightp_siid, devLightp_piids[0], "false");
            }
        }
    }
    else if (strcmp(param->cmd, "lights") == 0) {
        const char* value = param->lightp ? "true" : "false";
        if (param->index == 0) {
            miot_set_props_siid(devLights_did, devLights_siids, devLights_piid, value);
        }
        else {
            miot_set_prop(devLights_did, devLights_siids[param->index - 1], devLights_piid, value);
        }
    }
    else if (strcmp(param->cmd, "refresh_token") == 0) {
        miot_refresh_access_token();
    }
#endif  /* #if CONFIG_SWITCH86_XMIOT_ENABLE */
    else {
        miot_send_cmd(param->cmd);
    }

    lv_lock();
    cmd_task_status = false;
    lv_unlock();
    free(arg);
}
static void sendCommand(const char* cmd)
{
    _command_param_t* param = (_command_param_t*)malloc(sizeof(_command_param_t) + strlen(cmd) + 8);
    if (param == NULL) {
        return;
    }
    param->cmd = (char*)(param + 1);
    strcpy(param->cmd, cmd);
    sendCommand_param(param);
}

static void sendCommand_param(_command_param_t* param) 
{
    printf("status:%d, cmd:%s\n", cmd_task_status, param->cmd);
    if (cmd_task_status) {
        return;
    }
    cmd_task_status = true;
    lv_thread_init(&threadCmd, LV_THREAD_PRIO_MID, cmd_task, 0x2000, param);
}
