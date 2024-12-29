
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "nvs_cfg.h"
#include "wifi_station.h"
#include "doMIoT.h"
#include "doWeather.h"
#include "doMain.h"


#define COMMAND_ON "打开"
#define COMMAND_OFF "关闭"
#define ROOM_NAME "卧室"
#define LIGHT_ALL_NAME "所有的灯"
#define LIGHT0_NAME "台灯"
#define LIGHT1_NAME "筒灯"
#define LIGHT2_NAME "吸顶灯"
#define LIGHT3_NAME "灯带"

static bool light1_status = false;
static bool light2_status = false;
static bool light3_status = false;
static bool cmd_task_status = false;

static lv_thread_t threadWeather, threadWifi, threadCmd;

static void updateTime(lv_timer_t* timer);
static void getWeather(void* pvParameters);
static void sendCommand(const char* cmd);


void ui_main(void)
{
    ui_init();

    lv_screen_load((nvs_cfg_check(NVS_CFG_WIFI_INFO_NAMESPACE) == 0)? ui_Screen1 : ui_Screen2);

    do_main_ui_init();
}

void do_main_ui_init(void)
{
    lv_keyboard_set_textarea(ui_Keyboard4, ui_TextArea1);
    lv_obj_add_event_cb(ui_Button2, ui_event_Button_scan, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Keyboard4, ui_event_Key_Ok, LV_EVENT_READY, NULL);

    lv_obj_add_event_cb(ui_Image26, ui_event_All_on, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Image32, ui_event_All_off, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_Image30, ui_event_light1, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Image35, ui_event_light2, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Image38, ui_event_light3, LV_EVENT_CLICKED, NULL);

    lv_timer_create(updateTime, 1000, NULL);
    if (nvs_cfg_check(NVS_CFG_WIFI_INFO_NAMESPACE) == 0) {
        lv_thread_init(&threadWeather, LV_THREAD_PRIO_LOW, getWeather, 4096, NULL);
   }
    else {
        lv_thread_init(&threadWifi, LV_THREAD_PRIO_MID, scan_wifi_task, 4096, NULL);
    }
}


void light1_on()
{
    sendCommand(COMMAND_ON ROOM_NAME LIGHT1_NAME); 
    lv_img_set_src(ui_Image35, &ui_img_s3_switch1_on_png);//30
    lv_img_set_src(ui_Image34, &ui_img_s3_light1_on_png);//27
    light1_status = true;
}

void light1_off()
{
    sendCommand(COMMAND_OFF ROOM_NAME LIGHT1_NAME);
    lv_img_set_src(ui_Image35, &ui_img_s3_switch1_off_png);//30
    lv_img_set_src(ui_Image34, &ui_img_s3_light1_off_png);//27
    light1_status = false;
}

bool get_light1_status()
{
    return light1_status;
}

void light2_on()
{
    sendCommand(COMMAND_ON ROOM_NAME LIGHT2_NAME);
    lv_img_set_src(ui_Image30, &ui_img_s3_switch1_on_png);//35
    lv_img_set_src(ui_Image27, &ui_img_s3_light1_on_png);//34
    light2_status = true;
}

void light2_off()
{
    sendCommand(COMMAND_OFF ROOM_NAME LIGHT2_NAME);
    lv_img_set_src(ui_Image30, &ui_img_s3_switch1_off_png);//35
    lv_img_set_src(ui_Image27, &ui_img_s3_light1_off_png);//34
    light2_status = false;
}

bool get_light2_status()
{
    return light2_status;
}

void light3_on()
{
    sendCommand(COMMAND_ON ROOM_NAME LIGHT3_NAME);
    lv_img_set_src(ui_Image38, &ui_img_s3_switch1_on_png);
    lv_img_set_src(ui_Image37, &ui_img_s3_light1_on_png);
    light3_status = true;
}

void light3_off()
{
    sendCommand(COMMAND_OFF ROOM_NAME LIGHT3_NAME);
    lv_img_set_src(ui_Image38, &ui_img_s3_switch1_off_png);
    lv_img_set_src(ui_Image37, &ui_img_s3_light1_off_png);
    light3_status = false;
}

bool get_light3_status()
{
    return light3_status;
}

void ui_event_All_on(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        sendCommand(COMMAND_ON ROOM_NAME LIGHT_ALL_NAME);
    }
}

void ui_event_All_off(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        sendCommand(COMMAND_OFF ROOM_NAME LIGHT_ALL_NAME);
    }
}

void ui_event_light1(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        if (get_light2_status())
        {
            light2_off();
        }
        else
        {
            light2_on();
        }
    }
}

void ui_event_light2(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        if (get_light1_status())
        {
            light1_off();
        }
        else
        {
            light1_on();
        }
    }
}

void ui_event_light3(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        if (get_light3_status())
        {
            light3_off();
        }
        else
        {
            light3_on();
        }
    }
}

void ui_event_Image16(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        _ui_flag_modify(ui_Image16, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
        _ui_flag_modify(ui_Image17, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);

        int32_t v1 = lv_slider_get_value(ui_Slider1);
        int32_t v2 = lv_slider_get_value(ui_Slider2);

        char cmd[256];
        snprintf(cmd, sizeof(cmd), "%s,把亮度调到%ld，把色温调到%ld。", COMMAND_ON ROOM_NAME LIGHT0_NAME, v1, 1700+48*v2);
        sendCommand(cmd);
    }
}
void ui_event_Image17(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        _ui_flag_modify(ui_Image17, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
        _ui_flag_modify(ui_Image16, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);

        sendCommand(COMMAND_OFF ROOM_NAME LIGHT0_NAME);
    }
}

void ui_event_Button_scan(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (event_code == LV_EVENT_CLICKED)
    {
        lv_thread_init(&threadWifi, LV_THREAD_PRIO_MID, scan_wifi_task, 4096, NULL);
    }
}


void scan_wifi_task(void *pvParameters)
{
    lv_delay_ms(200);

    char ssids[256] = {0};
    wifi_station_scan(ssids, sizeof(ssids));
  
    lv_lock();
    lv_dropdown_set_options(ui_Dropdown2, ssids);
    lv_label_set_text(ui_Label12, "请连接WiFi");
    lv_unlock();
}

void connect_wifi_task(void *pvParameters)
{
    const char* ssid = (const char*)pvParameters;
    const char* pswd = ssid + strlen(ssid) + 1;

    lv_delay_ms(200);
    int ret = wifi_station_connect(ssid, pswd);
   
    lv_lock();
    lv_label_set_text(ui_Label12, (ret == 0)? "Success" : "Failed");
    lv_unlock();

    lv_delay_ms(1000);

    if (ret == 0){
        lv_lock();
        _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen1_screen_init);
        lv_unlock();

        lv_thread_init(&threadWeather, LV_THREAD_PRIO_LOW, getWeather, 4096, NULL);
    }
    else {
        lv_lock();
        lv_label_set_text(ui_Label12, "请连接WiFi");
        lv_obj_clear_flag(ui_Button2, LV_OBJ_FLAG_HIDDEN);
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
 
    lv_label_set_text(ui_Label1, topTimeText);
    lv_label_set_text(ui_Label3, topDateText);

    // mod pointer
    lv_img_set_angle(ui_Image55, timeinfo.tm_sec * 60 - 770);
    lv_img_set_angle(ui_Image54, timeinfo.tm_min * 60 - 610);
    lv_img_set_angle(ui_Image53, timeinfo.tm_hour * 5 * 60 + 700);
}

void ui_event_Key_Ok(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if ((event_code == LV_EVENT_READY) && !lv_obj_has_flag(ui_Button2, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_add_flag(ui_Button2, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_Label12, "Connect...");
        lv_obj_clear_state(ui_TextArea1, LV_STATE_FOCUSED);

        const char* pswd = lv_textarea_get_text(ui_TextArea1);
        char* ssid_pswd = (char*)malloc(32 + 32);
        if (ssid_pswd != NULL)
        {
            lv_dropdown_get_selected_str(ui_Dropdown2, ssid_pswd, 32);
            strcpy(ssid_pswd + strlen(ssid_pswd) + 1, pswd);

            lv_thread_init(&threadWifi, LV_THREAD_PRIO_MID, connect_wifi_task, 4096, ssid_pswd);
        }
    }
}

static int updateWeather(void* arg, int index, const char* value) 
{
    printf("updateWeather index:%d, value:%s\n", index, value);
    lv_obj_t* objs[] = { ui_Label2, ui_Label4 , ui_Label5 , ui_Label6 };
    lv_lock();
    lv_label_set_text(objs[index], value);
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