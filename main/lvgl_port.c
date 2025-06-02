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
#include "yos_http.h"
#include "lvgl.h"
#include "lvgl_port.h"

static const char *TAG = "lvgl_port";
static TaskHandle_t lvgl_task_handle = NULL;
static int64_t lvgl_touchpad_time = 0;
static void (*_lcd_sleep_cb)(bool sleep);

static void mem_fs_init(void);


static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
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

    mem_fs_init();
    return ESP_OK;
}

void lvgl_port_set_lcd_sleep_cb(void (*sleep_cb)(bool sleep))
{
	_lcd_sleep_cb = sleep_cb;
}


/** mem_fs */
typedef struct _mem_fs_buf_s {
    uint8_t* base;
    uint32_t size;
    uint32_t offset;
}_mem_fs_buf_t;
static lv_fs_drv_t mem_fs_drv;
static _mem_fs_buf_t mem_fs_buf;


static void* mem_fs_open(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode) {
    mem_fs_buf.offset = 0;
    return &mem_fs_buf;
}
static lv_fs_res_t mem_fs_close(lv_fs_drv_t* drv, void* file_p) {
    return LV_FS_RES_OK;
}
static lv_fs_res_t mem_fs_read(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br) {
    _mem_fs_buf_t* mem = (_mem_fs_buf_t*)file_p;
    uint32_t ss = ((mem->size - mem->offset) >= btr) ? btr : (mem->size - mem->offset);
    lv_memcpy(buf, mem->base + mem->offset, ss);
    mem->offset += ss;
    *br = ss;
    return LV_FS_RES_OK;
}
static lv_fs_res_t mem_fs_seek(lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence) {
    if (whence == LV_FS_SEEK_SET) {
        ((_mem_fs_buf_t*)file_p)->offset = pos;
    }
    else if (whence == LV_FS_SEEK_CUR) {
        ((_mem_fs_buf_t*)file_p)->offset += pos;
    }
    else {
        ((_mem_fs_buf_t*)file_p)->offset = ((_mem_fs_buf_t*)file_p)->size - pos;
    }
    return LV_FS_RES_OK;
}
static lv_fs_res_t mem_fs_tell(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p) {
    *pos_p = ((_mem_fs_buf_t*)file_p)->offset;
    return LV_FS_RES_OK;
}

static void mem_fs_init(void) {
    lv_fs_drv_init(&mem_fs_drv);
    mem_fs_drv.letter = 'M';
    mem_fs_drv.open_cb = mem_fs_open;
    mem_fs_drv.close_cb = mem_fs_close;
    mem_fs_drv.read_cb = mem_fs_read;
    mem_fs_drv.seek_cb = mem_fs_seek;
    mem_fs_drv.tell_cb = mem_fs_tell;
    lv_fs_drv_register(&mem_fs_drv);
}

static lv_obj_t* dyn_img = NULL;
static void display_dyn_img_task(void *pvParameters){
    const char* header = "Authorization: Basic " CONFIG_SWITCH86_LVGL_PORT_IMG_AUTH;
    yos_http_static_request(CONFIG_SWITCH86_LVGL_PORT_IMG_URL, NULL, header, NULL, 0, &(mem_fs_buf.base), &(mem_fs_buf.size));
    if (mem_fs_buf.base != NULL) {
        lv_lock();
        if (mem_fs_buf.base[0] == 'G'){
            dyn_img = lv_gif_create(lv_layer_top());
            lv_obj_center(dyn_img);
            lv_img_dsc_t img_dsc = { .data = mem_fs_buf.base, .data_size = mem_fs_buf.size };
            lv_gif_set_src(dyn_img, &img_dsc);
            //lv_gif_set_loop_count(dyn_img, 1);

        }else{
            dyn_img = lv_image_create(lv_layer_top());
            lv_obj_center(dyn_img);
            lv_image_set_src(dyn_img, "M:http.jpeg");
        }
        lv_unlock();
    }

    vTaskDelete(NULL);
}

void lvgl_port_display_dyn_img(bool sleep) {
    if (dyn_img != NULL) {
        lv_lock();
        lv_obj_del(dyn_img);
        dyn_img = NULL;
        lv_unlock();
    }
    if (mem_fs_buf.base != NULL) {
        yos_http_static_free(mem_fs_buf.base);
        mem_fs_buf.base = NULL;
        mem_fs_buf.size = 0;
    }

    if (!sleep) {
        xTaskCreate(&display_dyn_img_task, "dyn_img_task", 0x2000, NULL, 5, NULL);
    }
}