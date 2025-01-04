

#include "ui.h"

#define _UI_SCREEN_INDEX 3

LV_IMG_DECLARE(ui_img_s2_back2_png);   // assets\s4\back4.png
#define ui_img_s4_back4_png ui_img_s2_back2_png
LV_IMG_DECLARE(ui_img_s4_mode2_png);   // assets\s4\mode2.png
LV_IMG_DECLARE(ui_img_s4_mode1_png);   // assets\s4\mode1.png
LV_IMG_DECLARE(ui_img_s3_switch1_on_png);   // assets\s3\switch1_on.png
#define ui_img_s4_switch1_on_png ui_img_s3_switch1_on_png

static void (*_command_cb)(int32_t index, bool on);
static void ui_event_scene_ud(lv_event_t* e);


lv_obj_t* ui_Screen12_screen_init(void)
{
    lv_obj_t* ui_Screen12 = lv_obj_create(NULL);
    lv_obj_remove_flag( ui_Screen12, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
    lv_obj_set_style_bg_color(ui_Screen12, lv_color_hex(0x181C20), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* ui_Image39 = lv_image_create(ui_Screen12);
    lv_image_set_src(ui_Image39, &ui_img_s4_back4_png);
    lv_obj_set_align(ui_Image39, LV_ALIGN_BOTTOM_MID);
    //lv_obj_add_flag( ui_Image39, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags

    lv_obj_t* ui_Image40 = lv_image_create(ui_Screen12);
    lv_image_set_src(ui_Image40, &ui_img_s4_mode2_png);
    lv_obj_align(ui_Image40, LV_ALIGN_CENTER, 0, -130);

    lv_obj_t* ui_Image41 = lv_image_create(ui_Screen12);
    lv_image_set_src(ui_Image41, &ui_img_s4_mode1_png);
    lv_obj_align( ui_Image41, LV_ALIGN_CENTER, 0, 90);


    for (int i = 0; i < 4; i++) {
        lv_obj_t* ui_Image64 = lv_image_create(ui_Screen12);
        lv_image_set_src(ui_Image64, &ui_img_s4_switch1_on_png);
        lv_obj_align(ui_Image64, LV_ALIGN_CENTER, -172 + (115 * i), 128);
        lv_obj_add_flag(ui_Image64, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);   /// Flags
        lv_image_set_scale(ui_Image64, 220);

        lv_obj_t* ui_Button3 = lv_button_create(ui_Screen12);
        lv_obj_set_size(ui_Button3, 62, 50);
        lv_obj_align(ui_Button3, LV_ALIGN_CENTER, -172 + (115 * i), 130);
        lv_obj_add_flag(ui_Button3, LV_OBJ_FLAG_SCROLL_ON_FOCUS);   /// Flags
        lv_obj_set_style_bg_color(ui_Button3, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_Button3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_set_user_data(ui_Image64, (void*)(i + 1));
        lv_obj_add_event_cb(ui_Image64, ui_event_scene_ud, LV_EVENT_CLICKED, ui_Button3);
        lv_obj_set_user_data(ui_Button3, (void*)(i + 1));
        lv_obj_add_event_cb(ui_Button3, ui_event_scene_ud, LV_EVENT_CLICKED, ui_Image64);
    }


    ui_create_gesture_image(ui_Screen12, _UI_SCREEN_INDEX);

    lv_obj_add_event_cb(ui_Screen12, ui_event_screen_x, LV_EVENT_GESTURE, (void*)_UI_SCREEN_INDEX);
    return ui_Screen12;
}

void ui_Screen12_set_command_cb(void (*command_cb)(int32_t index, bool on))
{
    _command_cb = command_cb;
}

static void ui_event_scene_ud(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
    lv_obj_t* other = lv_event_get_user_data(e);
    int scene = (int)lv_obj_get_user_data(target);
    _command_cb(scene, lv_obj_check_type(target, &lv_button_class));
    lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(other, LV_OBJ_FLAG_HIDDEN);
}
