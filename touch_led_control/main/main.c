#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "driver/gpio.h"  // For GPIO control
#include "driver/ledc.h"  // For PWM control

static const char *TAG = "LEDControl";

// GPIO pins for LEDs
#define LED1_GPIO GPIO_NUM_20
#define LED2_GPIO GPIO_NUM_21
#define LED3_GPIO GPIO_NUM_22

// Button and slider states
static bool button_state[3] = {false, false, false};
static uint16_t last_brightness[3] = {0, 0, 0};

// LEDC channel configuration
ledc_channel_config_t ledc_channels[] = {
    { .speed_mode = LEDC_LOW_SPEED_MODE, .channel = LEDC_CHANNEL_0, .gpio_num = LED1_GPIO, .intr_type = LEDC_INTR_DISABLE, .timer_sel = LEDC_TIMER_0, .duty = 0, .hpoint = 0 },
    { .speed_mode = LEDC_LOW_SPEED_MODE, .channel = LEDC_CHANNEL_5, .gpio_num = LED2_GPIO, .intr_type = LEDC_INTR_DISABLE, .timer_sel = LEDC_TIMER_0, .duty = 0, .hpoint = 0 },
    { .speed_mode = LEDC_LOW_SPEED_MODE, .channel = LEDC_CHANNEL_2, .gpio_num = LED3_GPIO, .intr_type = LEDC_INTR_DISABLE, .timer_sel = LEDC_TIMER_0, .duty = 0, .hpoint = 0 }
};

// Button event handler
static void button_event_handler(lv_event_t * e) {
    uint8_t index = (uint8_t)(uintptr_t)lv_event_get_user_data(e);

    button_state[index] = !button_state[index];
    if (button_state[index]) {
        ledc_set_duty(ledc_channels[index].speed_mode, ledc_channels[index].channel, (last_brightness[index] * 8191) / 100);
        ledc_update_duty(ledc_channels[index].speed_mode, ledc_channels[index].channel);
        ESP_LOGI(TAG, "LED%d ON, Brightness: %d", index + 1, last_brightness[index]);
    } else {
        ledc_set_duty(ledc_channels[index].speed_mode, ledc_channels[index].channel, 0);
        ledc_update_duty(ledc_channels[index].speed_mode, ledc_channels[index].channel);
        ESP_LOGI(TAG, "LED%d OFF", index + 1);
    }

    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_label_set_text(label, button_state[index] ? "ON" : "OFF");
}

// Slider event handler
static void slider_event_handler(lv_event_t * e) {
    uint8_t index = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *slider = lv_event_get_target(e);

    last_brightness[index] = lv_slider_get_value(slider);
    if (button_state[index]) {
        ledc_set_duty(ledc_channels[index].speed_mode, ledc_channels[index].channel, (last_brightness[index] * 8191) / 100);
        ledc_update_duty(ledc_channels[index].speed_mode, ledc_channels[index].channel);
    }

    ESP_LOGI(TAG, "Slider%d value: %d", index + 1, last_brightness[index]);
}

// Create button
// void create_button(uint8_t index, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
//     lv_obj_t *btn = lv_btn_create(lv_scr_act());
//     lv_obj_set_size(btn, 100, 50);
//     lv_obj_align(btn, align, x_ofs, y_ofs);
//     lv_style_set_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE));
//     lv_obj_add_event_cb(btn, button_event_handler, LV_EVENT_CLICKED, (void *)(uintptr_t)index);

//     lv_obj_t *label = lv_label_create(btn);
//     lv_label_set_text(label, "OFF");
//     lv_obj_center(label);
// }

void create_button(uint8_t index, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 100, 50);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_add_event_cb(btn, button_event_handler, LV_EVENT_CLICKED, (void *)(uintptr_t)index);

    // Set the button color based on the index
    lv_color_t color;
    if (index == 0) {
        color = lv_color_hex(0xFF0000); // Red
    } else if (index == 1) {
        color = lv_color_hex(0x00FF00); // Green
    } else {
        color = lv_color_hex(0x0000FF); // Blue
    }
    lv_obj_set_style_bg_color(btn, color, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Add a label to the button
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "OFF");
    lv_obj_center(label);
}




// Create slider
void create_slider(uint8_t index, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
    lv_obj_t *slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(slider, 30, 200); // Make the slider vertical
    lv_obj_align(slider, align, x_ofs, y_ofs);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT); // Grey background
    lv_obj_set_style_border_color(slider, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT); // Grey border
    lv_obj_add_event_cb(slider, slider_event_handler, LV_EVENT_VALUE_CHANGED, (void *)(uintptr_t)index);
}

// Initialize GPIO and LEDC
void init_hardware(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED1_GPIO) | (1ULL << LED2_GPIO) | (1ULL << LED3_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    for (int i = 0; i < 3; i++) {
        ledc_channel_config(&ledc_channels[i]);
    }
}

// GUI task
void guiTask(void *pvParameter) {
    create_button(0, LV_ALIGN_CENTER, -150, -100);
    create_slider(0, LV_ALIGN_CENTER, -150, 50);

    create_button(1, LV_ALIGN_CENTER, 0, -100);
    create_slider(1, LV_ALIGN_CENTER, 0, 50);

    create_button(2, LV_ALIGN_CENTER, 150, -100);
    create_slider(2, LV_ALIGN_CENTER, 150, 50);

    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Main function
void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    init_hardware();

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
        }
    };

    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();
    lv_init();
    xTaskCreatePinnedToCore(guiTask, "gui", 4096, NULL, 1, NULL, 1);
}
