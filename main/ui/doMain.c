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

static bool cmd_task_status = false;
static lv_thread_t threadWeather, threadWifi, threadCmd;

static void wifi_command(const char* ssid, const char* pswd);
static void lightp_command(int32_t index, int32_t lightp, int32_t colorp);
static void light_command(int32_t index, bool on);
static void updateTime(lv_timer_t* timer);
static void getWeather(void* pvParameters);
static void sendCommand(const char* cmd);


void ui_main(void)
{
    ui_init();

    int32_t index = (nvs_cfg_check(NVS_CFG_WIFI_INFO_NAMESPACE) == 0) ? 0 : -1;
    lv_screen_load(ui_screen_get(index));

    do_main_ui_init();
}

void do_main_ui_init(void)
{
    ui_Screen2_set_command_cb(wifi_command);

    ui_Screen10_set_command_cb(lightp_command);

    ui_Screen11_set_command_cb(light_command);

    lv_timer_create(updateTime, 1000, NULL);
    if (nvs_cfg_check(NVS_CFG_WIFI_INFO_NAMESPACE) == 0) {
        lv_thread_init(&threadWeather, LV_THREAD_PRIO_LOW, getWeather, 4096, NULL);
   }
    else {
        lv_thread_init(&threadWifi, LV_THREAD_PRIO_MID, scan_wifi_task, 4096, NULL);
    }
}

static void lightp_command(int32_t index, int32_t lightp, int32_t colorp)
{
    if (lightp > 0) {
        char cmd[256];
        const char* fmt = COMMAND_ON ROOM_NAME LIGHT0_NAME ",把亮度调到%ld，把色温调到%ld。";
        snprintf(cmd, sizeof(cmd), fmt, lightp, 1700 + 48 * colorp);
        sendCommand(cmd);
    }
    else {
        sendCommand(COMMAND_OFF ROOM_NAME LIGHT0_NAME);
    }
}

static void light_command(int32_t index, bool on)
{
    sendCommand(on? light_on_command_list[index] : light_off_command_list[index]);
}


static void wifi_command(const char* ssid, const char* pswd)
{
    if ((ssid == NULL) || (pswd == NULL)) {
        lv_thread_init(&threadWifi, LV_THREAD_PRIO_MID, scan_wifi_task, 4096, NULL);
    }
    else {
        size_t slen = strlen(ssid) + 1;
        size_t plen = strlen(pswd) + 1;
        char* ssid_pswd = (char*)malloc(slen + plen);
        if (ssid_pswd == NULL) {
            return;
        }
        memcpy(ssid_pswd, ssid, slen);
        memcpy(ssid_pswd + slen, pswd, plen);
       
        lv_thread_init(&threadWifi, LV_THREAD_PRIO_MID, connect_wifi_task, 4096, ssid_pswd);
    }
}


void scan_wifi_task(void *pvParameters)
{
    lv_delay_ms(200);

    char ssids[256] = {0};
    wifi_station_scan(ssids, sizeof(ssids));
  
    lv_lock();
    ui_Screen2_set_options_text(ssids, "请连接WiFi", false);
    lv_unlock();
}

void connect_wifi_task(void *pvParameters)
{
    const char* ssid = (const char*)pvParameters;
    const char* pswd = ssid + strlen(ssid) + 1;

    lv_delay_ms(200);
    int ret = wifi_station_connect(ssid, pswd);
   
    lv_lock();
    ui_Screen2_set_options_text(NULL, (ret == 0)? "Success" : "Failed", false);
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
        ui_Screen2_set_options_text(NULL, "请连接WiFi", true);
        lv_unlock();
    }

    free(pvParameters);
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