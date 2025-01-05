

#include <stdio.h>
#include <string.h>
#include "ui.h"

#define _UI_SCREEN_INDEX -3

static void (*_command_cb)(const char* city_pos, const char* api_key);
static void degree_convert(char* degree, char* min, char* sec);
static void ui_event_Button1(lv_event_t* e);
static void ui_event_TextareaX(lv_event_t* e);
static void ui_event_Key_Ok(lv_event_t* e);


lv_obj_t* ui_ScreenC3_screen_init(void)
{
    lv_obj_t* screenC3 = lv_obj_create(NULL);
    lv_obj_remove_flag(screenC3, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
    lv_obj_set_style_bg_color(screenC3, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
    lv_obj_set_style_bg_opa(screenC3, 255, LV_PART_MAIN| LV_STATE_DEFAULT);


    lv_obj_t* panel1 = lv_obj_create(screenC3);
    lv_obj_set_size(panel1, 492, 66);
    lv_obj_align(panel1, LV_ALIGN_CENTER, -1, -209);
    lv_obj_t* label1 = lv_label_create(panel1);
    lv_label_set_text(label1, "QWeather");
    lv_obj_set_align(label1, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(label1, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* button1 = lv_button_create(screenC3);
    lv_obj_set_size(button1, 85, 41);
    lv_obj_align(button1, LV_ALIGN_CENTER, 179, -208);
    lv_obj_add_flag(button1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);   /// Flags
    lv_obj_t* label2 = lv_label_create(button1);
    lv_label_set_text(label2, "Close");
    lv_obj_set_align(label2, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(label2, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* textarea1s[6];
    const char* placeholders[2][3] = { {"latitude°", "minutes'", "seconds\""}, {"longitude", "minutes", "seconds"} };
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            lv_obj_t* textarea1 = lv_textarea_create(screenC3);
            lv_obj_set_size(textarea1, 116, LV_SIZE_CONTENT);
            lv_obj_align(textarea1, LV_ALIGN_CENTER, -141+j*142, -128 + i*67);
            lv_textarea_set_placeholder_text(textarea1, placeholders[i][j]);
            lv_textarea_set_one_line(textarea1, true);
            lv_textarea_set_accepted_chars(textarea1, "0123456789");
            lv_textarea_set_max_length(textarea1, j == 0 ? 3 : 2);
            lv_obj_set_style_text_font(textarea1, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            textarea1s[i * 3 + j] = textarea1;
        }
    }

    lv_obj_t* textarea2 = lv_textarea_create(screenC3);
    lv_obj_set_size(textarea2, 399, LV_SIZE_CONTENT);
    lv_obj_align(textarea2, LV_ALIGN_CENTER, 1, 6);
    lv_textarea_set_placeholder_text(textarea2,"API KEY");
    lv_textarea_set_one_line(textarea2,true);
    //lv_textarea_set_password_mode(textarea2, true);
    lv_textarea_set_max_length(textarea2, 32);
    lv_textarea_set_accepted_chars(textarea2, "0123456789abcdef");
    lv_obj_set_style_text_font(textarea2, &lv_font_montserrat_20, LV_PART_MAIN| LV_STATE_DEFAULT);

    lv_obj_t* label3 = lv_label_create(screenC3);
    lv_label_set_text(label3, "Ready");
    lv_obj_align(label3, LV_ALIGN_CENTER, 0, 48);
    lv_obj_set_style_text_font(label3, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* keyboard1 = lv_keyboard_create(screenC3);
    lv_obj_set_size(keyboard1, 475, 171);
    lv_obj_align(keyboard1, LV_ALIGN_CENTER, -1, 150);


    lv_obj_add_event_cb(button1, ui_event_Button1, LV_EVENT_CLICKED, NULL);
    for (int k = 0; k < 6; k++) {
        lv_obj_add_event_cb(textarea1s[k], ui_event_TextareaX, LV_EVENT_FOCUSED, keyboard1);
    }
    lv_obj_add_event_cb(textarea2, ui_event_TextareaX, LV_EVENT_FOCUSED, keyboard1);

    lv_keyboard_set_textarea(keyboard1, textarea1s[0]);
    lv_obj_add_event_cb(keyboard1, ui_event_Key_Ok, LV_EVENT_READY, textarea1s[0]);

    return screenC3;
}

void ui_ScreenC3_set_command_cb(void (*command_cb)(const char* city_pos, const char* api_key))
{
    _command_cb = command_cb;
}


void ui_ScreenC3_set_config_with_index(int32_t index, const char* value)
{
    lv_obj_t* screenC3 = ui_screen_get(_UI_SCREEN_INDEX);
    if (index == 0) {
        char tmp[64];
        strncpy(tmp, value, sizeof(tmp));

        char* latitude1 = strchr(tmp, ',');
        char* longitude1 = tmp;
        if (latitude1 != NULL) {
            *(latitude1++) = '\0';

            char latitude2[3], latitude3[3], longitude2[3], longitude3[3];
            degree_convert(latitude1, latitude2, latitude3);
            degree_convert(longitude1, longitude2, longitude3);

            char* values[] = { latitude1, latitude2, latitude3, longitude1, longitude2, longitude3 };
            for (int i = 0; i < 6; i++) {
                lv_obj_t* textarea1 = lv_obj_get_child_by_type(screenC3, i, &lv_textarea_class);
                lv_textarea_set_text(textarea1, values[i]);
            }
        }
    }
    else {
        lv_obj_t* textarea7 = lv_obj_get_child_by_type(screenC3, 6, &lv_textarea_class);
        lv_textarea_set_text(textarea7, value);
    }
}

void ui_ScreenC3_set_result(int32_t op, const char* result)
{
    lv_obj_t* screenC3 = ui_screen_get(_UI_SCREEN_INDEX);
    lv_obj_t* label3 = lv_obj_get_child_by_type(screenC3, 0, &lv_label_class);
    lv_label_set_text(label3, result);

    lv_obj_t* button1 = lv_obj_get_child_by_type(screenC3, 0, &lv_button_class);
    lv_obj_t* keyboard1 = lv_obj_get_child_by_type(screenC3, 0, &lv_keyboard_class);
    lv_obj_remove_flag(button1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(keyboard1, LV_OBJ_FLAG_HIDDEN);
}


static void degree_convert(char* degree, char* min, char* sec) {
    char* dot = strchr(degree, '.');
    if (dot != NULL) {
        (*dot++) = '\0';
        int32_t i, n = 0;
        for (i = 0; i < 4 && dot[i]; i++) {
            n = n * 10 + (dot[i] - '0');
        }
        for (; i < 4; i++) {
            n = n * 10;
        }
        n = n * 60 / 100;
#ifdef _WIN32
        _itoa(n / 100, min, 10);
        _itoa((n % 100) * 60 / 100, sec, 10);
#else
        itoa(n / 100, min, 10);
        itoa((n % 100) * 60 / 100, sec, 10);
#endif
    }
    else {
        min[0] = '\0';
        sec[0] = '\0';
    }
}

static void ui_event_Button1(lv_event_t* e) 
{
    ui_screen_change(0);
}

static void ui_event_TextareaX(lv_event_t* e)
{
    lv_obj_t* keyboard1 = lv_event_get_user_data(e);
    lv_keyboard_set_textarea(keyboard1, lv_event_get_target(e));
}

static void ui_event_Key_Ok(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
    lv_obj_t* textarea1 = lv_event_get_user_data(e);
    lv_obj_t* parent = lv_obj_get_parent(textarea1);
    int32_t i = lv_obj_get_index(textarea1);

    const char* latitude1 = lv_textarea_get_text(textarea1);
    const char* latitude2 = lv_textarea_get_text(lv_obj_get_child(parent, i + 1));
    const char* latitude3 = lv_textarea_get_text(lv_obj_get_child(parent, i + 2));

    const char* longitude1 = lv_textarea_get_text(lv_obj_get_child(parent, i + 3));
    const char* longitude2 = lv_textarea_get_text(lv_obj_get_child(parent, i + 4));
    const char* longitude3 = lv_textarea_get_text(lv_obj_get_child(parent, i + 5));
 
    const char* api_key = lv_textarea_get_text(lv_obj_get_child(parent, i + 6));
    lv_obj_t* label3 = lv_obj_get_child(parent, i+7);
 
    char city_pos[64];
    int32_t latitude1p = (atoi(latitude2) * 10000 / 60) + (atoi(latitude3) * 10000 / 3600);
    int32_t longitude1p = (atoi(longitude2) * 10000 / 60) + (atoi(longitude3) * 10000 / 3600);
    snprintf(city_pos, sizeof(city_pos), "%s.%04ld,%s.%04ld", longitude1, longitude1p, latitude1, latitude1p);

    lv_obj_t* button1 = lv_obj_get_child_by_type(parent, 0, &lv_button_class);
    lv_obj_add_flag(button1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(label3, "query...");
    _command_cb(city_pos, api_key);
 }
