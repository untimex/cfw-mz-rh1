#include "cfw_config.h"
#include "fw/core.h"

extern int cfw_is_40elk(void);

static uint8_t phase;
static uint8_t last_count;
static uint8_t last_scale;

uint32_t cfw_remote_start_count(uint32_t mode, const uint8_t *timers)
{
    if (mode != 0 && timers[mode] && timers[mode] != 255 &&
        *(volatile uint8_t *)0x0200464d == 0 && cfw_is_40elk()) {
        phase = 0;
        return cfw_config.remote_light_scale ? 200 : 100;
    }
    return timers[mode];
}

__attribute__((naked)) void cfw_remote_start_hook(void)
{
    __asm__ volatile (
        "push {r0, r2, r3, lr}\n"
        "movs r0, r7\n"
        "bl cfw_remote_start_count\n"
        "movs r1, r0\n"
        "pop {r0, r2, r3}\n"
        "strb r1, [r0, #14]\n"
        "pop {pc}\n");
}

/* Sony 1.0A0. Slot 2's remaining count is at record + 0x0e.
 * The stock caller filters idle (0) and held-on (0xff) before this hook. */
#define REMOTE_TIMER_RECORD ((const void *)0x0200330a)
#define STOCK_BACKLIGHT_MODE (*(volatile const uint8_t *)0x0200464d)

uint32_t cfw_remote_countdown(uint32_t remaining, const void *record)
{
    if (record != REMOTE_TIMER_RECORD)
        return remaining - 1;

    uint8_t scale = cfw_config.remote_light_scale;
    if (scale != 4 || STOCK_BACKLIGHT_MODE != 0 || !cfw_is_40elk()) {
        phase = 0;
        last_scale = 0;
        return remaining - 1;
    }

    if (remaining != last_count || scale != last_scale)
        phase = 0;

    last_scale = scale;
    if (++phase == 2) {
        phase = 0;
        --remaining;
    }

    last_count = remaining;
    return remaining;
}

/* At 0x2a1e2 Sony holds the record in r7 and count in r0. Preserve the
 * scratch registers. Sony tests the stored result immediately on return. */
__attribute__((naked)) void cfw_remote_countdown_hook(void)
{
    __asm__ volatile (
        ".syntax unified\n"
        "push {r1, r2, r3, lr}\n"
        "movs r1, r7\n"
        "bl cfw_remote_countdown\n"
        "strb r0, [r7, #14]\n"
        "pop {r1, r2, r3, pc}\n"
    );
}

#define LIGHT_FLAGS (*(volatile const uint32_t *)0x008005dc)
#define REMOTE_INPUT ((volatile const uint8_t *)0x00800760)
#define ORIGINAL(address) ((func1 *)THUMB_ADDR(address))

enum { REMOTE_CENTER = 0x20, REMOTE_TIMER_PRESET = 2 };
static uint8_t consumed_center;

static uint8_t light_is_on(void)
{
    uint32_t flags = LIGHT_FLAGS;
    return (flags & (1u << 17)) && !(flags & (1u << 22));
}

uint32_t cfw_joy_decode_adc(uint32_t raw)
{
    uint32_t code = ORIGINAL(0x3e440)(raw);
    /* Sony accepts a new ADC code after two consecutive equal samples.
     * Retain the wake-only decision across a single noisy release sample. */
    if (consumed_center && code != REMOTE_CENTER && REMOTE_INPUT[0x14] == code)
        consumed_center = 0;
    return code;
}

/* At 0x3e0c8 Sony has selected a debounced key but has not yet armed
 * press, long-press or release handlers. A nonzero result skips them all.
 * Code 0x20 occurs in the remote ADC table, not in the two body key tables. */
uint32_t cfw_joy_key_gate(uint32_t code)
{
    uint32_t blocked = ORIGINAL(0x3e1a0)(code);
    if (blocked || code != REMOTE_CENTER || REMOTE_INPUT[0x15] != REMOTE_CENTER)
        return blocked;
    if (consumed_center)
        return 1;
    if (!cfw_config.joy_dial_wake || !cfw_is_40elk() || REMOTE_INPUT[0x13] >= code || light_is_on())
        return 0;
    if (LIGHT_FLAGS & (1u << 22))  /* remote light output inhibited */
        return 0;
    if (((func *)THUMB_ADDR(0x43404))() != 0)  /* stock Backlight: Auto */
        return 0;

    /* Ask the stock UI to start its finite timer; it can veto this request.
     * Leave the key alone in modes where waking is unavailable. */
    ORIGINAL(0x46824)(REMOTE_TIMER_PRESET);
    if (!*(volatile const uint8_t *)0x02003318)
        return 0;
    /* 0x2a292(2) tail path returns the stock setter status. */
    if ((int32_t)ORIGINAL(0x2a292)(2) < 0)
        return 0;
    consumed_center = 1;
    return 1;
}
