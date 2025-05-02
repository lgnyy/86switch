

#include "ui.h"

#define _UI_SCREEN_INDEX -4

static void (*_command_cb)(const char* cmd, const char* param);
static void ui_event_button_close(lv_event_t* e);
static void ui_event_button_reboot(lv_event_t* e);


lv_obj_t* ui_ScreenC4_screen_init(void)
{
    lv_obj_t* screenC4 = lv_obj_create(NULL);
    lv_obj_remove_flag(screenC4, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
    lv_obj_set_style_bg_color(screenC4, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
    lv_obj_set_style_bg_opa(screenC4, 255, LV_PART_MAIN| LV_STATE_DEFAULT);


    lv_obj_t* panel1 = lv_obj_create(screenC4);
    lv_obj_set_size(panel1, 492, 66);
    lv_obj_align(panel1, LV_ALIGN_CENTER, -1, -209);
    lv_obj_t* label1 = lv_label_create(panel1);
    lv_label_set_text(label1, "Settings");
    lv_obj_set_align(label1, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(label1, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* button1 = lv_button_create(screenC4);
    lv_obj_set_size(button1, 85, 41);
    lv_obj_align(button1, LV_ALIGN_CENTER, 179, -208);
    lv_obj_add_flag(button1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);   /// Flags
    lv_obj_t* label2 = lv_label_create(button1);
    lv_label_set_text(label2, "Close");
    lv_obj_set_align(label2, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(label2, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* button2 = lv_button_create(screenC4);
    lv_obj_set_size(button2, 85, 41);
    lv_obj_align(button2, LV_ALIGN_CENTER, -179, -208);
    lv_obj_add_flag(button2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);   /// Flags
    lv_obj_t* label2_1 = lv_label_create(button2);
    lv_label_set_text(label2_1, "Reboot");
    lv_obj_set_align(label2_1, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(label2_1, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);


    lv_obj_t* label3 = lv_label_create(screenC4);
    lv_label_set_text(label3, "Ready");
    lv_obj_align(label3, LV_ALIGN_CENTER, 0, 200);
    lv_obj_set_style_text_font(label3, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(button1, ui_event_button_close, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(button2, ui_event_button_reboot, LV_EVENT_CLICKED, NULL);

     return screenC4;
}

void ui_ScreenC4_set_command_cb(void (*command_cb)(const char* cmd, const char* param))
{
    _command_cb = command_cb;
}

void ui_ScreenC4_set_config_with_index(int32_t index, const char* value)
{
    lv_obj_t* screenC4 = ui_screen_get(_UI_SCREEN_INDEX);
    lv_obj_t* label3 = lv_obj_get_child_by_type(screenC4, 0, &lv_label_class);
 
    lv_label_set_text(label3, value);
}

void ui_ScreenC4_set_result(int32_t op, const char* result)
{
    lv_obj_t* screenC4 = ui_screen_get(_UI_SCREEN_INDEX);
    if (op == 1) { // show qr
        lv_obj_t* qr = lv_qrcode_create(screenC4);
        lv_qrcode_set_size(qr, 336);
        lv_qrcode_update(qr, result, lv_strlen(result));
        lv_obj_center(qr);
    }
}

static void ui_event_button_close(lv_event_t* e) 
{
    _command_cb("stop", NULL);
    ui_screen_del(_UI_SCREEN_INDEX);
    ui_screen_change(0);
}

static void ui_event_button_reboot(lv_event_t* e)
{
    _command_cb("reboot", NULL);
}
