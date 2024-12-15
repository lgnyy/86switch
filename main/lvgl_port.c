/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_port.h"

static const char *TAG = "lv_port";
static SemaphoreHandle_t lvgl_mux;                  // LVGL mutex
static TaskHandle_t lvgl_task_handle = NULL;


#if LVGL_PORT_AVOID_TEAR_ENABLE
#if LVGL_PORT_DIRECT_MODE
static void flush_callback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;

    /* Action after last area refresh */
    if (lv_disp_flush_is_last(drv)) {
        /* Switch the current RGB frame buffer to `color_map` */
        esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);

        /* Waiting for the last frame buffer to complete transmission */
        ulTaskNotifyValueClear(NULL, ULONG_MAX);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    lv_disp_flush_ready(drv);
}

#elif LVGL_PORT_FULL_REFRESH && LVGL_PORT_LCD_RGB_BUFFER_NUMS == 3

static lv_draw_buf_t lvgl_port_flush_bufs[3];
static lv_draw_buf_t *lvgl_port_rgb_last_buf = NULL;
static lv_draw_buf_t *lvgl_port_rgb_next_buf = NULL;
static lv_draw_buf_t *lvgl_port_flush_next_buf = NULL;

void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) lv_display_get_user_data(disp_drv);
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;

    lv_draw_buf_t * draw_buf = lv_display_get_buf_active(disp_drv);
    ESP_LOGI(TAG, "buf_active:%p, px_map:%p", draw_buf, px_map);
    lv_display_set_draw_buffers(disp_drv, draw_buf, lvgl_port_flush_next_buf);

    lvgl_port_flush_next_buf = draw_buf;

    /* Switch the current RGB frame buffer to `px_map` */
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);

    lvgl_port_rgb_next_buf = draw_buf;

    lv_display_flush_ready(disp_drv);
}

#else
#error("not support!")
#endif

#else

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

#endif /* LVGL_PORT_AVOID_TEAR_ENABLE */



static lv_display_t *display_init(esp_lcd_panel_handle_t panel_handle)
{
    //static lv_disp_draw_buf_t disp_buf = { 0 };     // Contains internal graphic buffer(s) called draw buffer(s)
    //static lv_disp_drv_t disp_drv = { 0 };          // Contains LCD panel handle and callback functions


    // alloc draw buffers used by LVGL
    void *buf1 = NULL;
    void *buf2 = NULL;

    ESP_LOGD(TAG, "Malloc memory for LVGL buffer");
    ESP_LOGI(TAG, "esp_get_free_heap_size:%ld, _internal_heap_size: %ld", esp_get_free_heap_size(), esp_get_free_internal_heap_size());
#if LVGL_PORT_AVOID_TEAR_ENABLE
    int buffer_size = LVGL_PORT_H_RES * LVGL_PORT_V_RES * sizeof(lv_color16_t);
#if LVGL_PORT_DIRECT_MODE
#if LVGL_PORT_LCD_RGB_BUFFER_NUMS == 1
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 1, &buf1));
#else
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
#endif
#elif LVGL_PORT_FULL_REFRESH && LVGL_PORT_LCD_RGB_BUFFER_NUMS == 3
    // With the usage of three buffers and full-refresh, we always have one buffer available for rendering, eliminating the need to wait for the RGB's sync signal
    void *buf0 = NULL;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 3, &buf0, &buf1, &buf2));
    lv_draw_buf_init(lvgl_port_flush_bufs+0, LVGL_PORT_H_RES, LVGL_PORT_V_RES, LV_COLOR_FORMAT_RGB565, 0, buf0, buffer_size);
    lv_draw_buf_init(lvgl_port_flush_bufs+1, LVGL_PORT_H_RES, LVGL_PORT_V_RES, LV_COLOR_FORMAT_RGB565, 0, buf1, buffer_size);
    lv_draw_buf_init(lvgl_port_flush_bufs+2, LVGL_PORT_H_RES, LVGL_PORT_V_RES, LV_COLOR_FORMAT_RGB565, 0, buf2, buffer_size);
    lvgl_port_rgb_last_buf = lvgl_port_flush_bufs+0;
    lvgl_port_rgb_next_buf = lvgl_port_rgb_last_buf;
    lvgl_port_flush_next_buf = lvgl_port_flush_bufs+2;
#endif
#else
    int buffer_size = LVGL_PORT_H_RES * LVGL_PORT_BUFFER_HEIGHT * sizeof(lv_color16_t);
    // Normmaly, for RGB LCD, we just use one buffer for LVGL rendering
    buf1 = heap_caps_malloc(buffer_size, LVGL_PORT_BUFFER_MALLOC_CAPS);
    if (buf1 == NULL){
    ESP_LOGI(TAG, "esp_get_free_heap_size:%ld, _internal_heap_size: %ld", esp_get_free_heap_size(), esp_get_free_internal_heap_size());
        buf1 = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    }
    assert(buf1);
    ESP_LOGI(TAG, "LVGL buffer size: %dKB", buffer_size / 1024);
#endif /* LVGL_PORT_AVOID_TEAR_ENABLE */

    // initialize LVGL draw buffers
    ESP_LOGD(TAG, "Register display driver to LVGL");
    lv_display_t * disp = lv_display_create(LVGL_PORT_H_RES, LVGL_PORT_V_RES);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_user_data(disp, panel_handle);
#if LVGL_PORT_AVOID_TEAR_ENABLE
#if LVGL_PORT_DIRECT_MODE
    lv_display_set_buffers(disp, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_DIRECT);
#elif LVGL_PORT_FULL_REFRESH && LVGL_PORT_LCD_RGB_BUFFER_NUMS == 3    
    lv_display_set_draw_buffers(disp, lvgl_port_rgb_last_buf+1, lvgl_port_rgb_last_buf+2);
    lv_display_set_render_mode(disp, LV_DISPLAY_RENDER_MODE_FULL);
#endif
#else
    ESP_LOGI(TAG, "LVGL LVGL_PORT_BUFFER_HEIGHT: %d",LVGL_PORT_BUFFER_HEIGHT);
    lv_display_set_buffers(disp, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
 #endif
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
static esp_err_t tick_init(void)
{
    lv_tick_set_cb(tick_get_time); // xTaskGetTickCount
    return ESP_OK;
}

static void lvgl_port_task(void *arg)
{
    ESP_LOGD(TAG, "Starting LVGL task");

    uint32_t task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
    while (1) {
        if (lvgl_port_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            lvgl_port_unlock();
        }
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

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);

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

bool lvgl_port_lock(int timeout_ms)
{
    assert(lvgl_mux && "lvgl_port_init must be called first");

    const TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    assert(lvgl_mux && "lvgl_port_init must be called first");
    xSemaphoreGiveRecursive(lvgl_mux);
}

bool lvgl_port_notify_rgb_vsync(void)
{
    BaseType_t need_yield = pdFALSE;
#if LVGL_PORT_AVOID_TEAR_ENABLE
#if LVGL_PORT_FULL_REFRESH && LVGL_PORT_LCD_RGB_BUFFER_NUMS == 3
    if (lvgl_port_rgb_next_buf != lvgl_port_rgb_last_buf) {
        lvgl_port_flush_next_buf = lvgl_port_rgb_last_buf;
        lvgl_port_rgb_last_buf = lvgl_port_rgb_next_buf;
    }
#else
    // Notify that the current RGB frame buffer has been transmitted
    xTaskNotifyFromISR(lvgl_task_handle, ULONG_MAX, eNoAction, &need_yield);
#endif
#endif
    return (need_yield == pdTRUE);
}
