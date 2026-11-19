#include "track.h"
#include "fw/display.h"
#include "cfw_config.h"

uint8_t cfw_disp_buff_0x30[2];

static uint8_t oled_config_to_brightness(uint8_t val)
{
    // The return values must be chosen specifically to match the translation 
    // LUTs used in modern OLED replacements; they are not random!

    switch (val)
    {
    case CFW_CONFIG_OLED_BRIGHTNESS_VERY_LOW:
        return 0x01;
    case CFW_CONFIG_OLED_BRIGHTNESS_LOW:
        return 0x04;
    case CFW_CONFIG_OLED_BRIGHTNESS_MEDIUM_1:
        return 0x10;
    case CFW_CONFIG_OLED_BRIGHTNESS_MEDIUM_2:
        return 0x19;
    default:
    case CFW_CONFIG_OLED_BRIGHTNESS_HIGH:
        return 0x1b;
    }
}

uint32_t cfw_disp_cmd_0x30_set_brightness(uint8_t *p)
{
    cfw_disp_buff_0x30[0] = 0x30;
    if ((*p >> 3 & 1) == 0)
        cfw_disp_buff_0x30[1] = oled_config_to_brightness(cfw_config.cfw_config_oled_brightness_high);
    else
        cfw_disp_buff_0x30[1] = oled_config_to_brightness(cfw_config.cfw_config_oled_brightness_low);

    return queue_disp_cmd(cfw_disp_buff_0x30, 2);
}


uint8_t prev_mode = 0;

__attribute__((naked))
void cfw_enable_blackout()
{
    asm volatile(
        "push {r0-r4, lr}\n" ::: "memory");

    // base struct ptr in r4, store it before gcc messes with it
    register void *conf asm("r4");
    asm volatile("" : "=r"(conf));

    uint8_t current = *((uint8_t *)conf + 0xC);
    uint8_t new = 2;
    
    if ((uint8_t *)conf + 0xC == device_power_mode_b && (current == 3 || current == 4))
        prev_mode = current;
    
    if (current == 3 && cfw_config.cfw_config_oled_auto_off == CFW_CONFIG_OLED_MODE_DIM_ONLY)
        new = 3;

    if (current == 4 && cfw_config.cfw_config_oled_usb_mode == CFW_CONFIG_OLED_MODE_NORMAL)
        new = 4;

    *((uint8_t *)conf + 0xC) = new;

    asm volatile(
        "pop {r0-r4, pc}\n" ::: "memory");
}

static int hidden_battery(void)
{
    return (prev_mode == 3 && cfw_config.cfw_config_oled_auto_off == CFW_CONFIG_OLED_MODE_FULLY_OFF)
        || (prev_mode == 4 && cfw_config.cfw_config_oled_usb_mode == CFW_CONFIG_OLED_MODE_FULLY_OFF);
}

static uint32_t empty;
void cfw_bat_disp_enqueue_cmd(uint32_t cmd, void *buff, uint32_t n)
{
    if (device_power_mode_b[0] == 2 && hidden_battery()) buff = &empty;
    fw_disp_enqueue_cmd(cmd, buff, n);
}

/* Sony keeps body and remote data in different slots. Only the body slot
 * is inspected here. The stock display scheduler continues to service both. */
static uint8_t slept, blank_page, clear_accepted;
static uint32_t previous_battery;
static int plain_blank(void)
{
    uint32_t page = *(VOL32)0x800678;
    uint8_t submode = *(VOL8)0x800680;
    return device_power_mode_b[0] == 2 && (page == 0x23 || page == 0x24)
        && submode != 6 && submode != 10 && *(VOL8)0x800697 != 9
        && !(*(VOL32)0x800560 & 2);
}

void cfw_prepare_oled_icons(void)
{
    uint8_t mode = device_power_mode_b[0];
    if (mode != 2) blank_page = 0;
    if (mode == 1 || mode == 2) {
        slept = 1;
        if (mode == 1 || (blank_page && plain_blank())) return;
    } else if (slept) {
        slept = 0;
        chinese_display_invalidate();
        track_view_state.track_id = -1;
        track_view_state.time_s = -1;
    }
    ((func *)THUMB_ADDR(0x3a5f8))();
}

void cfw_prepare_oled_status(void)
{
    if (device_power_mode_b[0] != 1 && !(blank_page && plain_blank()))
        ((func *)THUMB_ADDR(0x3aba4))();
}

void cfw_oled_frame(void)
{
    if (plain_blank()) {
        uint8_t page = *(VOL32)0x800678;
        /* Only Normal leaves a battery icon visible. Its low-battery blink
         * policy and all four fields presented to the driver are inputs. */
        uint32_t battery = 0xffffffff;
        if (!hidden_battery())
            battery = (*(VOL32)0x8006f4 & 7) | (*(VOL8)0x8006f8 << 3)
                | (*(VOL8)0x8006f9 << 11) | (*(VOL8)0x8006fa << 19)
                | ((uint32_t)(*(VOL8)0x8005c3 != 0) << 27);
        if (blank_page == page && clear_accepted && previous_battery == battery
            && !*(VOL16)0x800ffc) return;
        previous_battery = battery;
        blank_page = page;
    } else {
        blank_page = 0;
    }
    clear_accepted = 0;
    ((func *)THUMB_ADDR(0xae89c))();
    if (*(VOL8)0x802594 >= 50) blank_page = 0;
}

/* A row clear is queued after rendering. Retry on the next frame if the
 * stock 50-entry queue was full when the two-row clear was submitted. */
void cfw_submit_oled_clear(uint32_t cmd, uint8_t *mask, uint32_t priority)
{
    if (blank_page && (*mask & 3) == 3 && *(VOL8)0x802594 < 50)
        clear_accepted = 1;
    fw_disp_enqueue_cmd(cmd, mask, priority);
}

/* End-of-tick driver work also runs after the remote renderer. Do not
 * requeue the same power command for an idle OLED. Always drain outstanding
 * work, including controller activity requested from other firmware paths. */
void cfw_finish_oled_tick(uint32_t selector, uint32_t dim)
{
    if (blank_page && clear_accepted && plain_blank() &&
        *(VOL8)0x805388 == 0x0b && !*(VOL8)0x802594 &&
        !*(VOL8)0x804ecf && !(*(VOL8)0x804ec8 & 0x30)) return;
    if (*(VOL8)0x802594 >= 50) blank_page = 0;
    ((func2 *)THUMB_ADDR(0x9f59c))(selector, dim);
    ((func *)THUMB_ADDR(0x9ebb4))();
}
