

#include "ui.h"

#define _UI_SCREEN_INDEX 4

LV_IMG_DECLARE(ui_img_s5_back5_png);   // assets\s5\back5.png
LV_IMG_DECLARE(ui_img_s5_hour_png);   // assets\s5\hour.png
LV_IMG_DECLARE(ui_img_s5_min_png);   // assets\s5\min.png
LV_IMG_DECLARE(ui_img_s5_sec_png);   // assets\s5\sec.png
LV_IMG_DECLARE(ui_img_s5_w_circle_png);   // assets\s5\w_circle.png
LV_IMG_DECLARE(ui_img_s5_c_circle_png);   // assets\s5\c_circle.png


lv_obj_t* ui_Screen13_screen_init(void)
{
    lv_obj_t* ui_Screen13 = lv_obj_create(NULL);
    lv_obj_remove_flag( ui_Screen13, LV_OBJ_FLAG_SCROLLABLE );    /// Flags

    lv_obj_t* ui_Image52 = lv_image_create(ui_Screen13);
    lv_image_set_src(ui_Image52, &ui_img_s5_back5_png);
    lv_obj_set_align( ui_Image52, LV_ALIGN_CENTER );

    lv_obj_t* ui_Image53 = lv_image_create(ui_Screen13);
    lv_image_set_src(ui_Image53, &ui_img_s5_hour_png);
    lv_obj_align(ui_Image53, LV_ALIGN_CENTER, -57, -45);
    lv_image_set_pivot(ui_Image53, 115, 85);
    lv_image_set_rotation(ui_Image53, 540);

    lv_obj_t* ui_Image54 = lv_image_create(ui_Screen13);
    lv_image_set_src(ui_Image54, &ui_img_s5_min_png);
    lv_obj_align( ui_Image54, LV_ALIGN_CENTER, 76, -41);
    lv_image_set_pivot(ui_Image54, 1, 83);
    lv_image_set_rotation(ui_Image54, -620);

    lv_obj_t* ui_Image55 = lv_image_create(ui_Screen13);
    lv_image_set_src(ui_Image55, &ui_img_s5_sec_png);
    lv_obj_align(ui_Image55, LV_ALIGN_CENTER, 57, -1);
    lv_image_set_pivot(ui_Image55,42,7);
    lv_image_set_rotation(ui_Image55,-905);


    lv_obj_t* ui_Image56 = lv_image_create(ui_Screen13);
    lv_image_set_src(ui_Image56, &ui_img_s5_w_circle_png);
    lv_obj_set_align(ui_Image56, LV_ALIGN_CENTER);

    lv_obj_t* ui_Image57 = lv_image_create(ui_Screen13);
    lv_image_set_src(ui_Image57, &ui_img_s5_c_circle_png);
    lv_obj_set_align(ui_Image57, LV_ALIGN_CENTER);


    ui_create_gesture_image(ui_Screen13, _UI_SCREEN_INDEX);

    lv_obj_add_event_cb(ui_Screen13, ui_event_screen_x, LV_EVENT_GESTURE, (void*)_UI_SCREEN_INDEX);
    return ui_Screen13;
}

void ui_Screen13_set_time(int hour, int min, int sec)
{
    // mod pointer
    lv_obj_t* ui_Screen13 = ui_screen_get(_UI_SCREEN_INDEX);
    lv_image_set_rotation(lv_obj_get_child(ui_Screen13, 3), sec * 60 - 770);
    lv_image_set_rotation(lv_obj_get_child(ui_Screen13, 2), min * 60 - 610);
    lv_image_set_rotation(lv_obj_get_child(ui_Screen13, 1), hour * 5 * 60 + 700);
}