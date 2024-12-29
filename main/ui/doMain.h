#include <stdbool.h>
#include "lvgl.h"
#include "ui.h"

void ui_main(void);

void do_main_ui_init(void);

void scan_wifi_task(void *pvParameters);

void ui_event_Key_Ok(lv_event_t *e);

void ui_event_Button_scan(lv_event_t *e);

void connect_wifi_task(void *pvParameters);


void ui_event_All_on(lv_event_t *e);

void ui_event_All_off(lv_event_t *e);

void ui_event_light1(lv_event_t *e);

void ui_event_light2(lv_event_t *e);

void ui_event_light3(lv_event_t *e);

bool get_light1_status();

bool get_light2_status();

bool get_light3_status();

