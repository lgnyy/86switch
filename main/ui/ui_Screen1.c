

#include "ui.h"

#define _UI_SCREEN_INDEX 0

LV_IMG_DECLARE(ui_img_s1_back1_png);   // assets\s1\back1.png
LV_IMG_DECLARE(ui_img_s1_sunny_png);   // assets\s1\sunny.png
LV_IMG_DECLARE(ui_img_s1_tmp_png);   // assets\s1\tmp.png
LV_IMG_DECLARE(ui_img_s1_wind_png);   // assets\s1\wind.png
LV_IMG_DECLARE(ui_img_s1_hu_png);   // assets\s1\hu.png


lv_obj_t* ui_Screen1_screen_init(void)
{
    lv_obj_t* ui_Screen1 = lv_obj_create(NULL);
    lv_obj_remove_flag( ui_Screen1, LV_OBJ_FLAG_SCROLLABLE );    /// Flags

    lv_obj_t* ui_Image1 = lv_image_create(ui_Screen1);
    lv_image_set_src(ui_Image1, &ui_img_s1_back1_png);
    lv_obj_set_align(ui_Image1, LV_ALIGN_CENTER);
    //lv_obj_add_flag( ui_Image1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST );   /// Flags

    lv_obj_t* ui_Image2 = lv_image_create(ui_Screen1);
    lv_image_set_src(ui_Image2, &ui_img_s1_sunny_png);
    lv_obj_align(ui_Image2, LV_ALIGN_CENTER, -172, -79);
    lv_obj_t* ui_Label2 = lv_label_create(ui_Screen1);
    lv_obj_align(ui_Label2, LV_ALIGN_CENTER, -133, -77);
    lv_obj_set_style_text_font(ui_Label2, &ui_font_Font2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label2, "晴");


    lv_obj_t* ui_Label1 = lv_label_create(ui_Screen1);
    lv_obj_align(ui_Label1, LV_ALIGN_CENTER, -103, 0);
    lv_obj_set_style_text_font(ui_Label1, &ui_font_Font1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label1, "08:00");

    lv_obj_t* ui_Label3 = lv_label_create(ui_Screen1);
    lv_obj_align(ui_Label3, LV_ALIGN_CENTER, -100, 52);
    lv_obj_set_style_text_font(ui_Label3, &ui_font_Font4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label3, "星期天 2023/11/26");


    lv_obj_t* ui_Image6 = lv_image_create(ui_Screen1);
    lv_image_set_src(ui_Image6, &ui_img_s1_tmp_png);
    lv_obj_align(ui_Image6, LV_ALIGN_CENTER, -180, 102);
    lv_obj_t* ui_Label4 = lv_label_create(ui_Screen1);
    lv_obj_align(ui_Label4, LV_ALIGN_CENTER, -155, 108);
    lv_obj_set_style_text_font(ui_Label4, &ui_font_Font3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label4, "28");
    lv_obj_t* ui_Label15 = lv_label_create(ui_Screen1);
    lv_obj_align(ui_Label15, LV_ALIGN_CENTER, -137, 108);
    lv_obj_set_style_text_font(ui_Label15, &ui_font_Font3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label15, "℃");

    lv_obj_t* ui_Image7 = lv_image_create(ui_Screen1);
    lv_image_set_src(ui_Image7, &ui_img_s1_wind_png);
    lv_obj_align(ui_Image7, LV_ALIGN_CENTER, -90, 102);
    lv_obj_t* ui_Label5 = lv_label_create(ui_Screen1);
    lv_obj_align(ui_Label5, LV_ALIGN_CENTER, -67, 108);
    lv_obj_set_style_text_font(ui_Label5, &ui_font_Font3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label5, "4");
    lv_obj_t* ui_Label16 = lv_label_create(ui_Screen1);
    lv_obj_align(ui_Label16, LV_ALIGN_CENTER, -44, 108);
    lv_obj_set_style_text_font(ui_Label16, &ui_font_Font3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label16, "级风");

    lv_obj_t* ui_Image8 = lv_image_create(ui_Screen1);
    lv_image_set_src(ui_Image8, &ui_img_s1_hu_png);
    lv_obj_align(ui_Image8, LV_ALIGN_CENTER, 6, 102);
    lv_obj_t* ui_Label6 = lv_label_create(ui_Screen1);
    lv_obj_align(ui_Label6, LV_ALIGN_CENTER, 32, 108);
    lv_obj_set_style_text_font(ui_Label6, &ui_font_Font3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label6, "68");
    lv_obj_t* ui_Label17 = lv_label_create(ui_Screen1);
    lv_obj_align(ui_Label17, LV_ALIGN_CENTER, 51, 108);
    lv_obj_set_style_text_font(ui_Label17, &ui_font_Font3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label17, "%");

    ui_create_gesture_image(ui_Screen1, _UI_SCREEN_INDEX);

    lv_obj_add_event_cb(ui_Screen1, ui_event_screen_x, LV_EVENT_GESTURE, (void*)_UI_SCREEN_INDEX);
    return ui_Screen1;
}

void ui_Screen1_set_date_time(const char* date, const char* time)
{
    lv_obj_t* ui_Screen1 = ui_screen_get(_UI_SCREEN_INDEX);
    lv_label_set_text(lv_obj_get_child(ui_Screen1, 3), time);
    lv_label_set_text(lv_obj_get_child(ui_Screen1, 4), date);
}

void ui_Screen1_set_weather_info(int index, const char* value)
{
    const int32_t idxs[] = {2, 6, 9, 12};
    lv_obj_t* ui_Screen1 = ui_screen_get(_UI_SCREEN_INDEX);
    lv_label_set_text(lv_obj_get_child(ui_Screen1, idxs[index]), value);
}