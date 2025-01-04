

#include "ui.h"

#define _UI_SCREEN_INDEX 2
#define LIGHT1_NAME "筒灯"
#define LIGHT2_NAME "吸顶灯"
#define LIGHT3_NAME "灯带"

LV_IMG_DECLARE(ui_img_s2_back2_png);   // assets\s3\back3.png
#define ui_img_s3_back3_png ui_img_s2_back2_png
LV_IMG_DECLARE(ui_img_s3_back3_1_png);   // assets\s3\back3_1.png
LV_IMG_DECLARE(ui_img_s3_b_light_on_png);   // assets\s3\b_light_on.png
LV_IMG_DECLARE(ui_img_s3_b_switch_on_png);   // assets\s3\b_switch_on.png
LV_IMG_DECLARE(ui_img_s3_b_light_off_png);   // assets\s3\b_light_off.png
LV_IMG_DECLARE(ui_img_s3_b_switch_off_png);   // assets\s3\b_switch_off.png
LV_IMG_DECLARE(ui_img_s3_back3_2_png);   // assets\s3\back3_2.png
LV_IMG_DECLARE(ui_img_s3_light1_off_png);   // assets\s3\light1_off.png
LV_IMG_DECLARE(ui_img_s3_switch1_off_png);   // assets\s3\switch1_off.png
LV_IMG_DECLARE(ui_img_s3_switch1_on_png);   // assets\s3\switch1_on.png
LV_IMG_DECLARE(ui_img_s3_light1_on_png);   // assets\s3\light1_on.png
//LV_IMG_DECLARE(ui_img_s3_switch_3_on_png);   // assets\s3\switch_3_on.png

static void (*_command_cb)(int32_t index, bool on);
static void ui_event_All_eud(lv_event_t* e);
static void ui_event_light_ud(lv_event_t* e);


lv_obj_t* ui_Screen11_screen_init(void)
{
    lv_obj_t* ui_Screen11 = lv_obj_create(NULL);
    lv_obj_remove_flag( ui_Screen11, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
    lv_obj_set_style_bg_color(ui_Screen11, lv_color_hex(0x181C20), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* ui_Image23 = lv_image_create(ui_Screen11);
    lv_image_set_src(ui_Image23, &ui_img_s3_back3_png);
    lv_obj_set_align(ui_Image23, LV_ALIGN_BOTTOM_MID);
    //lv_obj_add_flag( ui_Image23, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags

    lv_obj_t* ui_Image24 = lv_image_create(ui_Screen11);
    lv_image_set_src(ui_Image24, &ui_img_s3_back3_1_png);
    lv_obj_align(ui_Image24, LV_ALIGN_CENTER, -110, -135);

    lv_obj_t* ui_Image29 = lv_image_create(ui_Image24);
    lv_image_set_src(ui_Image29, &ui_img_s3_b_light_on_png);
    lv_obj_align(ui_Image29, LV_ALIGN_CENTER, -42, -30);

    lv_obj_t* ui_Image26 = lv_image_create(ui_Image24);
    lv_image_set_src(ui_Image26, &ui_img_s3_b_switch_on_png);
    lv_obj_align(ui_Image26, LV_ALIGN_CENTER, 43, -30);
    lv_obj_add_flag(ui_Image26, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);   /// Flags

    lv_obj_t* ui_Label7 = lv_label_create(ui_Image24);
    lv_obj_align( ui_Label7, LV_ALIGN_CENTER, -41, 33);
    lv_obj_set_style_text_color(ui_Label7, lv_color_hex(0x9F9F9F), LV_PART_MAIN | LV_STATE_DEFAULT );
    lv_obj_set_style_text_opa(ui_Label7, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Label7, &ui_font_Font4, LV_PART_MAIN| LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label7, "全开");


    lv_obj_t* ui_Image25 = lv_image_create(ui_Screen11);
    lv_image_set_src(ui_Image25, &ui_img_s3_back3_1_png);
    lv_obj_align(ui_Image25, LV_ALIGN_CENTER, 111, -135);

    lv_obj_t* ui_Image31 = lv_image_create(ui_Image25);
    lv_image_set_src(ui_Image31, &ui_img_s3_b_light_off_png);
    lv_obj_align( ui_Image31, LV_ALIGN_CENTER, -42, -30);

    lv_obj_t* ui_Image32 = lv_image_create(ui_Image25);
    lv_image_set_src(ui_Image32, &ui_img_s3_b_switch_off_png);
    lv_obj_align(ui_Image32, LV_ALIGN_CENTER, 44, -30);
    lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);   /// Flags

    lv_obj_t* ui_Label9 = lv_label_create(ui_Image25);
    lv_obj_align(ui_Label9, LV_ALIGN_CENTER, -40, 33);
    lv_obj_set_style_text_color(ui_Label9, lv_color_hex(0x9F9F9F), LV_PART_MAIN | LV_STATE_DEFAULT );
    lv_obj_set_style_text_opa(ui_Label9, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Label9, &ui_font_Font4, LV_PART_MAIN| LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label9, "全关");

    //const char* light_name_list[] = { "客厅灯", "卧室灯", "廊灯" };
    const char* light_name_list[] = { LIGHT1_NAME, LIGHT2_NAME, LIGHT3_NAME };
    for (int i = 0; i < 3; i++) {
        lv_obj_t* ui_Image28 = lv_image_create(ui_Screen11);
        lv_image_set_src(ui_Image28, &ui_img_s3_back3_2_png);
        lv_obj_align(ui_Image28, LV_ALIGN_CENTER, 150 * (i-1), 85);

        lv_obj_t* ui_Image27 = lv_image_create(ui_Image28);
        lv_image_set_src(ui_Image27, &ui_img_s3_light1_off_png);
        lv_obj_align(ui_Image27, LV_ALIGN_CENTER, 1, -72);
#if UI_NO_FEEDBACK_MODE
        lv_image_set_src(ui_Image27, &ui_img_s3_switch1_on_png);
        lv_obj_add_flag(ui_Image27, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);   /// Flags
#endif

        lv_obj_t* ui_Image30 = lv_image_create(ui_Image28);
        lv_image_set_src(ui_Image30, &ui_img_s3_switch1_off_png);
        lv_obj_align(ui_Image30, LV_ALIGN_CENTER, -1, 52);
        lv_obj_add_flag(ui_Image30, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);   /// Flags

        lv_obj_t* ui_Label8 = lv_label_create(ui_Image28);
        lv_obj_align(ui_Label8, LV_ALIGN_CENTER, 0, -7);
        lv_obj_set_style_text_color(ui_Label8, lv_color_hex(0x9F9F9F), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(ui_Label8, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(ui_Label8, &ui_font_Font3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label8, light_name_list[i]);

#if !(UI_NO_FEEDBACK_MODE)
        lv_obj_set_user_data(ui_Image30, (void*)(i + 1));
        lv_obj_add_event_cb(ui_Image30, ui_event_light_ud, LV_EVENT_CLICKED, ui_Image27);
#else
        lv_obj_set_user_data(ui_Image27, (void*)(i + 1));
        lv_obj_add_event_cb(ui_Image27, ui_event_light_ud, LV_EVENT_ALL, (void*)1);
        lv_obj_set_user_data(ui_Image30, (void*)(i + 1));
        lv_obj_add_event_cb(ui_Image30, ui_event_light_ud, LV_EVENT_ALL, (void*)0);
#endif
    }


    ui_create_gesture_image(ui_Screen11, _UI_SCREEN_INDEX);

    lv_obj_add_event_cb(ui_Screen11, ui_event_screen_x, LV_EVENT_GESTURE, (void*)_UI_SCREEN_INDEX);
    lv_obj_add_event_cb(ui_Image26, ui_event_All_eud, LV_EVENT_ALL, (void*)1);
    lv_obj_add_event_cb(ui_Image32, ui_event_All_eud, LV_EVENT_ALL, (void*)0);
    return ui_Screen11;
}

void ui_Screen11_set_command_cb(void (*command_cb)(int32_t index, bool on))
{
    _command_cb = command_cb;
}


static void ui_event_All_eud(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        lv_obj_set_style_image_recolor_opa(target, LV_OPA_20, 0);
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_CLICKED) {
        lv_obj_set_style_image_recolor_opa(target, LV_OPA_TRANSP, 0);
        if (code == LV_EVENT_CLICKED) {
            _command_cb(0, lv_event_get_user_data(e) != 0);
        }
    }
}

void ui_event_light_ud(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
#if !(UI_NO_FEEDBACK_MODE)
    lv_obj_t* other = lv_event_get_user_data(e);
    // next on
    bool on = lv_image_get_src(target) != &ui_img_s3_switch1_on_png;
    _command_cb((int32_t)lv_obj_get_user_data(target), on);
    lv_image_set_src(target, on?  &ui_img_s3_switch1_on_png : &ui_img_s3_switch1_off_png);
    lv_image_set_src(other, on? &ui_img_s3_light1_on_png : &ui_img_s3_light1_off_png);
#else
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        lv_obj_set_style_image_recolor_opa(target, LV_OPA_20, 0);
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_CLICKED) {
        lv_obj_set_style_image_recolor_opa(target, LV_OPA_TRANSP, 0);
        if (code == LV_EVENT_CLICKED) {
            bool on = lv_event_get_user_data(e) != NULL;
            _command_cb((int32_t)lv_obj_get_user_data(target), on);
        }
    }
#endif
}
