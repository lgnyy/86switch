

#include "ui.h"

#define _UI_SCREEN_INDEX -1

LV_IMG_DECLARE(ui_img_logo_png);   // assets\logo.png

static lv_obj_t* _ui_Button2;
static void (*_command_cb)(const char* ssid, const char* pswd);
static void ui_event_Button1(lv_event_t* e);
static void ui_event_Button_scan(lv_event_t* e);
static void ui_event_Key_Ok(lv_event_t* e);


lv_obj_t* ui_Screen2_screen_init(void)
{
    lv_obj_t* ui_Screen2 = lv_obj_create(NULL);
    lv_obj_remove_flag( ui_Screen2, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
    lv_obj_set_style_bg_color(ui_Screen2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
    lv_obj_set_style_bg_opa(ui_Screen2, 255, LV_PART_MAIN| LV_STATE_DEFAULT);


    lv_obj_t* ui_Panel1 = lv_obj_create(ui_Screen2);
    lv_obj_set_size(ui_Panel1, 492, 66);
    lv_obj_align(ui_Panel1, LV_ALIGN_CENTER, -1, -209);

    lv_obj_t* ui_Label12 = lv_label_create(ui_Panel1);
    lv_label_set_text(ui_Label12, "请连接WiFi");
    lv_obj_set_align(ui_Label12, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(ui_Label12, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* ui_Button1 = lv_button_create(ui_Screen2);
    lv_obj_set_size(ui_Button1, 85, 41);
    lv_obj_align(ui_Button1, LV_ALIGN_CENTER, 179, -208);
    lv_obj_add_flag(ui_Button1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);   /// Flags

    lv_obj_t* ui_Label13 = lv_label_create(ui_Button1);
    lv_label_set_text(ui_Label13, "跳过");
    lv_obj_set_align(ui_Label13, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(ui_Label13, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);


    lv_obj_t* ui_Dropdown2 = lv_dropdown_create(ui_Screen2);
    lv_dropdown_set_options( ui_Dropdown2, "SSID" );
    lv_obj_set_size(ui_Dropdown2, 292, LV_SIZE_CONTENT);
    lv_obj_align( ui_Dropdown2, LV_ALIGN_CENTER, -52, -118);
    lv_obj_add_flag( ui_Dropdown2, LV_OBJ_FLAG_SCROLL_ON_FOCUS );   /// Flags
    lv_obj_set_style_text_font(ui_Dropdown2, &lv_font_montserrat_20, LV_PART_MAIN| LV_STATE_DEFAULT);

    lv_obj_t* ui_Button2 = lv_button_create(ui_Screen2);
    lv_obj_set_size(ui_Button2, 85, 41);
    lv_obj_align( ui_Button2, LV_ALIGN_CENTER, 151, -118);
    lv_obj_add_flag( ui_Button2, LV_OBJ_FLAG_SCROLL_ON_FOCUS );   /// Flags

    lv_obj_t* ui_Label14 = lv_label_create(ui_Button2);
    lv_label_set_text(ui_Label14,"刷新");
    lv_obj_set_align(ui_Label14, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(ui_Label14, &ui_font_Font4, LV_PART_MAIN| LV_STATE_DEFAULT);


    lv_obj_t* ui_TextArea1 = lv_textarea_create(ui_Screen2);
    lv_obj_set_size(ui_TextArea1, 399, LV_SIZE_CONTENT);
    lv_obj_align( ui_TextArea1, LV_ALIGN_CENTER, 1, -51);
    lv_textarea_set_placeholder_text(ui_TextArea1,"Password");
    lv_textarea_set_one_line(ui_TextArea1,true);
    lv_textarea_set_password_mode(ui_TextArea1, true);
    lv_obj_set_style_text_font(ui_TextArea1, &lv_font_montserrat_20, LV_PART_MAIN| LV_STATE_DEFAULT);


    lv_obj_t* ui_Image63 = lv_image_create(ui_Screen2);
    lv_image_set_src(ui_Image63, &ui_img_logo_png);
    lv_obj_align( ui_Image63, LV_ALIGN_CENTER, -4, 19);

    lv_obj_t* ui_Keyboard4 = lv_keyboard_create(ui_Screen2);
    lv_obj_set_size(ui_Keyboard4, 475, 171);
    lv_obj_align(ui_Keyboard4, LV_ALIGN_CENTER, -1, 150);


    lv_obj_add_event_cb(ui_Button1, ui_event_Button1, LV_EVENT_CLICKED, NULL);

    lv_keyboard_set_textarea(ui_Keyboard4, ui_TextArea1);
    lv_obj_add_event_cb(ui_Button2, ui_event_Button_scan, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Keyboard4, ui_event_Key_Ok, LV_EVENT_READY, ui_Label12);

    _ui_Button2 = ui_Button2;
    return ui_Screen2;
}

void ui_Screen2_set_command_cb(void (*command_cb)(const char* ssid, const char* pswd))
{
    _command_cb = command_cb;
}

void ui_Screen2_set_options_text(const char* options, const char* text, bool finish)
{
    lv_obj_t* ui_Screen2 = ui_screen_get(_UI_SCREEN_INDEX);
    if (options != NULL) {
        lv_obj_t* ui_Dropdown2 = lv_obj_get_child(ui_Screen2, 2);
        lv_dropdown_set_options(ui_Dropdown2, options);
    }
    if (text != NULL) {
        lv_obj_t* ui_Label12 = lv_obj_get_child(lv_obj_get_child(ui_Screen2, 0), 0);
        lv_label_set_text(ui_Label12, text);
        if (finish) {
            lv_obj_remove_flag(_ui_Button2, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ui_event_Button1(lv_event_t* e) {
    ui_screen_change(0);
}

static void ui_event_Button_scan(lv_event_t* e)
{
    _command_cb(NULL, NULL);
}

static void ui_event_Key_Ok(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
    lv_obj_t* ui_TextArea1 = lv_keyboard_get_textarea(target);
    if (!lv_obj_has_flag(_ui_Button2, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_t* ui_Screen2 = ui_screen_get(_UI_SCREEN_INDEX);
        lv_obj_t* ui_Label12 = lv_event_get_user_data(e);
        lv_obj_t* ui_Dropdown2 = lv_obj_get_child(ui_Screen2, 2);
        lv_obj_add_flag(_ui_Button2, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_Label12, "Connect...");
        lv_obj_remove_state(ui_TextArea1, LV_STATE_FOCUSED);
        
        char ssid[32] = { 0 };
        lv_dropdown_get_selected_str(ui_Dropdown2, ssid, sizeof(ssid));
        const char* pswd = lv_textarea_get_text(ui_TextArea1);

        _command_cb(ssid, pswd);
    }
}
