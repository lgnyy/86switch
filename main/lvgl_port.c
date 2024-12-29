/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_port.h"

static const char *TAG = "lvgl_port";
static TaskHandle_t lvgl_task_handle = NULL;
static int64_t lvgl_touchpad_time = 0;
static void (*_lcd_sleep_cb)(bool sleep);


void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) lv_display_get_user_data(disp_drv);
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;

    /* Just copy data from the color map to the RGB frame buffer */
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);

    lv_disp_flush_ready(disp_drv);
}


static lv_display_t *display_init(esp_lcd_panel_handle_t panel_handle)
{
    ESP_LOGD(TAG, "Malloc memory for LVGL buffer");
    ESP_LOGI(TAG, "esp_get_free_heap_size:%ld, _internal_heap_size: %ld", esp_get_free_heap_size(), esp_get_free_internal_heap_size());

    int buffer_size = LVGL_PORT_H_RES * LVGL_PORT_BUFFER_HEIGHT * sizeof(lv_color16_t);
    // Normmaly, for RGB LCD, we just use one buffer for LVGL rendering
    void *buf1 = heap_caps_malloc(buffer_size, LVGL_PORT_BUFFER_MALLOC_CAPS);
    if (buf1 == NULL){
        ESP_LOGW(TAG, "esp_get_free_heap_size:%ld, _internal_heap_size: %ld", esp_get_free_heap_size(), esp_get_free_internal_heap_size());
        buf1 = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    }
    assert(buf1);
    ESP_LOGI(TAG, "LVGL buffer size: %dKB", buffer_size / 1024);

    // initialize LVGL draw buffers
    ESP_LOGD(TAG, "Register display driver to LVGL");
    lv_display_t * disp = lv_display_create(LVGL_PORT_H_RES, LVGL_PORT_V_RES);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_user_data(disp, panel_handle);

    ESP_LOGI(TAG, "LVGL LVGL_PORT_BUFFER_HEIGHT: %d",LVGL_PORT_BUFFER_HEIGHT);
    lv_display_set_buffers(disp, buf1, NULL, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    return disp;
}

static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)lv_indev_get_user_data(indev);
    assert(tp);

    uint16_t touchpad_x;
    uint16_t touchpad_y;
    uint8_t touchpad_cnt = 0;
    /* Read data from touch controller into memory */
    esp_lcd_touch_read_data(tp);

    /* Read data from touch controller */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, &touchpad_x, &touchpad_y, NULL, &touchpad_cnt, 1);
    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x;
        data->point.y = touchpad_y;
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGD(TAG, "Touch position: %d,%d", touchpad_x, touchpad_y);
        if (lvgl_touchpad_time == 0){ // 从休眠中唤醒
			_lcd_sleep_cb(false);
        }
        lvgl_touchpad_time = esp_timer_get_time();
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static lv_indev_t *indev_init(esp_lcd_touch_handle_t tp)
{
    assert(tp);
    
    /*Register a touchpad input device*/
    lv_indev_t *indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);
    lv_indev_set_user_data(indev_touchpad, tp);
    return indev_touchpad;
}

static uint32_t tick_get_time(){
    return (uint32_t)(esp_timer_get_time() / 1000);
} 
static void tick_delay(uint32_t ms){
    vTaskDelay(pdMS_TO_TICKS(ms));
}
static esp_err_t tick_init(void)
{
    lv_tick_set_cb(tick_get_time); // xTaskGetTickCount
    lv_delay_set_cb(tick_delay);
    return ESP_OK;
}

static void lvgl_port_task(void *arg)
{
    ESP_LOGD(TAG, "Starting LVGL task");

    while (1) {
		if ((lvgl_touchpad_time > 0) && ((esp_timer_get_time() - lvgl_touchpad_time) > 30000000)){ // 30秒
            ESP_LOGI(TAG, "esp_get_free_heap_size:%ld, _internal_heap_size: %ld", esp_get_free_heap_size(), esp_get_free_internal_heap_size());
			lvgl_touchpad_time = 0; // 进入休眠状态
			_lcd_sleep_cb(true);
		}
		uint32_t task_delay_ms = lv_timer_handler();

		if (task_delay_ms > LVGL_PORT_TASK_MAX_DELAY_MS) {
			task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
		} else if (task_delay_ms < LVGL_PORT_TASK_MIN_DELAY_MS) {
			task_delay_ms = LVGL_PORT_TASK_MIN_DELAY_MS;
		}
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

esp_err_t lvgl_port_init(esp_lcd_panel_handle_t lcd_handle, esp_lcd_touch_handle_t tp_handle)
{
    lv_init();

    ESP_ERROR_CHECK(tick_init());

    lv_disp_t *disp = display_init(lcd_handle);
    assert(disp);

    if (tp_handle) {
        lv_indev_t *indev = indev_init(tp_handle);
        assert(indev);
    }

    lvgl_touchpad_time = esp_timer_get_time();

#if CONFIG_SWITCH86_UI_ENABLE
    extern void ui_main(void);
    ui_main();
#else
    extern void lv_demo_widgets(void); 
    lv_demo_widgets();
#endif

    ESP_LOGI(TAG, "Create LVGL task");
    BaseType_t core_id = (LVGL_PORT_TASK_CORE < 0) ? tskNO_AFFINITY : LVGL_PORT_TASK_CORE;
    BaseType_t ret = xTaskCreatePinnedToCore(lvgl_port_task, "lvgl", LVGL_PORT_TASK_STACK_SIZE, NULL,
                                             LVGL_PORT_TASK_PRIORITY, &lvgl_task_handle, core_id);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void lvgl_port_set_lcd_sleep_cb(void (*sleep_cb)(bool sleep))
{
	_lcd_sleep_cb = sleep_cb;
}

