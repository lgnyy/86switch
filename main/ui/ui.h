

#ifndef _SWITCH86_UI_H
#define _SWITCH86_UI_H

#ifdef __cplusplus
extern "C" {
#endif

    #include "lvgl.h"

#define UI_NO_FEEDBACK_MODE 1

// SCREEN: ui_Screen2(ui_ScreenC1)
lv_obj_t* ui_Screen2_screen_init(void);
void ui_Screen2_set_command_cb(void (*command_cb)(const char* ssid, const char* pswd));
void ui_Screen2_set_options_text(const char* options, const char* text, bool finish);

#define ui_ScreenC1_screen_init ui_Screen2_screen_init
#define ui_ScreenC1_set_command_cb ui_Screen2_set_command_cb
#define ui_ScreenC1_set_options_text ui_Screen2_set_options_text

// SCREEN: ui_ScreenC2
lv_obj_t* ui_ScreenC2_screen_init(void);
void ui_ScreenC2_set_command_cb(void (*command_cb)(int op, const char* username, const char* passsword));
void ui_ScreenC2_set_config_with_index(int32_t index, const char* value);
void ui_ScreenC2_set_result(const char* result);

// SCREEN: ui_ScreenC3
lv_obj_t* ui_ScreenC3_screen_init(void);
void ui_ScreenC3_set_command_cb(void (*command_cb)(const char* city_pos, const char* api_key));
void ui_ScreenC3_set_config_with_index(int32_t index, const char* value);
void ui_ScreenC3_set_result(const char* result);



// SCREEN: ui_Screen1
lv_obj_t* ui_Screen1_screen_init(void);
void ui_Screen1_set_date_time(const char* date, const char* time);
void ui_Screen1_set_weather_info(int index, const char* value);

// SCREEN: ui_Screen10
lv_obj_t* ui_Screen10_screen_init(void);
void ui_Screen10_set_command_cb(void (*command_cb)(int32_t index, int32_t lightp, int32_t colorp));

// SCREEN: ui_Screen11
lv_obj_t* ui_Screen11_screen_init(void);
void ui_Screen11_set_command_cb(void (*command_cb)(int32_t index, bool on));


// SCREEN: ui_Screen12
lv_obj_t* ui_Screen12_screen_init(void);
void ui_Screen12_set_command_cb(void (*command_cb)(int32_t index, bool on));

// SCREEN: ui_Screen13
lv_obj_t* ui_Screen13_screen_init(void);
void ui_Screen13_set_time(int hour, int min, int sec);


LV_FONT_DECLARE( ui_font_Font1);
LV_FONT_DECLARE( ui_font_Font2);
LV_FONT_DECLARE( ui_font_Font3);
LV_FONT_DECLARE( ui_font_Font4);

void ui_init(void (*load_cb)(int32_t index));
lv_obj_t* ui_screen_get(int32_t index);
void ui_screen_change(int32_t index);
void ui_event_screen_x(lv_event_t* e);
void ui_create_gesture_image(lv_obj_t* parent, int curr_index);



#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
