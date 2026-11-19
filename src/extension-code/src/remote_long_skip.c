#include "cfw_config.h"
#include "fw/core.h"

extern int cfw_is_40elk(void);

#define INPUT ((volatile const uint8_t *)0x00800760)

enum {
    REMOTE_PREVIOUS = 0x6c,
    REMOTE_NEXT = 0x6e,
    STOPPED_KEYS = 0,
    PLAYBACK_KEYS = 1,
    TRACK_EVENT = 2,
    GROUP_NEXT = 0x08,
    GROUP_PREVIOUS = 0x10,
    STOPPED_NEXT = 0x40,
    STOPPED_PREVIOUS = 0x100,
    PLAYBACK_NEXT = 0x4000,
    PLAYBACK_PREVIOUS = 0x10000,
    HOLD_PENDING = 0x04,
    NATIVE_STOPPED_REPEAT = 0x10,
    NATIVE_SEEK_ACTIVE = 0x02,
};

typedef void input_event(uint32_t, uint32_t);
static input_event *const dispatch = (input_event *)THUMB_ADDR(0x3e938);
static uint8_t deferred_key;
static uint8_t deferred_mode;

static uint32_t short_event(uint8_t key, uint8_t mode)
{
    if (mode == STOPPED_KEYS)
        return key == REMOTE_NEXT ? STOPPED_NEXT : STOPPED_PREVIOUS;
    return key == REMOTE_NEXT ? PLAYBACK_NEXT : PLAYBACK_PREVIOUS;
}

static int eligible(uint8_t key, uint8_t mode)
{
    return cfw_config.remote_long_skip && INPUT[0x12] == mode &&
           INPUT[0x15] == key && cfw_is_40elk();
}

static int defer_short(uint32_t type, uint32_t mask, uint8_t mode)
{
    uint8_t key = INPUT[0x13];
    if ((key != REMOTE_NEXT && key != REMOTE_PREVIOUS) ||
        type != TRACK_EVENT || mask != short_event(key, mode) ||
        !eligible(key, mode))
        return 0;

    deferred_mode = mode;
    deferred_key = key;
    return 1;
}

void cfw_long_skip_press(uint32_t type, uint32_t mask)
{
    /* Sony arms the hold timer after this playback dispatch. */
    if (!defer_short(type, mask, PLAYBACK_KEYS))
        dispatch(type, mask);
}

uint32_t cfw_long_skip_stopped_press(uint32_t type, uint32_t mask)
{
    /* Sony arms the stopped repeat timer after this dispatch. Selecting
     * the playback hold flag sends its expiry through the same handler. */
    if (defer_short(type, mask, STOPPED_KEYS))
        return HOLD_PENDING;
    dispatch(type, mask);
    return NATIVE_STOPPED_REPEAT;
}

void cfw_long_skip_release(uint32_t key)
{
    if (deferred_key && deferred_key == key) {
        deferred_key = 0;
        /* On a debounced release the ADC candidate has changed, while
         * the stable key has not yet been updated by the scanner. */
        if (INPUT[0x14] != key && eligible(key, deferred_mode))
            dispatch(TRACK_EVENT, short_event(key, deferred_mode));
    }
    ((void (*)(uint32_t))THUMB_ADDR(0x3e52c))(key);
}

uint32_t cfw_long_skip_expire(uint32_t type, uint32_t mask)
{
    uint8_t key = deferred_key;
    deferred_key = 0;
    if (!key || INPUT[0x13] != key) {
        dispatch(type, mask);
        return NATIVE_SEEK_ACTIVE;
    }

    /* The native Group +/− event selects the adjacent group, or uses
     * Sony's ten-track navigation on a disc without group settings. */
    if (eligible(key, deferred_mode))
        dispatch(TRACK_EVENT, key == REMOTE_NEXT ? GROUP_NEXT : GROUP_PREVIOUS);
    return 0;
}
