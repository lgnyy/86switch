

#include "ui.h"


///////////////////// DECLARE ////////////////////
LV_IMG_DECLARE(ui_img_s1_cut1_png);   // assets\s1\cut1.png
LV_IMG_DECLARE(ui_img_s1_cut2_png);   // assets\s1\cut2.png


///////////////////// VARIABLES ////////////////////
static lv_obj_t* ui_screen_page_list[5];
static lv_obj_t* ui_screen_config_list[3];


///////////////////// ANIMATIONS ////////////////////

///////////////////// FUNCTIONS ////////////////////
///////////////////// SCREENS ////////////////////

void ui_init( void )
{
    lv_disp_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    ui_screen_page_list[0] = ui_Screen1_screen_init();
    ui_screen_page_list[1] = ui_Screen10_screen_init();
    ui_screen_page_list[2] = ui_Screen11_screen_init();
    ui_screen_page_list[3] = ui_Screen12_screen_init();
    ui_screen_page_list[4] = ui_Screen13_screen_init();
 }

lv_obj_t* ui_screen_get(int32_t index)
{
    if (index < 0) {
        int32_t i = -1 - index;
        if (ui_screen_config_list[i] == NULL) {
            ui_screen_config_list[i] = ui_Screen2_screen_init();
        }
        return ui_screen_config_list[i];
    }
    return ui_screen_page_list[index];
}

void ui_screen_change(int32_t index)
{
    lv_obj_t* target = ui_screen_get(index);
    lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
}

void ui_event_screen_x(lv_event_t* e)
{
    //lv_obj_t* target = lv_event_get_target(e);
    int32_t index = (int32_t)lv_event_get_user_data(e);
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_LEFT) {
        lv_indev_wait_release(lv_indev_active());
        ui_screen_change((index == 4) ? 0 : (index + 1));
    }
    else if (dir == LV_DIR_RIGHT) {
        lv_indev_wait_release(lv_indev_active());
        ui_screen_change((index == 0) ? 4 : (index - 1));
    }
}

void ui_create_gesture_image(lv_obj_t* parent, int curr_index)
{
    for (int i = 0; i < 5; i++) {
        lv_obj_t* ui_Image3 = lv_image_create(parent);
        lv_image_set_src(ui_Image3, (i == curr_index) ? &ui_img_s1_cut1_png : &ui_img_s1_cut2_png);
        lv_obj_align(ui_Image3, LV_ALIGN_CENTER, -60 + 30 * i, 215);
    }
}