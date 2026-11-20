#include "fw/core.h"
#include "fw/sys.h"
#include "fw/sys.h"
#include "fw/display.h"
#include "fw/menu.h"
#include "fw/tracks.h"

#include "menu_resources.h"
#include "menu_macros.h"

#include "cfw_config.h"
#include "chinese_display.h"
#include "track_config.h"
/*
#define MENU_TEXT_ABOUT "By Sir68k"

void make_about_string(struct t_disp_buff *buff)
{
    memcpy(buff, MENU_TEXT_ABOUT, sizeof(MENU_TEXT_ABOUT));
    buff->data[sizeof(MENU_TEXT_ABOUT)] = 0;

    buff->len = sizeof(MENU_TEXT_ABOUT);
    buff->encoding = 0;
}

void cfw_menu_about(uint32_t disp, uint32_t idx)
{

    volatile struct t_disp_buff *buff;

    uint32_t s_mode_a;
    uint32_t s_mode_b;
    uint32_t x;
    uint8_t g_menu_entry_mode;
    uint8_t g_menu_entry_x;

    x = 0;
    g_menu_entry_mode = *fw_menu_entry_mode;
    g_menu_entry_x = *fw_menu_entry_x;
    buff = get_disp_buffer(idx);

    if (g_menu_entry_mode == 1)
    {
        if ((g_menu_entry_x & 1) == 0)
            return;

        disp_reset_buffer(idx);
        make_about_string(buff);

        disp_buffer(idx, buff->b, buff->c);
    }
    else
    {
        if ((g_menu_entry_mode != 2) && (g_menu_entry_mode != 3))
            return;

        if ((g_menu_entry_x & 1) == 0)
        {
            x = g_disp_buff_str_tbl[idx].x;
        }
        else
        {
            disp_reset_buffer(idx);
            cpy_disp_buff_1(idx, &x);
            disp_buffer(idx, 0, 0);
        }

        disp_reset_buffer(idx);
        make_about_string(buff);
        prepare_text_for_scroll(idx);

        s_mode_a = 1;
        if (g_menu_entry_mode != 2)
            s_mode_a = 2;

        s_mode_b = 2;
        if ((x >> 3 & 1) == 0)
            s_mode_b = 0;

        disp_start_scroll(s_mode_a, s_mode_b);
    }

    cpy_disp_buff_2(idx, 0);
}

#define menu_cb_mc_a (const void*) THUMB_ADDR(0x000436d0)
#define menu_cb_mc_b (const void*) THUMB_ADDR(0x00043746)

static func1 * menu_callback_d = (func1*)THUMB_ADDR(0x000aa478);

uint32_t menu_register_event(uint32_t value) {

    if (value == MENU_KEY_STOP || value == MENU_KEY_MENU) {
        return menu_callback_d(MENU_KEY_STOP);
    }

    return 1;
}

static const struct t_menu_callback cfw_menu_callbacks[] = {
    { menu_register_event, 0x43 },
    { menu_register_event, 0x41 },

    { menu_register_event, 0x05 },
    { menu_register_event, 0x07 },
    { menu_register_event, 0x409 },

    { menu_register_event, 0x21 },
    { menu_register_event, 0x01 },
    { menu_register_event, 0x414 },
    { menu_register_event, 0x415 },
    { menu_register_event, 0x416 },
    { menu_register_event, 0x417 },
    { menu_register_event, 0x419 },

    { menu_cb_mc_b, 0x1C },
    { menu_cb_mc_b, 0x49 },
    { menu_cb_mc_b, 0x445 },
    { menu_cb_mc_b, 0x446 },
    { menu_cb_mc_b, 0x10083C },
    { menu_cb_mc_b, 0x83 },

    { (void *)0x00, 0x00 },
};

static const uint32_t config[] = {
    0x02000004,
    0x2D,

    0x11208,
    0x2B,

    0x00,
    0x2C,

    0x00,
    0x00,
};
*/



MENU_VALUE_START(oled_auto)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_NORMAL),    CFW_CONFIG_OLED_MODE_NORMAL)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_DIM_ONLY),  CFW_CONFIG_OLED_MODE_DIM_ONLY)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_FULLY_OFF), CFW_CONFIG_OLED_MODE_FULLY_OFF)
MENU_VALUE_END(oled_auto, cfw_config.cfw_config_oled_auto_off, store_config_to_eeprom)

MENU_VALUE_START(oled_usb)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_U_NORMAL),    CFW_CONFIG_OLED_MODE_NORMAL)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_U_FULLY_OFF), CFW_CONFIG_OLED_MODE_FULLY_OFF)
MENU_VALUE_END(oled_usb, cfw_config.cfw_config_oled_usb_mode, store_config_to_eeprom)

MENU_VALUE_START(oled_brightness_l)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_BR_L_L),   CFW_CONFIG_OLED_BRIGHTNESS_LOW)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_BR_L_M1),  CFW_CONFIG_OLED_BRIGHTNESS_MEDIUM_1)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_BR_L_M2),  CFW_CONFIG_OLED_BRIGHTNESS_MEDIUM_2)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_BR_L_H),   CFW_CONFIG_OLED_BRIGHTNESS_HIGH)
MENU_VALUE_END(oled_brightness_l, cfw_config.cfw_config_oled_brightness_low, store_config_to_eeprom)

MENU_VALUE_START(oled_brightness_h)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_BR_H_L),   CFW_CONFIG_OLED_BRIGHTNESS_LOW)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_BR_H_M1),  CFW_CONFIG_OLED_BRIGHTNESS_MEDIUM_1)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_BR_H_M2),  CFW_CONFIG_OLED_BRIGHTNESS_MEDIUM_2)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(OLED_BR_H_H),   CFW_CONFIG_OLED_BRIGHTNESS_HIGH)
MENU_VALUE_END(oled_brightness_h, cfw_config.cfw_config_oled_brightness_high, store_config_to_eeprom)

static uint32_t apply_chinese_mode(void) {
    chinese_display_invalidate();
    track_view_state.invalidated = 1;
    return store_config_to_eeprom();
}

MENU_VALUE_START(chinese_mode)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(CHINESE_OFF_LABEL), CHINESE_OFF)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(CHINESE_PINYIN_LABEL), CHINESE_PINYIN)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(CHINESE_ORIGINAL_LABEL), CHINESE_ORIGINAL)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(CHINESE_SKIP_LABEL), CHINESE_SKIP)
MENU_VALUE_END(chinese_mode, cfw_config.chinese_mode, apply_chinese_mode)

MENU_VALUE_START(track_style)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(TRACK_STYLE1),  1)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(TRACK_STYLE2),  2)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(TRACK_STYLE3),  3)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(TRACK_STYLE4),  4)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(TRACK_STYLE5),  5)
MENU_VALUE_END(track_style, cfw_config_track_style_mode, track_store_style_config)

MENU_VALUE_START(tmark_override)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(TMARK_NORM),  0)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(TMARK_SKIP),  1)
MENU_VALUE_END(tmark_override, cfw_config.cfw_tmark_override, store_config_to_eeprom)

static uint8_t remote_backlight_choice;

static uint32_t store_remote_backlight(void)
{
    unsigned selected = remote_backlight_choice;
    *(volatile uint8_t *)0x0200464d = selected <= 2 ? selected : 0;
    ((func *)THUMB_ADDR(0x4342c))();
    ((func *)THUMB_ADDR(0x3ff54))();
    cfw_config.remote_light_scale = selected == 3 ? 2 : selected == 4 ? 4 : 0;
    return store_config_to_eeprom();
}

MENU_VALUE_START(remote_light)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(REMOTE_STOCK), 0)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(REMOTE_DOUBLE), 3)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(REMOTE_QUAD), 4)
MENU_VALUE_ENTRY(0x20508, 1)
MENU_VALUE_ENTRY(0x30508, 2)
};
static const struct t_menu_value_config config_values_remote_light = {
    .entries = menu_values_remote_light,
    .ptr_a = &remote_backlight_choice, .ptr_b = &remote_backlight_choice,
    .len = sizeof(menu_values_remote_light) / sizeof(menu_values_remote_light[0]),
};
static const struct t_menu_config_entry menu_cfg_remote_light[] = {
    {{.uint = 0x02010008}, 0x2d},
    {{.ptr = &config_values_remote_light}, MENU_CFG_VAL_MENU},
    {{.ptr = store_remote_backlight}, MENU_CFG_CHANGE_HOOK},
    {{.uint = 0x20}, 0x28},
    {{.uint = 1}, 0x2c},
    {0, 0}
};

MENU_VALUE_START(joy_dial_wake)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(JOY_WAKE_OFF), 0)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(JOY_WAKE_ON), 1)
MENU_VALUE_END(joy_dial_wake, cfw_config.joy_dial_wake, store_config_to_eeprom)

MENU_VALUE_START(remote_long_skip)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(LONG_SKIP_OFF), 0)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(LONG_SKIP_ON), 1)
MENU_VALUE_END(remote_long_skip, cfw_config.remote_long_skip, store_config_to_eeprom)

MENU_VALUE_START(remote_display)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(DISPLAY_STOCK), 0)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(DISPLAY_ITA), 1)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(DISPLAY_ITL), 2)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(DISPLAY_TAL), 3)
MENU_VALUE_END(remote_display, cfw_config.remote_display, store_config_to_eeprom)

MENU_START(oled)
MENU_ITEM_VALUE(MENU_RESOURCE_ID(OLED_AUTO_OFF), oled_auto)
MENU_ITEM_VALUE(MENU_RESOURCE_ID(OLED_USB), oled_usb)
MENU_ITEM_VALUE(MENU_RESOURCE_ID(OLED_BR_LOW), oled_brightness_l)
MENU_ITEM_VALUE(MENU_RESOURCE_ID(OLED_BR_HIGH), oled_brightness_h)
MENU_END(oled)

uint8_t cfw_config_about;
MENU_VALUE_START(about)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(ABOUT_A), 0x00)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(ABOUT_B), 0x01)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(ABOUT_C), 0x02)
MENU_VALUE_ENTRY(MENU_RESOURCE_ID(ABOUT_D), 0x03)
MENU_VALUE_END_SIMPLE(about, cfw_config_about)

MENU_START(cfw)
MENU_ITEM_SUBMENU(MENU_RESOURCE_ID(OLED), oled)
MENU_ITEM_VALUE(MENU_RESOURCE_ID(TRACK_STYLE), track_style)
MENU_ITEM_VALUE(MENU_RESOURCE_ID(CHINESE_MODE), chinese_mode)
MENU_ITEM_VALUE(MENU_RESOURCE_ID(TMARK), tmark_override)
MENU_ITEM_VALUE(MENU_RESOURCE_ID(ABOUT), about)
MENU_END(cfw)
enum { REMOTE_DISPLAY_INDEX = 4 };
static const struct t_basic_menu menu_items_remote_option[] = {
    {0x208, fw_menu_main_callbacks, (void *)0xc9b84, 3},
    {0x308, fw_menu_main_callbacks, (void *)0xc9be4, 3},
    {0x408, fw_menu_main_callbacks, (void *)0xc9c3c, 3},
    {0x508, fw_menu_main_callbacks, menu_cfg_remote_light, 3},
    [REMOTE_DISPLAY_INDEX] = MENU_ITEM_VALUE(MENU_RESOURCE_ID(REMOTE_DISPLAY), remote_display)
    MENU_ITEM_VALUE(MENU_RESOURCE_ID(JOY_WAKE), joy_dial_wake)
    MENU_ITEM_VALUE(MENU_RESOURCE_ID(LONG_SKIP), remote_long_skip)
    {0x708, fw_menu_main_callbacks, (void *)0xc9d04, 3},
    {0x808, fw_menu_main_callbacks, (void *)0xc9d5c, 3},
    {0xa08, (void *)0xc9e64, (void *)0xc9e04, 1},
    {0xd08, fw_menu_main_callbacks, (void *)0xc9f34, 3},
    {0xc08, fw_menu_main_callbacks, (void *)0xc9edc, 3},
    {0xe08, fw_menu_main_callbacks, (void *)0xc9fec, 3},
    {0xf08, fw_menu_main_callbacks, (void *)0xca04c, 6},
    {0x1208, (void *)0xca174, (void *)0xca154, 1},
    {UINT32_MAX, 0, 0, 0}
};
static uint32_t menu_items_remote_option_md[
    (sizeof(menu_items_remote_option) - sizeof(menu_items_remote_option[0])) / sizeof(uint32_t)];

static const struct t_menu_config_entry remote_option_config = {
    {.ptr = menu_items_remote_option}, MENU_CFG_SUB_MENU
};
static const struct t_menu_config_entry remote_option_md_config = {
    {.ptr = menu_items_remote_option_md}, MENU_CFG_SUB_MENU
};

int cfw_is_40elk(void)
{
    if (!(*(volatile const uint8_t *)0x8004dd & 1)) return 0;
    /* The two D1 captures retain RMC02G even after unplugging.
     * Only use that identification while the stock link reports attached. */
    return *(volatile const uint16_t *)0x800506 == 0x4d52 &&
           *(volatile const uint32_t *)0x800508 == 0x47323043 &&
           *(volatile const uint8_t *)0x800661 == 3;
}

const struct t_menu_config_entry *cfw_lookup_option_config(uint32_t config, uint32_t kind)
{
    if (kind == MENU_CFG_SUB_MENU && config == 0xc9aa4 &&
        (*(volatile uint32_t *)0x805644 & 8) && cfw_is_40elk()) {
        if (*disc_type != DISC_TYPE_MD)
            return &remote_option_config;
        /* Build the MD table once, omitting the Hi-MD layout setting. */
        if (!menu_items_remote_option_md[0]) {
            const unsigned prefix = REMOTE_DISPLAY_INDEX * sizeof(menu_items_remote_option[0]);
            memcpy(menu_items_remote_option_md, menu_items_remote_option, prefix);
            memcpy((uint8_t *)menu_items_remote_option_md + prefix,
                   menu_items_remote_option + REMOTE_DISPLAY_INDEX + 1,
                   sizeof(menu_items_remote_option_md) - prefix);
        }
        return &remote_option_md_config;
    }
    return (const void *)((func2 *)THUMB_ADDR(0xaa924))(config, kind);
}

void cfw_open_option_item(const struct t_basic_menu *item, uint32_t selected)
{
    if (item->config == menu_cfg_remote_light) {
        unsigned native = *(volatile uint8_t *)0x0200464d;
        unsigned duration = cfw_config.remote_light_scale;
        remote_backlight_choice = native != 0 ? native : duration == 2 ? 3 : duration == 4 ? 4 : 0;
    }
    ((func2 *)THUMB_ADDR(0xa9a28))((uint32_t)item, selected);
}
