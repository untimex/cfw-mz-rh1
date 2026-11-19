#pragma once
#include "fw/core.h"

#define CFW_BLOCK_ID 18

#define CFW_CONFIG_OLED_BRIGHTNESS_VERY_LOW  1
#define CFW_CONFIG_OLED_BRIGHTNESS_LOW       2
#define CFW_CONFIG_OLED_BRIGHTNESS_MEDIUM_1  3
#define CFW_CONFIG_OLED_BRIGHTNESS_MEDIUM_2  4
#define CFW_CONFIG_OLED_BRIGHTNESS_HIGH      5

#define CFW_CONFIG_OLED_MODE_NORMAL     1
#define CFW_CONFIG_OLED_MODE_DIM_ONLY   2
#define CFW_CONFIG_OLED_MODE_FULLY_OFF  3

struct cfw_config {
    uint8_t cfw_config_oled_brightness_low;
    uint8_t cfw_config_oled_brightness_high;
    uint8_t cfw_config_oled_auto_off;
    uint8_t cfw_config_oled_usb_mode;
    uint8_t chinese_mode;
    uint8_t remote_long_skip;
    uint8_t v_7;
    uint8_t v_8;
    uint8_t cfw_tmark_override;
    uint8_t remote_light_scale;
    uint8_t joy_dial_wake;
    uint8_t remote_display;
    uint32_t track_layout_display[2];
};

void load_config_from_eeprom();
uint32_t store_config_to_eeprom();
extern struct cfw_config cfw_config;

void cfw_eeprom_read(uint32_t src, void* buff, uint32_t len);
void cfw_eeprom_write(uint32_t dst, void* buff, uint32_t len);