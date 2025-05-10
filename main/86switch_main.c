/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#if CONFIG_SWITCH86_LOG_SVR_ENABLE
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_st7701.h"
#include "lvgl_port.h"
#include "yos_nvs.h"
#include "yos_wifi.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define SWITCH86_LCD_H_RES               (480)          // LVGL_PORT_H_RES
#define SWITCH86_LCD_V_RES               (480)          // LVGL_PORT_V_RES
#define SWITCH86_LCD_PCLK_HZ             (11 * 1000 * 1000)
#define SWITCH86_LCD_BIT_PER_PIXEL       (18)
#define SWITCH86_RGB_BIT_PER_PIXEL       (16)
#define SWITCH86_RGB_DATA_WIDTH          (16)
#define SWITCH86_RGB_BOUNCE_BUFFER_SIZE  (SWITCH86_LCD_H_RES * CONFIG_SWITCH86_LCD_RGB_BOUNCE_BUFFER_HEIGHT)
#define SWITCH86_LCD_IO_RGB_DISP         (-1)             // -1 if not used
#define SWITCH86_LCD_IO_RGB_VSYNC        (GPIO_NUM_17)
#define SWITCH86_LCD_IO_RGB_HSYNC        (GPIO_NUM_16)
#define SWITCH86_LCD_IO_RGB_DE           (GPIO_NUM_18)
#define SWITCH86_LCD_IO_RGB_PCLK         (GPIO_NUM_21)
#define SWITCH86_LCD_IO_RGB_DATA0        (GPIO_NUM_4)
#define SWITCH86_LCD_IO_RGB_DATA1        (GPIO_NUM_5)
#define SWITCH86_LCD_IO_RGB_DATA2        (GPIO_NUM_6)
#define SWITCH86_LCD_IO_RGB_DATA3        (GPIO_NUM_7)
#define SWITCH86_LCD_IO_RGB_DATA4        (GPIO_NUM_15)
#define SWITCH86_LCD_IO_RGB_DATA5        (GPIO_NUM_8)
#define SWITCH86_LCD_IO_RGB_DATA6        (GPIO_NUM_20)
#define SWITCH86_LCD_IO_RGB_DATA7        (GPIO_NUM_3)
#define SWITCH86_LCD_IO_RGB_DATA8        (GPIO_NUM_46)
#define SWITCH86_LCD_IO_RGB_DATA9        (GPIO_NUM_9)
#define SWITCH86_LCD_IO_RGB_DATA10       (GPIO_NUM_10)
#define SWITCH86_LCD_IO_RGB_DATA11       (GPIO_NUM_11)
#define SWITCH86_LCD_IO_RGB_DATA12       (GPIO_NUM_12)
#define SWITCH86_LCD_IO_RGB_DATA13       (GPIO_NUM_13)
#define SWITCH86_LCD_IO_RGB_DATA14       (GPIO_NUM_14)
#define SWITCH86_LCD_IO_RGB_DATA15       (GPIO_NUM_0)
#define SWITCH86_LCD_IO_SPI_CS           (GPIO_NUM_39)
#define SWITCH86_LCD_IO_SPI_SCL          (GPIO_NUM_48)
#define SWITCH86_LCD_IO_SPI_SDA          (GPIO_NUM_47)
#define SWITCH86_LCD_IO_RST              (-1)             // -1 if not used
#define SWITCH86_PIN_NUM_BK_LIGHT        (GPIO_NUM_38)    // -1 if not used
#define SWITCH86_LCD_BK_LIGHT_ON_LEVEL   (1)
#define SWITCH86_LCD_BK_LIGHT_OFF_LEVEL  !SWITCH86_LCD_BK_LIGHT_ON_LEVEL

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your touch spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if CONFIG_SWITCH86_LCD_TOUCH_CONTROLLER_GT911
#define SWITCH86_TOUCH_HOST              (I2C_NUM_0)
#define SWITCH86_PIN_NUM_TOUCH_SCL       (GPIO_NUM_45)
#define SWITCH86_PIN_NUM_TOUCH_SDA       (GPIO_NUM_19)
#define SWITCH86_PIN_NUM_TOUCH_RST       (-1)            // -1 if not used
#define SWITCH86_PIN_NUM_TOUCH_INT       (-1)            // -1 if not used
#endif

static const char *TAG = "switch86";
static esp_lcd_panel_handle_t _lcd_handle = NULL;


static const st7701_lcd_init_cmd_t _lcd_init_cmds[] = {
//  {cmd, { data }, data_size, delay_ms}
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t []){0x3B, 0x00}, 2, 0},
    {0xC1, (uint8_t []){0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t []){0x31, 0x05}, 2, 0},
    {0xCD, (uint8_t []){0x00}, 1, 0},
    {0xB0, (uint8_t []){0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18}, 16, 0},
    {0xB1, (uint8_t []){0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18}, 16, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t []){0x60}, 1, 0},
    {0xB1, (uint8_t []){0x32}, 1, 0},
    {0xB2, (uint8_t []){0x07}, 1, 0},
    {0xB3, (uint8_t []){0x80}, 1, 0},
    {0xB5, (uint8_t []){0x49}, 1, 0},
    {0xB7, (uint8_t []){0x85}, 1, 0},
    {0xB8, (uint8_t []){0x21}, 1, 0},
    {0xC1, (uint8_t []){0x78}, 1, 0},
    {0xC2, (uint8_t []){0x78}, 1, 0},
    {0xE0, (uint8_t []){0x00, 0x1B, 0x02}, 3, 0},
    {0xE1, (uint8_t []){0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}, 11, 0},
    {0xE2, (uint8_t []){0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}, 12, 0},
    {0xE3, (uint8_t []){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t []){0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0}, 16, 0},
    {0xE6, (uint8_t []){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t []){0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0}, 16, 0},
    {0xEB, (uint8_t []){0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}, 7, 0},
    {0xEC, (uint8_t []){0x3C, 0x00}, 2, 0},
    {0xED, (uint8_t []){0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA}, 16, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xE5, (uint8_t []){0xE4}, 1, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x29, (uint8_t []){0x00}, 0, 0},
};

static esp_lcd_panel_handle_t _lcd_init(void)
{
    ESP_LOGI(TAG, "Install 3-wire SPI panel IO");
    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO,
        .cs_gpio_num = SWITCH86_LCD_IO_SPI_CS,
        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = SWITCH86_LCD_IO_SPI_SCL,
        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = SWITCH86_LCD_IO_SPI_SDA,
        .io_expander = NULL,
    };
    esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

    ESP_LOGI(TAG, "Install ST7701 panel driver");
    esp_lcd_panel_handle_t lcd_handle = NULL;
    esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .psram_trans_align = 64,
        .data_width = SWITCH86_RGB_DATA_WIDTH,
        .bits_per_pixel = SWITCH86_RGB_BIT_PER_PIXEL,
        .de_gpio_num = SWITCH86_LCD_IO_RGB_DE,
        .pclk_gpio_num = SWITCH86_LCD_IO_RGB_PCLK,
        .vsync_gpio_num = SWITCH86_LCD_IO_RGB_VSYNC,
        .hsync_gpio_num = SWITCH86_LCD_IO_RGB_HSYNC,
        .disp_gpio_num = SWITCH86_LCD_IO_RGB_DISP,
        .data_gpio_nums = {
            SWITCH86_LCD_IO_RGB_DATA0,
            SWITCH86_LCD_IO_RGB_DATA1,
            SWITCH86_LCD_IO_RGB_DATA2,
            SWITCH86_LCD_IO_RGB_DATA3,
            SWITCH86_LCD_IO_RGB_DATA4,
            SWITCH86_LCD_IO_RGB_DATA5,
            SWITCH86_LCD_IO_RGB_DATA6,
            SWITCH86_LCD_IO_RGB_DATA7,
            SWITCH86_LCD_IO_RGB_DATA8,
            SWITCH86_LCD_IO_RGB_DATA9,
            SWITCH86_LCD_IO_RGB_DATA10,
            SWITCH86_LCD_IO_RGB_DATA11,
            SWITCH86_LCD_IO_RGB_DATA12,
            SWITCH86_LCD_IO_RGB_DATA13,
            SWITCH86_LCD_IO_RGB_DATA14,
            SWITCH86_LCD_IO_RGB_DATA15,
        },
        //.timings = ST7701_480_480_PANEL_60HZ_RGB_TIMING(),
        .timings = { 
            .pclk_hz = SWITCH86_LCD_PCLK_HZ,
            .h_res = SWITCH86_LCD_H_RES, 
            .v_res = SWITCH86_LCD_V_RES, 
            .hsync_pulse_width = 8, 
            .hsync_back_porch = 50,
            .hsync_front_porch = 10, 
            .vsync_pulse_width = 8,
            .vsync_back_porch = 20, 
            .vsync_front_porch = 10,
            .flags.pclk_active_neg = false,
        },
        .flags.fb_in_psram = 1,
        .num_fbs = LVGL_PORT_LCD_RGB_BUFFER_NUMS,
        .bounce_buffer_size_px = SWITCH86_RGB_BOUNCE_BUFFER_SIZE,
    };
	//rgb_config.timings.pclk_hz = SWITCH86_LCD_PCLK_HZ;
    //rgb_config.timings.h_res = SWITCH86_LCD_H_RES;
    //rgb_config.timings.v_res = SWITCH86_LCD_V_RES;
    st7701_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,
        .init_cmds = _lcd_init_cmds,      // Uncomment these line if use custom initialization commands
        .init_cmds_size = sizeof(_lcd_init_cmds) / sizeof(_lcd_init_cmds[0]),
        .flags = {
            .auto_del_panel_io = 0,         /**
                                             * Set to 1 if panel IO is no longer needed after LCD initialization.
                                             * If the panel IO pins are sharing other pins of the RGB interface to save GPIOs,
                                             * Please set it to 1 to release the pins.
                                             */
            .mirror_by_cmd = 1,             // Set to 0 if `auto_del_panel_io` is enabled
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = SWITCH86_LCD_IO_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = SWITCH86_LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, &lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));
    esp_lcd_panel_disp_on_off(lcd_handle, true);

    return lcd_handle;
}


static esp_lcd_touch_handle_t _touch_init(void)
{
    esp_lcd_touch_handle_t tp_handle = NULL;
#if CONFIG_SWITCH86_LCD_TOUCH_CONTROLLER_GT911
    ESP_LOGI(TAG, "Initialize I2C bus");
    const i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = SWITCH86_TOUCH_HOST,
        .sda_io_num = SWITCH86_PIN_NUM_TOUCH_SDA,
        .scl_io_num = SWITCH86_PIN_NUM_TOUCH_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = 400 * 1000; // must

    ESP_LOGI(TAG, "Initialize I2C panel IO");
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &tp_io_config, &tp_io_handle));

    ESP_LOGI(TAG, "Initialize touch controller GT911");
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = SWITCH86_LCD_H_RES,
        .y_max = SWITCH86_LCD_V_RES,
        .rst_gpio_num = SWITCH86_PIN_NUM_TOUCH_RST,
        .int_gpio_num = SWITCH86_PIN_NUM_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp_handle));
#endif // CONFIG_SWITCH86_LCD_TOUCH_CONTROLLER_GT911

    return tp_handle;
}

static void esp_lcd_sleep(bool sleep)
{
    ESP_LOGI(TAG, "sleep:%d", sleep);
	//esp_lcd_panel_disp_sleep(_lcd_handle, false);
	esp_lcd_panel_disp_on_off(_lcd_handle, !sleep);
	gpio_set_level(SWITCH86_PIN_NUM_BK_LIGHT, sleep? SWITCH86_LCD_BK_LIGHT_OFF_LEVEL : SWITCH86_LCD_BK_LIGHT_ON_LEVEL);
}

#if CONFIG_SWITCH86_LOG_SVR_ENABLE
static int udp_log_write(const char* format, va_list args) {
    static int udp_socket = -1;
    static struct sockaddr_in dest_addr;
    if (udp_socket == -1){
        udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (udp_socket < 0) {
            ESP_LOGE("UDP_LOG", "Failed to create socket: errno %d", errno);
            return 0;
        }
        dest_addr.sin_addr.s_addr = inet_addr(CONFIG_SWITCH86_LOG_SVR_HOST);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(CONFIG_SWITCH86_LOG_SVR_PORT);
    }

    char buffer[512];
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    
    sendto(udp_socket, buffer, strlen(buffer), 0,  (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    return len; // over 512 bytes will be truncated
}
#endif

void app_main()
{
    ESP_LOGI(TAG, "begin esp_get_free_heap_size:%ld, _internal_heap_size: %ld", esp_get_free_heap_size(), esp_get_free_internal_heap_size());

    ESP_ERROR_CHECK(yos_nvs_init());

    if (SWITCH86_PIN_NUM_BK_LIGHT >= 0) {
        ESP_LOGI(TAG, "Turn off LCD backlight");
        gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << SWITCH86_PIN_NUM_BK_LIGHT
        };
        ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    }
   
    esp_lcd_panel_handle_t lcd_handle = _lcd_init();

    esp_lcd_touch_handle_t tp_handle = _touch_init();

    ESP_ERROR_CHECK(lvgl_port_init(lcd_handle, tp_handle));

    if (SWITCH86_PIN_NUM_BK_LIGHT >= 0) {
        ESP_LOGI(TAG, "Turn on LCD backlight");
        gpio_set_level(SWITCH86_PIN_NUM_BK_LIGHT, SWITCH86_LCD_BK_LIGHT_ON_LEVEL);
		
		_lcd_handle = lcd_handle;
		lvgl_port_set_lcd_sleep_cb(esp_lcd_sleep);
    }

    if (yos_wifi_station_init() == ESP_OK){ // 必须第一个运行   
#if CONFIG_SWITCH86_LOG_SVR_ENABLE
        esp_log_set_vprintf(udp_log_write);
#endif
    } 

    extern esp_err_t rec_asr_init(void (*sleep_cb)(bool sleep));
    extern esp_err_t time_sync_init(void);   
    time_sync_init();
    rec_asr_init(esp_lcd_sleep);
    
    ESP_LOGI(TAG, "end esp_get_free_heap_size:%ld, _internal_heap_size: %ld", esp_get_free_heap_size(), esp_get_free_internal_heap_size());
 }
