

#include "ui.h"

#if !(CONFIG_SWITCH86_XMIOT_ENABLE)
#define _UI_SCREEN_INDEX -2

LV_IMG_DECLARE(ui_img_logo_png);   // assets\logo.png

static void (*_command_cb)(int op, const char* username, const char* passsword);
static void ui_modify_button_flag(lv_obj_t* parent, bool hidden);
static void ui_event_textarea_focused(lv_event_t* e);
static void ui_event_button_close(lv_event_t* e);
static void ui_event_button_login(lv_event_t* e);
static void ui_event_button_query(lv_event_t* e);


lv_obj_t* ui_ScreenC2_screen_init(void)
{
    lv_obj_t* screenC2 = lv_obj_create(NULL);
    lv_obj_remove_flag(screenC2, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
    lv_obj_set_style_bg_color(screenC2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
    lv_obj_set_style_bg_opa(screenC2, 255, LV_PART_MAIN| LV_STATE_DEFAULT);


    lv_obj_t* panel1 = lv_obj_create(screenC2);
    lv_obj_set_size(panel1, 492, 66);
    lv_obj_align(panel1, LV_ALIGN_CENTER, -1, -209);
    lv_obj_t* label1 = lv_label_create(panel1);
    lv_label_set_text(label1, "MIoT");
    lv_obj_set_align(label1, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(label1, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* button1 = lv_button_create(screenC2);
    lv_obj_set_size(button1, 85, 41);
    lv_obj_align(button1, LV_ALIGN_CENTER, 179, -208);
    lv_obj_add_flag(button1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);   /// Flags
    lv_obj_t* label2 = lv_label_create(button1);
    lv_label_set_text(label2, "Close");
    lv_obj_set_align(label2, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(label2, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);


    lv_obj_t* textarea1 = lv_textarea_create(screenC2);
    lv_obj_set_size(textarea1, 292, LV_SIZE_CONTENT);
    lv_obj_align(textarea1, LV_ALIGN_CENTER, -52, -90);
    lv_textarea_set_placeholder_text(textarea1, "expires_ts");
    lv_textarea_set_one_line(textarea1, true);
    lv_obj_set_style_text_font(textarea1, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* button2 = lv_button_create(screenC2);
    lv_obj_set_size(button2, 85, 41);
    lv_obj_align(button2, LV_ALIGN_CENTER, 151, -90);
    lv_obj_add_flag(button2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);   /// Flags
    lv_obj_t* label3 = lv_label_create(button2);
    lv_label_set_text(label3, "Token");
    lv_obj_set_align(label3, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(label3, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* textarea3 = lv_textarea_create(screenC2);
    lv_obj_set_size(textarea3, 292, LV_SIZE_CONTENT);
    lv_obj_align(textarea3, LV_ALIGN_CENTER, -52, 6);
    lv_textarea_set_placeholder_text(textarea3, "Wifi Speaker DID");
    lv_textarea_set_one_line(textarea3, true);
    lv_obj_set_style_text_font(textarea3, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* button3 = lv_button_create(screenC2);
    lv_obj_set_size(button3, 85, 41);
    lv_obj_align(button3, LV_ALIGN_CENTER, 151, 6);
    lv_obj_add_flag(button3, LV_OBJ_FLAG_SCROLL_ON_FOCUS);   /// Flags
    lv_obj_t* label4 = lv_label_create(button3);
    lv_label_set_text(label4, "Modify"); // Query
    lv_obj_set_align(label4, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(label4, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);


    lv_obj_t* label5 = lv_label_create(screenC2);
    lv_label_set_text(label5, "Ready");
    lv_obj_align(label5, LV_ALIGN_CENTER, 0, 48);
    lv_obj_set_style_text_font(label5, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* keyboard1 = lv_keyboard_create(screenC2);
    lv_obj_set_size(keyboard1, 475, 171);
    lv_obj_align(keyboard1, LV_ALIGN_CENTER, -1, 150);


    lv_keyboard_set_textarea(keyboard1, textarea1);
    lv_obj_add_event_cb(textarea1, ui_event_textarea_focused, LV_EVENT_FOCUSED, keyboard1);
    lv_obj_add_event_cb(textarea3, ui_event_textarea_focused, LV_EVENT_FOCUSED, keyboard1);
    lv_obj_add_event_cb(button1, ui_event_button_close, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(button2, ui_event_button_login, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(button3, ui_event_button_query, LV_EVENT_CLICKED, NULL);

     return screenC2;
}

void ui_ScreenC2_set_command_cb(void (*command_cb)(int op, const char* username, const char* passsword))
{
    _command_cb = command_cb;
}

void ui_ScreenC2_set_config_with_index(int32_t index, const char* value)
{
    int32_t obj_index = index;
    lv_obj_t* screenC2 = ui_screen_get(_UI_SCREEN_INDEX);
    lv_obj_t* textarea1 = lv_obj_get_child_by_type(screenC2, obj_index, &lv_textarea_class);
 
    lv_textarea_set_text(textarea1, value);
}

void ui_ScreenC2_set_result(int32_t op, const char* result)
{
    lv_obj_t* screenC2 = ui_screen_get(_UI_SCREEN_INDEX);
    if (op == 1) { // show qr
        lv_obj_t* qr = lv_qrcode_create(screenC2);
        lv_qrcode_set_size(qr, 360);
        lv_qrcode_update(qr, result, lv_strlen(result));
        lv_obj_center(qr);
    }
    else { // set status
        lv_obj_t* label3 = lv_obj_get_child_by_type(screenC2, 0, &lv_label_class);
        lv_label_set_text(label3, result);

        lv_obj_t* qr = lv_obj_get_child_by_type(screenC2, 0, &lv_qrcode_class);
        if (qr != NULL) {
            lv_obj_del(qr);
        }
        ui_modify_button_flag(screenC2, false);
    }
}


static void ui_modify_button_flag(lv_obj_t* parent, bool hidden)
{
    uint32_t n = lv_obj_get_child_count_by_type(parent, &lv_button_class);
    for (uint32_t i = 1; i < n; i++) {
        lv_obj_t* button1 = lv_obj_get_child_by_type(parent, i, &lv_button_class);
        if (hidden) {
            lv_obj_add_flag(button1, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_remove_flag(button1, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ui_event_textarea_focused(lv_event_t* e)
{
    lv_obj_t* keyboard1 = lv_event_get_user_data(e);
    lv_keyboard_set_textarea(keyboard1, lv_event_get_target(e));
}

static void ui_event_button_close(lv_event_t* e) 
{
    _command_cb(0x81, NULL, NULL);
    ui_screen_del(_UI_SCREEN_INDEX);
    ui_screen_change(0);
}

static void ui_event_button_login(lv_event_t* e)
{
    lv_obj_t* screenC2 = lv_obj_get_parent(lv_event_get_target(e));
    lv_obj_t* label3 = lv_obj_get_child_by_type(screenC2, 0, &lv_label_class);

    lv_label_set_text(label3, "Get Token...");
    ui_modify_button_flag(screenC2, true);
    _command_cb(1, NULL, NULL);
}

static void ui_event_button_query(lv_event_t* e)
{
    lv_obj_t* screenC2 = lv_obj_get_parent(lv_event_get_target(e));
    lv_obj_t* textarea2 = lv_obj_get_child_by_type(screenC2, 1, &lv_textarea_class);
    //lv_obj_t* label3 = lv_obj_get_child_by_type(screenC2, 0, &lv_label_class);

    const char* speaker_did = lv_textarea_get_text(textarea2);

    //lv_label_set_text(label3, "Query...");
    //ui_modify_button_flag(screenC2, true);
    _command_cb(2, speaker_did, NULL);
}
#endif // #if !(CONFIG_SWITCH86_XMIOT_ENABLE)
