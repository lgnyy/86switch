#include <stdio.h>
#include <string.h>
#include <time.h>
#include "nvs_cfg.h"
#include "wifi_station.h"
#include "doMIoT.h"
#include "doWeather.h"
#include "doMain.h"
#ifdef _WIN32
#define localtime_r(_Time, _Tm) localtime_s(_Tm, _Time)
#endif

#define COMMAND_ON "打开"
#define COMMAND_OFF "关闭"
#define ROOM_NAME "卧室"
#define LIGHT_ALL_NAME "所有的灯"
#define LIGHT0_NAME "台灯"
#define LIGHT1_NAME "筒灯"
#define LIGHT2_NAME "吸顶灯"
#define LIGHT3_NAME "灯带"
#define SCENE1_NAME "睡眠"
#define SCENE2_NAME "会客"
#define SCENE3_NAME "明亮"
#define SCENE4_NAME "观影"


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


static bool cmd_task_status = false;
static lv_thread_t threadWeatherFirst, threadWeather, threadWifi, threadCmd;

static char* merge_two_strings(const char* s1, const char* s2);
static void ui_load_cb(int32_t index);
static int weather_load_config(void* ctx, nvs_cfg_read_cb_t read_cb, void* arg);
static void wifi_command(const char* ssid, const char* pswd);
static void miot_command(int op, const char* username, const char* passsword);
static void weather_command(const char* city_pos, const char* api_key);
static void lightp_command(int32_t index, int32_t lightp, int32_t colorp);
static void light_command(int32_t index, bool on);
static void scene_command(int32_t index, bool on);
static void updateTime(lv_timer_t* timer);
static int updateWeather(void* arg, int index, const char* value);
static void getWeather(void* pvParameters);
static void sendCommand(const char* cmd);


void ui_main(void)
{
    ui_ScreenC1_set_command_cb(wifi_command);
    ui_ScreenC2_set_command_cb(miot_command);
    ui_ScreenC3_set_command_cb(weather_command);

    ui_Screen10_set_command_cb(lightp_command);
    ui_Screen11_set_command_cb(light_command);
    ui_Screen12_set_command_cb(scene_command);

    ui_init(ui_load_cb);

    int32_t index = (nvs_cfg_check(NVS_CFG_WIFI_INFO_NAMESPACE) == 0) ? 0 : -1;
    lv_screen_load(ui_screen_get(index));

    lv_timer_create(updateTime, 1000, NULL);
    if (nvs_cfg_check(NVS_CFG_WIFI_INFO_NAMESPACE) == 0) {
        lv_thread_init(&threadWeather, LV_THREAD_PRIO_LOW, getWeather, 4096, NULL);
    }
    else {
        lv_thread_init(&threadWifi, LV_THREAD_PRIO_MID, wifi_scan_task, 4096, NULL);
    }
}

static void ui_load_cb(int32_t index)
{
    if (index == -2) {

    }
    else if (index == -3) {
        nvs_cfg_load(NVS_CFG_WEATHER_INFO_NAMESPACE, weather_load_config, NULL);
    }
}
static int weather_load_config(void* ctx, nvs_cfg_read_cb_t read_cb, void* arg) {
    const char** keys = weather_get_config_keys();
    char value[64];
    for (int i = 0; keys[i]; i++) {
        if (read_cb(arg, keys[i], value, sizeof(value)) == 0) {
            ui_ScreenC3_set_config_with_index(i, value);
        }
    }
    return 0;
}

static void lightp_command(int32_t index, int32_t lightp, int32_t colorp)
{
    if (lightp > 0) {
        char cmd[256];
        const char* fmt = lightp_on_command_list[1 + index];
        snprintf(cmd, sizeof(cmd), fmt, lightp, 1700 + 48 * colorp);
        sendCommand(cmd);
    }
    else {
        sendCommand(lightp_off_command_list[1 + index]);
    }
}

static void light_command(int32_t index, bool on)
{
    sendCommand(on? light_on_command_list[index] : light_off_command_list[index]);
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

static void wifi_command(const char* ssid, const char* pswd)
{
    if ((ssid == NULL) || (pswd == NULL)) {
        lv_thread_init(&threadWifi, LV_THREAD_PRIO_MID, wifi_scan_task, 4096, NULL);
    }
    else {
        char* ssid_pswd = merge_two_strings(ssid, pswd);
        if (ssid_pswd == NULL) {
            return;
        }

        lv_thread_init(&threadWifi, LV_THREAD_PRIO_MID, wifi_connect_task, 4096, ssid_pswd);
    }
}

void wifi_scan_task(void *pvParameters)
{
    lv_delay_ms(200);

    char ssids[256] = {0};
    wifi_station_scan(ssids, sizeof(ssids));
  
    lv_lock();
    ui_ScreenC1_set_options_text(ssids, "请连接WiFi", false);
    lv_unlock();
}

void wifi_connect_task(void *pvParameters)
{
    const char* ssid = (const char*)pvParameters;
    const char* pswd = ssid + strlen(ssid) + 1;

    lv_delay_ms(200);
    int ret = wifi_station_connect(ssid, pswd);
   
    lv_lock();
    ui_ScreenC1_set_options_text(NULL, (ret == 0)? "Success" : "Failed", false);
    lv_unlock();

    lv_delay_ms(1000);

    if (ret == 0){
        lv_lock();
        ui_screen_change(0);
        lv_unlock();

        lv_thread_init(&threadWeather, LV_THREAD_PRIO_LOW, getWeather, 4096, NULL);
    }
    else {
        lv_lock();
        ui_ScreenC1_set_options_text(NULL, "请连接WiFi", true);
        lv_unlock();
    }

    free(pvParameters);
}


static void miot_command(int op, const char* username, const char* passsword)
{

}


void weather_query_task(void* pvParameters)
{
    int ret = weather_query_first((char*)pvParameters, updateWeather, NULL);

    lv_lock();
    ui_ScreenC3_set_result((ret == 0) ? "Success" : "Failed");
    lv_unlock();

    free(pvParameters);
}

static void weather_command(const char* city_pos, const char* api_key)
{
    char* pos_key = merge_two_strings(city_pos, api_key);
    if (pos_key == NULL) {
        return;
    }

    lv_thread_init(&threadWeatherFirst, LV_THREAD_PRIO_MID, weather_query_task, 4096, pos_key);
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
    miot_send_cmd((char*)arg);

    lv_lock();
    cmd_task_status = false;
    lv_unlock();
    free(arg);
}
static void sendCommand(const char* cmd) 
{
    printf("status:%d, cmd:%s\n", cmd_task_status, cmd);
    if (cmd_task_status) {
        return;
    }
    cmd_task_status = true;

    char* arg = (char*)malloc(strlen(cmd) + 1);
    if (arg == NULL) {
        return;
    }
    strcpy(arg, cmd);
    lv_thread_init(&threadCmd, LV_THREAD_PRIO_MID, cmd_task, 4096, arg);
}