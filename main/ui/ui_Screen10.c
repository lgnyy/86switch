

#include "ui.h"

#define _UI_SCREEN_INDEX 1
#define _UI_SCREEN10_2CLOSE 1

LV_IMG_DECLARE(ui_img_s2_back2_png);   // assets\s2\back2.png
LV_IMG_DECLARE(ui_img_s2_color_png);   // assets\s2\color.png
LV_IMG_DECLARE(ui_img_s2_light_png);   // assets\s2\light.png
LV_IMG_DECLARE(ui_img_s2_switch2_off_png);   // assets\s2\switch2_off.png
#if !(_UI_SCREEN10_2CLOSE)
LV_IMG_DECLARE(ui_img_s2_switch2_on_png);   // assets\s2\switch2_on.png
#endif

static void (*_command_cb)(int32_t index, int32_t lightp, int32_t colorp);
static void ui_event_Image16(lv_event_t* e);
static void ui_event_Image17(lv_event_t* e);
static void ui_event_Slider1(lv_event_t* e);


lv_obj_t* ui_Screen10_screen_init(void)
{
    lv_obj_t* ui_Screen10 = lv_obj_create(NULL);
    lv_obj_remove_flag( ui_Screen10, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
    lv_obj_set_style_bg_color(ui_Screen10, lv_color_hex(0x181C20), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* ui_Image11 = lv_image_create(ui_Screen10);
    lv_image_set_src(ui_Image11, &ui_img_s2_back2_png);
    lv_obj_set_align(ui_Image11, LV_ALIGN_BOTTOM_MID);
    //lv_obj_add_flag( ui_Image11, LV_OBJ_FLAG_CLICKABLE );   /// Flags
    //lv_obj_remove_flag( ui_Image11, LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_CHAIN );    /// Flags

    lv_obj_t* ui_Image16 = lv_image_create(ui_Screen10);
    lv_image_set_src(ui_Image16, &ui_img_s2_switch2_off_png);
    lv_obj_align(ui_Image16, LV_ALIGN_CENTER, _UI_SCREEN10_2CLOSE ?80:0, -140);
    lv_obj_add_flag(ui_Image16, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);   /// Flags

#if !(_UI_SCREEN10_2CLOSE)
    lv_obj_t* ui_Image17 = lv_image_create(ui_Screen10);
    lv_image_set_src(ui_Image17, &ui_img_s2_switch2_on_png);
    lv_obj_align(ui_Image17, LV_ALIGN_CENTER, 0, -140);
    lv_obj_add_flag( ui_Image17, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
#else
    lv_obj_t* ui_Image17 = lv_image_create(ui_Screen10);
    lv_image_set_src(ui_Image17, &ui_img_s2_switch2_off_png);
    lv_image_set_scale(ui_Image17, 200);
    lv_obj_align(ui_Image17, LV_ALIGN_CENTER, -100, -140);
    lv_obj_add_flag(ui_Image17, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);   /// Flags
#endif

    lv_obj_t* ui_Image15 = lv_image_create(ui_Screen10);
    lv_image_set_src(ui_Image15, &ui_img_s2_light_png);
    lv_obj_align(ui_Image15, LV_ALIGN_CENTER, -171, -69);

    static lv_style_t slider_style;
    lv_style_init(&slider_style);

    lv_obj_t* ui_Slider1 = lv_slider_create(ui_Screen10);
    lv_slider_set_value( ui_Slider1, 5, LV_ANIM_OFF);
    //if (lv_slider_get_mode(ui_Slider1)==LV_SLIDER_MODE_RANGE ) lv_slider_set_left_value( ui_Slider1, 0, LV_ANIM_OFF);
    lv_obj_set_size( ui_Slider1, 374, 52);
    lv_obj_align(ui_Slider1, LV_ALIGN_CENTER, 0, -7);


    lv_obj_t* ui_Image14 = lv_image_create(ui_Screen10);
    lv_image_set_src(ui_Image14, &ui_img_s2_color_png);
    lv_obj_align(ui_Image14, LV_ALIGN_CENTER, -172, 46);

    lv_obj_t* ui_Slider2 = lv_slider_create(ui_Screen10);
    lv_slider_set_value( ui_Slider2, 10, LV_ANIM_OFF);
    //if (lv_slider_get_mode(ui_Slider2)==LV_SLIDER_MODE_RANGE ) lv_slider_set_left_value( ui_Slider2, 0, LV_ANIM_OFF);
    lv_obj_set_size( ui_Slider2, 374, 53 );
    lv_obj_align(ui_Slider2, LV_ALIGN_CENTER, 0, 101);


    for (int i = 0; i < 2; i++) {
        lv_obj_t* ui_Slider = (i == 0) ? ui_Slider1 : ui_Slider2;
        lv_obj_set_style_radius(ui_Slider, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_Slider, lv_color_hex(0x202429), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_Slider, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(ui_Slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_Slider, lv_color_hex(0xF2BC53), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_Slider, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        //lv_obj_set_style_bg_color(ui_Slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT );
        lv_obj_set_style_bg_opa(ui_Slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    }


    ui_create_gesture_image(ui_Screen10, _UI_SCREEN_INDEX);

    lv_obj_add_event_cb(ui_Screen10, ui_event_screen_x, LV_EVENT_GESTURE, (void*)_UI_SCREEN_INDEX);
#if !(_UI_SCREEN10_2CLOSE)
    lv_obj_add_event_cb(ui_Image16, ui_event_Image16, LV_EVENT_CLICKED, ui_Image17);
    lv_obj_add_event_cb(ui_Image17, ui_event_Image17, LV_EVENT_CLICKED, ui_Image16);
#else
    lv_obj_add_event_cb(ui_Image16, ui_event_Image16, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Image17, ui_event_Image17, LV_EVENT_ALL, NULL);
#endif
    lv_obj_add_event_cb(ui_Slider1, ui_event_Slider1, LV_EVENT_CLICKED, ui_Slider2);
 
   return ui_Screen10;
}

void ui_Screen10_set_command_cb(void (*command_cb)(int32_t index, int32_t lightp, int32_t colorp))
{
    _command_cb = command_cb;
}


static void ui_event_Image16(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
#if !(_UI_SCREEN10_2CLOSE)
    lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lv_event_get_user_data(e), LV_OBJ_FLAG_HIDDEN);

    _command_cb(1, 80, 80);
#else
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        lv_obj_set_style_image_recolor_opa(target, LV_OPA_50, 0);
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_CLICKED) {
        lv_obj_set_style_image_recolor_opa(target, LV_OPA_TRANSP, 0);
        if (code == LV_EVENT_CLICKED) {
            _command_cb(-1, 0, 0);
        }
    }
#endif
}

static void ui_event_Image17(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
    
#if !(_UI_SCREEN10_2CLOSE)
    lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lv_event_get_user_data(e), LV_OBJ_FLAG_HIDDEN);

    _command_cb(0, 0, 0);
#else
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        lv_obj_set_style_image_recolor_opa(target, LV_OPA_50, 0);
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_CLICKED) {
        lv_obj_set_style_image_recolor_opa(target, LV_OPA_TRANSP, 0);
        if (code == LV_EVENT_CLICKED) {
            _command_cb(0, 0, 0);
        }
    }
#endif
}

static void ui_event_Slider1(lv_event_t* e)
{
    lv_obj_t* ui_Slider1 = lv_event_get_target(e);
    int32_t v1 = lv_slider_get_value(ui_Slider1);
    if (v1 > 5) {
        lv_obj_t* ui_Slider2 = lv_event_get_user_data(e);
        int32_t v2 = lv_slider_get_value(ui_Slider2);
        _command_cb(1, v1, v2);
    }
    else {
        _command_cb(1, 0, 0);
    }
}
